# - Filters -
import Internal
import PyTree
import Converter
from Distributed import convert2PartialTree, _convert2PartialTree, convert2SkeletonTree, _convert2SkeletonTree, convertFile2SkeletonTree, _readPyTreeFromPaths, _readZones, \
_convert2SkeletonTree, readNodesFromPaths, writeNodesFromPaths, writePyTreeFromPaths, deletePaths
import numpy

# Prend un fileName, si c'est toto/*, rend la liste des fichiers
def expand(fileName):
  s = fileName.rsplit('/', 1)
  if len(s) > 1 and (s[1] == '*' or s[1] == '*.cgns' or s[1] == '*.hdf'): # multiple
    import glob
    if s[1] == '*': files = glob.glob(s[0]+'/*.cgns')
    else: files = glob.glob(fileName)
  return files

#============================================================================
# Lecture des noms Base/Zones + dims
# Retourne un squelette (depth=3) + la liste des zones path names (znp)
# si baseNames: retourne les znp pour la base donnee
# si familyZoneNames: retourne les znp des zones de famille donnee
# si BCType: retourne les znp des zones contenant une BC de type BCType
# ou de famille 'familySpecified:WALL'
#============================================================================
def readZoneHeaders(fileName, format=None, baseNames=None, familyZoneNames=None, BCType=None):
    a = convertFile2SkeletonTree(fileName, format, maxDepth=3, maxFloatSize=6)
    # filter by base names
    if baseNames is not None:
        if not isinstance(baseNames, list): baseNames = [baseNames]
        znp = []
        for b in baseNames:
            l = Internal.getNodeFromName1(a, b)
            if l is not None:
               zs = Internal.getZones(l)
               for z in zs: znp.append('/'+b+'/'+z[0])
    # filter by zone family names
    elif familyZoneNames is not None:
        if not isinstance(familyZoneNames, list): familyZoneNames = [familyZoneNames]
        znp = []
        bases = Internal.getBases(a)
        for b in bases:
            zones = Internal.getZones(b)
            for z in zones:
                f = Internal.getNodeFromType1(z, 'FamilyName_t')
                if f is not None:
                    for fz in familyZoneNames:
                        if Internal.getValue(f) == fz:
                            znp.append('/'+b[0]+'/'+z[0])
    elif BCType is not None:
        families = PyTree.getFamilyBCNamesOfType(a, BCType)
        s = BCType.split(':')
        if len(s) == 2: families.append(s[1])
        print families
        if BCType == 'BCWall':
          families1 = PyTree.getFamilyBCNamesOfType(a, 'BCWallInviscid')
          families2 = PyTree.getFamilyBCNamesOfType(a, 'BCWallViscous*')
        znp = []
        bases = Internal.getBases(a)
        for b in bases:
            zones = Internal.getZones(b)
            for z in zones:
                path = '/'+b[0]+'/'+z[0]
                _readPyTreeFromPaths(a, fileName, path+'/ZoneBC')
                nodes = Internal.getNodesFromValue(z, BCType)
                nodes += PyTree.getFamilyBCs(z, families)
                #_convert2SkeletonTree(z)
                if nodes != []: znp.append(path)
    else:
        znp = Internal.getZonePaths(a, pyCGNSLike=True)
    return a, znp

#========================================================================
# Load par containeurs
# Load les containeurs "cont" dans a pour les znp donnes
# cont='GridCoordinates', 'FlowSolution'
#========================================================================
def _loadContainers(a, fileName, znp, cont, format=None):
    if isinstance(cont, list): conts = cont
    else: conts = [cont]
    if isinstance(znp, list): znps = znp
    else: znps = [znp]
    for p in znps:
        paths = []
        for c in conts: paths.append('%s/%s'%(p,c))
        _readPyTreeFromPaths(a, fileName, paths, format)
    return None

#=========================================================================
# Load connectivity
# Load les connectivites dans a pour les znps donnes
#=========================================================================
def _loadConnectivity(a, fileName, znp, format=None):
    if isinstance(znp, list): znps = znp
    else: znps = [znp]
    for p in znps:
        z = Internal.getNodeFromPath(a, p)
        elts = Internal.getNodesFromType1(z, 'Elements_t')
        paths = []
        for e in elts: paths.append(p+'/'+e[0])
        _readPyTreeFromPaths(a, fileName, paths, format)    
    return None

