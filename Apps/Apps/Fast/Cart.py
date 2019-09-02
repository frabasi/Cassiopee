import Generator
import Generator.PyTree as G
import Converter.Distributed as Distributed
import KCore.test as test
import Converter.PyTree as C
import Converter.Mpi as Cmpi
import Converter.Internal as Internal
import Connector.ToolboxIBM as TIBM
import Connector.PyTree as X
import Transform.PyTree as T
# Generates in parallel a Cartesian mesh
# if ext=0, match and nearmatch joins are not computed
def generateCartMesh(t_case, snears=0.01, dfar=10., dfarList=[], vmin=21, check=False, tbox=None, snearsf=None, ext=2, dimPb=3):
    
    if isinstance(t_case, str): tb = C.convertFile2PyTree(t_case)
    else: tb = t_case

    # list of dfars
    if dfarList == []:
        zones = Internal.getZones(tb)
        dfarList = [dfar*1.]*len(zones)
        for c, z in enumerate(zones): 
            n = Internal.getNodeFromName2(z, 'dfar')
            if n is not None: dfarList[c] = Internal.getValue(n)*1.
    # a mettre dans la classe ou en parametre de prepare1 ??? 
    to = None
    symmetry = 0
    fileout = None
    if check: fileout = 'octree.cgns'
    DEPTH= ext
    rank = Cmpi.rank

    # Octree identical on all procs
    test.printMem('>>> Octree unstruct [start]')
    # Build octree
    o = TIBM.buildOctree(tb, snears=snears, snearFactor=1., dfar=dfar, dfarList=dfarList, to=to, tbox=tbox, snearsf=snearsf,
                         dimPb=dimPb, vmin=vmin, symmetry=symmetry, fileout=fileout, rank=rank)
    # build parent octree 3 levels higher
    # returns a list of 4 octants of the parent octree in 2D and 8 in 3D
    parento = TIBM.buildParentOctrees__(o, tb, snears=snears, snearFactor=4., dfar=dfar, dfarList=dfarList, to=to, tbox=tbox, 
                                        snearsf=snearsf, dimPb=dimPb, vmin=vmin, symmetry=symmetry, fileout=fileout, rank=rank)
    test.printMem(">>> Octree unstruct [end]")

    # Split octree
    test.printMem(">>> Octree unstruct split [start]")
    bb = G.bbox(o)
    NPI = Cmpi.size
    if NPI == 1: p = Internal.copyRef(o) # keep reference
    else: p = T.splitNParts(o, N=NPI, recoverBC=False)[rank]
    del o
    test.printMem(">>> Octree unstruct split [end]")

    # fill vmin + merge in parallel
    test.printMem(">>> Octree struct [start]")
    res = TIBM.octree2StructLoc__(p, vmin=vmin, ext=-1, optimized=0, parento=parento, sizeMax=1000000)
    del p 
    if parento is not None:
        for po in parento: del po
    t = C.newPyTree(['CARTESIAN', res])
    zones = Internal.getZones(t)
    for z in zones: z[0] = z[0]+'X%d'%rank
    Cmpi._setProc(t, rank)
    C._addState(t, 'EquationDimension', dimPb)
    test.printMem(">>> Octree struct [end]")
    
    # Add xzones for ext
    if ext>0:
        test.printMem(">>> extended cart grids [start]")
        tbb = Cmpi.createBBoxTree(t)
        interDict = X.getIntersectingDomains(tbb)
        graph = Cmpi.computeGraph(tbb, type='bbox', intersectionsDict=interDict, reduction=False)
        del tbb
        Cmpi._addXZones(t, graph, variables=[], cartesian=True)
        test.printMem(">>> extended cart grids [after add XZones]")
        zones = Internal.getZones(t)
        coords = C.getFields(Internal.__GridCoordinates__, zones, api=2)
        coords = Generator.generator.extendCartGrids(coords, DEPTH+1, 1)
        C.setFields(coords, zones, 'nodes')
        Cmpi._rmXZones(t)
        coords = None; zones = None
        test.printMem(">>> extended cart grids (after rmXZones) [end]")
    
        TIBM._addBCOverlaps(t, bbox=bb)
        TIBM._addExternalBCs(t, bbox=bb, dimPb=dimPb)

    else:
        if dimPb == 3: ratios = [[2,2,2],[4,4,4],[8,8,8],[16,16,16]]
        else: ratios = [[2,2,1],[4,4,1],[8,8,1],[16,16,1]]
        tbb = Cmpi.createBBoxTree(t)
        interDict = X.getIntersectingDomains(tbb)
        graph = Cmpi.computeGraph(tbb, type='bbox', intersectionsDict=interDict, reduction=False)
        del tbb
        Cmpi._addXZones(t, graph, variables=[], cartesian=True)
        test.printMem(">>> extended cart grids [after add XZones]")
        t = X.connectMatch(t, dim=dimPb)
        for ratio0 in ratios:
            t = X.connectNearMatch(t,ratio=ratio0,dim=dimPb)
        Cmpi._rmXZones(t)
        C._fillEmptyBCWith(t,"nref","BCFarfield",dim=dimPb)
    return t

#====================================================================================
# Prend les snear dans t, les multiplie par factor
def snearFactor(t, factor=1.):
    tp = Internal.copyRef(t)
    _snearFactor(t, value)
    return tp

def _snearFactor(t, factor=1.):
    zones = Internal.getZones(t)
    for z in zones:
        nodes = Internal.getNodesFromName2(z, 'snear')
        for n in nodes:
            Internal._setValue(n, factor*Internal.getValue(n))
    return None

# Set snear in zones
def setSnear(t, value):
    tp = Internal.copyRef(t)
    _setSnear(t, value)
    return tp

def _setSnear(z, value):
    zones = Internal.getZones(z)
    for z in zones:
        Internal._createUniqueChild(z, '.Solver#define', 'UserDefinedData_t')
        n = Internal.getNodeFromName1(z, '.Solver#define')
        Internal._createUniqueChild(n, 'snear', 'DataArray_t', value)
    return None

# Set dfar in zones 
def setDfar(t, value):
    tp = Internal.copyRef(t)
    _setDfar(t, value)
    return tp

def _setDfar(z, value):
    zones = Internal.getZones(z)
    for z in zones:
        Internal._createUniqueChild(z, '.Solver#define', 'UserDefinedData_t')
        n = Internal.getNodeFromName1(z, '.Solver#define')
        Internal._createUniqueChild(n, 'dfar', 'DataArray_t', value)
    return None
