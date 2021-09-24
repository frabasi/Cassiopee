# bufferized parallel probe
import Converter.PyTree as C
import Geom.PyTree as D
import Converter.Mpi as Cmpi
import Converter.Internal as Internal
import Generator.PyTree as G
import Converter.Filter as Filter
import Converter.Distributed as Distributed
import numpy

class Probe:

    def init0(self):
        # probe Coordinates
        self._posX = None
        self._posY = None
        self._posZ = None
    
        # probe center index in block
        self._ind = None
        # bloc name containing probe
        self._blockName = None
        # the proc bloc is on
        self._proc = 0
        # distance probe-node
        self._dist = None

        # pointer on pyTree probe is attached to
        self._t = None

        # file attached to
        self._fileName = None
        # current container in file to write to
        self._filecur = 0

        # internal buffer
        self._bsize = 100
        # current position in buffer
        self._icur = 0

        # list of extracted field names and pointers
        self._fields = []
        self._pfields = []

        # zone storing probe data
        self._pZone = None

    # init from position
    def __init__(self, t, X, fileName, fields=None, append=True):
        self.init0()
        self._fileName = fileName
        self._t = t
        self._posX = X[0]
        self._posY = X[1]
        self._posZ = X[2]
        self.locateProbe(t, X)
        self.checkVariables(fields)
        self.createProbeZone()
        self.checkFile(append=append)
        
    # locate probe in t from position X
    # IN: posX, posY, posZ
    # OUT: ind, blockName, dist
    def locateProbe(self, t, X):
        P = D.point(X)
        zones = Internal.getZones(t)
        dist = 1.e16; blockName = None; ind = 0
        
        for z in zones:
            hook = C.createHook(z, function='nodes')
            (i, d) = C.nearestNodes(hook, P)
            if d[0] < dist: dist = d[0]; blockName = z[0]; ind = i[0]
            C.freeHook(hook)

        # parallel
        ret = Cmpi.allgather(dist)
        
        dist = 1.e16
        for p, i in enumerate(ret):
            if i is not None and i < dist: 
                dist = i; proc = p
        print('Info: probe found on proc:', proc, dist)
        
        [ind,blockName,dist,proc] = Cmpi.bcast([ind,blockName,dist,proc], root=proc)

        # set
        self._ind = ind
        self._blockName = blockName
        self._dist = dist
        self._proc = proc
        return None
        
    # locate probe from ind and blockName
    def locateProbe2(self, t, ind, blockName, proc):
        self._ind = ind
        self._blockName = blockName
        self._dist = 0.
        self._proc = proc
        return None

    # print information on probe
    def print(self):
        if Cmpi.rank != self._proc: return
        print('Info: Position: ', self._posX, self._posY, self._posZ)
        print('Info: Block', self._blockName)
        print('Info: Block global index:', self._ind)
        print('Info: distance probe-node:', self._dist)
        print('Info: filecur:', self._filecur)
        print('Info: icur:', self._icur)
        return None

    # Create the probe zone with buffer size
    def createProbeZone(self):
        if Cmpi.rank != self._proc: return
        self._pZone = G.cart((0,0,0), (1,1,1), (self._bsize,1,1))
        self._pZone[0] = 'probe'
        C._initVars(self._pZone, '{CoordinateX}=-1.') # time sentinel
        # create vars in probe
        for v in self._fields:
            C._initVars(self._pZone, '{nodes:%s}=0.'%v)
        return None

    # Check file, if it doesnt exist, write probe zone in it
    # else get the filecur
    def checkFile(self, append=True):
        if Cmpi.rank != self._proc: return
        if append:
            create = False
            try:
                tl = Cmpi.convertFile2SkeletonTree(self._fileName)
                nodes = Internal.getNodesFromName(tl, 'GridCoordinates#*')
                self._filecur = len(nodes)
            except: create = True
        else: create = True
        if create:
            C.convertPyTree2File(self._pZone, self._fileName)
            self._filecur = 0
        # load GC
        nodes = Distributed.readNodesFromPaths(self._fileName, ['CGNSTree/Base/probe/GridCoordinates'])
        px = Internal.getNodeFromName2(self._pZone, 'CoordinateX')
        px2 = Internal.getNodeFromName2(nodes[0], 'CoordinateX')
        px[1] = px2[1]
        px = px[1]
        a = px > -0.5
        self._icur = numpy.count_nonzero(a)
        # load FS
        nodes = Distributed.readNodesFromPaths(self._fileName, ['CGNSTree/Base/probe/FlowSolution'])
        cont = Internal.getNodeFromName2(self._pZone, 'FlowSolution')
        cont[2] = nodes[0][2]
        print('Info: filecur:', self._filecur)
        print('Info: icur:', self._icur)
        return None

    # verifie la var list dans t, conserve les pointeurs d'acces
    def checkVariables(self, varList):
        if Cmpi.rank != self._proc: return
        block = Internal.getNodeFromName2(self._t, self._blockName)
        if varList is None: # get all vars from blockName
            varList = C.getVarNames(block, excludeXYZ=True)[0]
        for v in varList:
            vs = v.split(':')
            contName = Internal.__FlowSolutionNodes__
            if len(vs) == 2 and vs[0] == 'centers': 
                contName = Internal.__FlowSolutionCenters__
                v = vs[1]
            cont = Internal.getNodeFromName1(block, contName)
            if cont is None: 
                raise ValueError("probe: can not find solution container in t.")
            var = Internal.getNodeFromName1(cont, v)
            if var is None:
                raise ValueError("probe: can not find field %s in t."%v)
            self._fields.append(v)
            self._pfields.append(var[1])
        return None

    # trigger extraction on current t
    def extract(self, time):
        if Cmpi.rank != self._proc: return None
        # set time in CoordinateX
        px = Internal.getNodeFromName2(self._pZone, 'CoordinateX')[1]
        px = px.ravel('k')
        px[self._icur] = time
        for c in range(len(self._fields)):
            f = self._pfields[c].ravel('k')
            v = self._fields[c]
            pf = Internal.getNodeFromName2(self._pZone, v)[1]
            pf = pf.ravel('k')
            pf[self._icur] = f[self._ind]
        self._icur += 1
        if self._icur >= self._bsize: self.flush()
        return None

    # flush containers of probe
    def flush(self):
        if Cmpi.rank != self._proc: return None
        # flush containers
        gc = Internal.getNodeFromName1(self._pZone, 'GridCoordinates')
        fc = Internal.getNodeFromName1(self._pZone, 'FlowSolution')
        if self._icur >= self._bsize: # because buffer is out
            gc = Internal.copyNode(gc)
            gc[0] = 'GridCoordinates#%d'%self._filecur
            fc = Internal.copyNode(fc)
            fc[0] = 'FlowSolution#%d'%self._filecur
            print('Info: flush %d (full).'%self._filecur)
            paths = ['CGNSTree/Base/probe','CGNSTree/Base/probe']
            nodes = [gc,fc]
            Distributed.writeNodesFromPaths(self._fileName, paths, nodes, mode=0)
            self._filecur += 1
            C._initVars(self._pZone, '{CoordinateX}=-1.') # time sentinel
            for v in self._fields: C._initVars(self._pZone, '{%s}=0.'%v)
            self._icur = 0
        else: # explicit flush
            nodes = [gc,fc]
            paths = ['CGNSTree/Base/probe/GridCoordinates', 'CGNSTree/Base/probe/FlowSolution']
            Distributed.writeNodesFromPaths(self._fileName, paths, nodes, mode=1)
        return None