#==========================================================================
# Load par variables
# Load les variables "var" pour les znp donnes
# var='Density', 'centers:Density', 'FlowSolution/Density' or list of vars
#==========================================================================
def _loadVariables(a, fileName, znp, var, format):
    if isinstance(var, list): vars = var
    else: vars = [var]
    if isinstance(znp, list): znps = znp
    else: znps = [znp]
    fvars = []; cont = []
    for v in vars:
       s = v.split(':')
       if v[0:10] == 'Coordinate': 
        fvars.append(Internal.__GridCoordinates__+'/'+v); cont = Internal.__GridCoordinates__
       elif len(s) == 2 and s[0] == 'centers': 
        fvars.append(Internal.__FlowSolutionCenters__+'/'+s[1]); cont = Internal.__FlowSolutionCenters__
       elif len(s) == 2 and s[0] == 'nodes': 
        fvars.append(Internal.__FlowSolutionNodes__+'/'+s[1]); cont = Internal.__FlowSolutionNodes__
       else:
        s = v.split('/')
        if len(s) == 2: 
          fvars.append(s[0]+'/'+s[1]); cont = Internal.__FlowSolutionNodes__
        else: 
          fvars.append(Internal.__FlowSolutionNodes__+'/'+v); cont = Internal.__FlowSolutionNodes__

    for p in znps:
        paths = []
        for v in fvars: paths.append('%s/%s'%(p,v))
        _readPyTreeFromPaths(a, fileName, paths, format)
        # Ensure location in containers
        zp = Internal.getNodeFromPath(a, p)
        fp = Internal.getNodeFromPath(a, paths[0])
        if zp is not None and fp is not None:
            c = Internal.getNodeFromName1(zp, cont)
            if c is not None:
              if PyTree.getNPts(zp) != fp[1].size:
                Internal.createUniqueChild(c, 'GridLocation', 'GridLocation_t', 'CellCenter')
    return None

#==================================================================================
# Fully load all path in a
#==================================================================================
def _loadZones(a, fileName, znp, format=None):
  if isinstance(znp, list): znps = znp
  else: znps = [znp]
  _readPyTreeFromPaths(a, fileName, znps, format)

# Fully load zoneBC_t and GridConnectivity_t of znp
def _loadZoneBCs(a, fileName, znp, format=None):
  if isinstance(znp, list): znps = znp
  else: znps = [znp]
  paths = []
  for p in znps:
    n = Internal.getNodeFromPath(a, p)
    if n is not None:
      children = n[2]
      for i in children:
        if i[3] == 'ZoneBC_t': paths.append(p+'/'+i[0])
        if i[3] == 'ZoneGridConnectivity_t': paths.append(p+'/'+i[0])
  if paths != []: _readPyTreeFromPaths(a, fileName, paths, format)
  return None

# Load zone BC but without loading BCDataSet fields
def _loadZoneBCsWoData(a, fileName, znp, format=None):
  if isinstance(znp, list): znps = znp
  else: znps = [znp]
  paths = []
  for p in znps:
    n = Internal.getNodeFromPath(a, p)
    if n is not None:
      children = n[2]
      for i in children:
        if i[3] == 'GridConnectivity_t': paths.append(p+'/'+i[0])
  if paths != []: _readPyTreeFromPaths(a, fileName, paths, format)
  paths = []
  for p in znps:
    n = Internal.getNodeFromPath(a, p)
    if n is not None:
      children = n[2]
      for i in children:
        if i[3] == 'ZoneBC_t': paths.append(p+'/'+i[0])
  print paths
  if paths != []: _readPyTreeFromPaths(a, fileName, paths, format, maxDepth=2, setOnlyValue=False)
  return None

# Load fully extras node (not Zone or Base)
def _loadExtras(a, fileName, format=None):
  # Fully load nodes of level 1 except CGNSBase_t
  children = a[2]
  paths = []
  for i in children:
    if i[3] != 'CGNSBase_t': paths.append(i[0])
  
  # Fully load nodes of level 2 except Zone_t
  bases = Internal.getBases(a)
  for b in bases:
    children = b[2]
    for i in children:
      if i[3] != 'Zone_t': paths.append(b[0]+'/'+i[0])
  if paths != []: _readPyTreeFromPaths(a, fileName, paths, format)
  return None

# get variables: return a list
# this doesnt mean that all vars are defined in all zones!
def getVariables(fileName, znp, cont=None, format=None):
  if isinstance(znp, list): znps = znp
  else: znps = [znp]
  if cont is None: # check containers in files
    conts = set()
    nodes = readNodesFromPaths(fileName, znp, format, maxDepth=1, maxFloatSize=0)
    for n in nodes:
      for j in n[2]:
        if j[3] == 'FlowSolution_t': conts.add(j[0])
    cont = list(conts)
  elif cont == 'centers': cont = [Internal.__FlowSolutionCenters__]
  elif cont == 'nodes': cont = [Internal.__FlowSolutionNodes__]
  elif isinstance(cont, str): cont = [cont]
  paths = []
  for c in cont:
    for p in znps:
      paths.append(p+'/'+c)
  vars = set()
  nodes = readNodesFromPaths(fileName, paths, format, maxDepth=1, maxFloatSize=0)
  for n in nodes:
    for j in n[2]:
      if j[3] == 'DataArray_t': vars.add(n[0]+'/'+j[0])
  return list(vars)

# Return the list of variables in BCDataSet
def getBCVariables(a, fileName, znp, cont=None, format=None):
  if isinstance(znp, list): znps = znp
  else: znps = [znp]
  paths = []
  for p in znps:
      zp = Internal.getNodeFromPath(a, p)
      if zp is not None:
        zoneBC = Internal.getNodesFromType1(zp, 'ZoneBC_t')
        for zbc in zoneBC:
          paths.append(p+'/'+zbc[0])
  nodes = readNodesFromPaths(fileName, paths, format, maxDepth=-1, maxFloatSize=0)
  vars = set()
  for n in nodes:
    p = Internal.getNodesFromType(n, 'BCData_t')
    for j in p:
      for k in j[2]:
        if k[3] == 'DataArray_t': vars.add(k[0])
  return list(vars)

# Load un bboxTree
def bboxTree(fileName):
    # Load only bboxes
    return None

# Load only zones that match a bbox
def isInBBox(a, fileName, bbox, znp):
    xmin = bbox[0]; ymin = bbox[1]; zmin = bbox[2]
    xmax = bbox[3]; ymax = bbox[4]; zmax = bbox[5]
    if isinstance(znp, list): znps = znp
    else: znps = [znp]
    out = []
    for p in znps:
       z = Internal.getNodeFromPath(a, p)
       path = ['%s/GridCoordinates/CoordinateX'%p]
       _readPyTreeFromPaths(a, fileName, path)
       pt = Internal.getNodeFromName2(z, 'CoordinateX')
       vmin = numpy.min(pt[1])
       vmax = numpy.max(pt[1])
       pt[1] = None
       if vmax < xmin or vmin > xmax: out.append(False); continue
       path = ['%s/GridCoordinates/CoordinateY'%p]
       _readPyTreeFromPaths(a, fileName, path)
       pt = Internal.getNodeFromName2(z, 'CoordinateY')
       vmin = numpy.min(pt[1])
       vmax = numpy.max(pt[1])
       pt[1] = None
       if vmax < ymin or vmin > ymax: out.append(False); continue
       path = ['%s/GridCoordinates/CoordinateZ'%p]
       _readPyTreeFromPaths(a, fileName, path)
       pt = Internal.getNodeFromName2(z, 'CoordinateZ')
       vmin = numpy.min(pt[1])
       vmax = numpy.max(pt[1])
       pt[1] = None
       if vmax < zmin or vmin > zmax: out.append(False); continue
       out.append(True)
    if len(out) == 1: out = out[0]
    return out

#==========================================================
class Handle:
  """Handle for filter."""
  def __init__(self, fileName, where=None):
    self.fileName = fileName
    self.fileVars = None # vars existing in file
    self.fileBCVars = None # BCDataSet vars existing in file
    self.znp = None # zone paths existing in file
    self.size = None # size des zones du fichier
    self.bary = None # Barycentres zones du fichier
    self.bbox = None # BBox zones du fichier
    self.hmoy = None # pas moyen des zones du fichier
    self.format = Converter.checkFileType(fileName) # Real format of file
    
  # Retourne les chemins des zones de a
  def getZonePaths(self, a):
    out = []
    bases = Internal.getBases(a)
    for b in bases:
      zones = Internal.getZones(b)
      for z in zones: out.append('/'+b[0]+'/'+z[0])
    return out

  def setZnp(a):
    self.znp = []
    bases = Internal.getBase()
    for b in bases:
      zones = Internal.getZones(b)
      for z in zones: self.znp.append('/'+b[0]+'/'+z[0])

  # Retourne les variables du fichier
  def getVariables(self, a=None, cont=None):
    if a is not None: p = self.getZonePaths(a)
    else: p = [self.znp[0]] # only first zone
    vars = getVariables(self.fileName, p, cont, self.format)
    self.fileVars = vars
    return vars

  # Retourne les variables de BCDatSet du fichier
  def getBCVariables(self, a):
    if a is not None: p = self.getZonePaths(a)
    else: p = self.znp
    vars = getBCVariables(a, self.fileName, p, None, self.format)
    self.fileBCVars = vars
    return vars

  # Charge un squelette, stocke les chemins des zones du fichier (znp)
  # Stocke le nombre de pts de chaque zone
  def loadSkeleton(self):
    # lecture squelette
    a, self.znp = readZoneHeaders(self.fileName, self.format)
    # evaluation taille des zones
    self.size = {}
    for zn in self.znp:
      z = Internal.getNodeFromPath(a, zn)
      if z[1] is not None:
        pt = z[1].ravel('k')
        if pt[2] == 0: s = pt[0]
        else: s = pt[0]*pt[1]*pt[2]
      else: s = 0
      self.size[zn] = s
    return a

  # Calcul et stocke des infos geometriques sur les zones
  def geomProp(self, znp=None):
    if znp is None: znp = self.znp

    if self.bbox is None: self.bbox = {}
    if self.bary is None: self.bary = {}
    if self.hmoy is None: self.hmoy = {}
    for p in znp:
       if self.bbox.has_key(p): continue
       xc = 0.; yc = 0.; zc = 0.
       bbox = [0.,0.,0.,0.,0.,0.]
       hmoy = 0.; hmax = 0.; hmin = 0.
       # Coordinate X
       path = '%s/GridCoordinates/CoordinateX'%p
       n = readNodesFromPaths(self.fileName, path, maxDepth=0)
       vmin = numpy.min(n[1])
       vmax = numpy.max(n[1])
       bbox[0] = vmin; bbox[3] = vmax
       xc = 0.5*(vmin+vmax)
       # Coordinate Y
       path = '%s/GridCoordinates/CoordinateY'%p
       n = readNodesFromPaths(self.fileName, path, maxDepth=0)
       vmin = numpy.min(n[1])
       vmax = numpy.max(n[1])
       bbox[1] = vmin; bbox[4] = vmax
       yc = 0.5*(vmin+vmax)
       # Coordinate Z
       path = '%s/GridCoordinates/CoordinateZ'%p
       n = readNodesFromPaths(self.fileName, path, maxDepth=0)
       vmin = numpy.min(n[1])
       vmax = numpy.max(n[1])
       bbox[2] = vmin; bbox[5] = vmax
       zc = 0.5*(vmin+vmax)
       self.bary[p] = (xc,yc,zc)
       self.bbox[p] = bbox
       dhx = bbox[3]-bbox[0]
       dhy = bbox[4]-bbox[1]
       dhz = bbox[5]-bbox[2]
       npts = self.size[p]**0.33
       self.hmoy[p] = (dhx+dhy+dhz)*0.33*npts

  # Charge les Grid coordinates + grid connectivity + BC
  # pour toutes les zones de a
  def _loadGCBC(self, a):
    self._loadContainer(a, 'GridCoordinates')
    self._loadConnectivity(a)
    self._loadZoneBCs(a)
    Internal._fixNGon(a)
    return None

  # Charge tous les noeuds extra de a
  def _loadExtras(self, a):
    _loadExtras(a, self.fileName, self.format)
    return None

  # Charge completement toutes les zones de a ou de chemin fourni
  def _loadZones(self, a, znp=None):
    if znp is None: znp = self.getZonePaths(a)
    _loadZones(a, self.fileName, znp, self.format)
    return None

  # Charge les grid coordinates + grid connectivity + BC
  # pour les zones specifiees
  def _loadZonesWoVars(self, a, znp=None):
    if znp is None: znp = self.getZonePaths(a)
    # Read paths as skeletons
    _readPyTreeFromPaths(a, self.fileName, znp, self.format, maxFloatSize=0)
    _loadContainers(a, self.fileName, znp, 'GridCoordinates', self.format)
    _loadConnectivity(a, self.fileName, znp, self.format)
    _loadZoneBCs(a, self.fileName, znp, self.format)
    for zp in znp:
      _convert2PartialTree(Internal.getNodeFromPath(a, zp))
    return None

  # Charge toutes les BCs (avec BCDataSet) des zones de a  
  def _loadZoneBCs(self, a):
    p = self.getZonePaths(a)
    _loadZoneBCs(a, self.fileName, self.znp, self.format)
    return None

  # Charge toutes les BCs (sans BCDataSet) des zones de a
  def _loadZoneBCsWoData(self, a):
    p = self.getZonePaths(a)
    _loadZoneBCsWoData(a, self.fileName, p, self.format)
    return None

  # Charge la connectivite pour toutes les zones de a
  def _loadConnectivity(self, a):
    p = self.getZonePaths(a)
    _loadConnectivity(a, self.fileName, p, self.format)
    return None

  # Charge le container "cont" pour toutes les zones de a
  def _loadContainer(self, a, cont):
    p = self.getZonePaths(a)
    _loadContainers(a, self.fileName, p, cont, self.format)
    return None

  # Charge la ou les variables "var" pour toutes les zones de a
  def _loadVariables(self, a, var):
    p = self.getZonePaths(a)
    _loadVariables(a, self.fileName, p, var, self.format)
    return None
  
  def isInBBox(self, a, bbox, znp=None):
    if znp is None: znp = self.znp
    return isInBBox(a, self.fileName, bbox, znp)
