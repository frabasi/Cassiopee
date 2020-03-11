# Class for FastLBM prepare
import FastC.PyTree as FastC
import Converter.PyTree as C
import Generator.PyTree as G
import Transform.PyTree as T
import Converter.Internal as Internal
import Connector.PyTree as X
from Apps.Fast.Common import Common
import numpy 
try: range = xrange
except: pass

#================================================================================
# Multiblock prepare (avec split)
# NP is the target number of processors
#================================================================================ 
def prepare(t_case, t_out, tc_out, translation=[0.,0.,0.], NP=0, format='single', NG=1):
    import Converter.Mpi as Cmpi
    rank = Cmpi.rank; size = Cmpi.size
    ret = None
    # sequential prep
    if rank == 0: ret = prepare0(t_case, t_out, tc_out, translation, NP, format, NG=NG)
    return ret


def prepare0(t_case, t_out, tc_out, translation=[0.,0.,0.], NP=0, format='single',NG=1):
    if isinstance(t_case, str): t = C.convertFile2PyTree(t_case)
    else: t = t_case
    #
    TX = translation[0]; TY = translation[1]; TZ = translation[2]
    #
    C._addState(t, 'GoverningEquations', 'LBMLaminar')
    C._addState(t,adim='adim1', MInf=0.05)# ne sert qu a passer le warmup
    #
    if NP > 1 : t = T.splitSize(t, R=NP, type=2, minPtsPerDir=9)
    t = X.connectMatch(t)
    isPerio=0
    
    if TX != 0.:
        isPerio=1
        t = X.connectMatchPeriodic(t,translation=[TX,0,0])
    if TY != 0.:
        t = X.connectMatchPeriodic(t,translation=[0,TY,0])
        isPerio=1

    zonesNamesORIG=[]
    for z in Internal.getZones(t): zonesNamesORIG.append(z[0])

    # Creation des zones periodiques : periodicite par translation en x et y
    if isPerio:
        C._addPeriodicZones__(t[2][1])
        
        # Rajout des coins
        zonesDup=[]; zonesDupNames=[]
        for z in Internal.getZones(t[2][1]):
            isperiod = Internal.getNodeFromName1(z, 'TempPeriodicZone')    
            if isperiod is not None:
                C._rmBCOfType(z,'BCMatch')
                zonesDup.append(z)
                zonesDupNames.append(z[0])
        zcorners=[]
        for z in Internal.getZones(t):
            isperiod = Internal.getNodeFromName1(z, 'TempPeriodicZone')
            if isperiod is None:
                for j in [-1,1]:
                    for i in [-1,1]:                
                        zdup = T.translate(z,(i*TX,j*TY,0.))
                        C._rmBCOfType(zdup,"BCMatch")
                        zdupname = C.getZoneName(Internal.getName(zdup)+'_dup')
                        zdup[0] = zdupname
                        zonesDup.append(zdup)
                        #
                        zonesDup = X.connectMatch(zonesDup)
                        zdup=zonesDup[-1]      
                        gc = Internal.getNodeFromType(zdup,'GridConnectivity1to1_t')
                        if gc is not None:
                            zdonorname = Internal.getValue(gc)
                            Internal.createChild(zdup,'TempPeriodicZone','UserDefinedData_t',\
                                                 value=zdonorname,children=None)
                            zcorners.append(zdup)
                        del zonesDup[-1]
        del zonesDup                
        t[2][1][2]+= zcorners
        del zcorners
        
        dictOfTempPerNodes={}
        for z in Internal.getZones(t):
            pernode = Internal.getNodeFromName1(z,'TempPeriodicZone')
            if pernode is not None:
                dictOfTempPerNodes[z[0]]=pernode
        Internal._rmNodesFromName2(t[2][1],'TempPeriodicZone')
    
    # duplication par periodicite en Z
    if isPerio: C._rmBCOfType(t,"BCMatch")
    #en fait, il faudrait enlever la notion de periodicite, noeud GridConnectivityProperty
    # pour ne pas redupliquer des zones
    if TZ !=0.:
        isPerio = 1
        t = X.connectMatchPeriodic(t,translation=[0.,0.,TZ])
        C._addPeriodicZones__(t[2][1])
        for zn in dictOfTempPerNodes:
            TempNode = dictOfTempPerNodes[zn]
            z = Internal.getNodeFromName2(t,zn)
            z[2].append(TempNode)
        C._rmBCOfType(t,"BCMatch")

    t = X.connectMatch(t)
    Internal._addGhostCells(t,t,NG)
    C._rmBCOfType(t,"BCMatch")
    C._fillEmptyBCWith(t,'overlap','BCOverlap',dim=3)
    X._applyBCOverlaps(t, depth=1, loc='centers')
    tc = C.node2Center(t)
    #
    if isPerio: C._removeDuplicatedPeriodicZones__(t)
    #
    tc = X.setInterpData(t,tc,nature=1, loc='centers', storage='inverse', 
                         sameName=1,itype='chimera')
  
    # on remet chez la zone initiale les donnees d interp
    if isPerio:
        for z in Internal.getZones(tc):
            if z[0] not in zonesNamesORIG:        
                parentz,noz = Internal.getParentOfNode(tc,z)
                zdnames = z[0].split('_')
                znameorig = zdnames[0]
                zorig = Internal.getNodeFromNameAndType(tc,znameorig,'Zone_t')
                IDS = Internal.getNodesFromType(z,'ZoneSubRegion_t')
                for ID in IDS:
                    ID_orig = Internal.getNodeFromNameAndType(zorig,ID[0],'ZoneSubRegion_t')
                    if ID_orig is None:
                        zorig[2].append(ID)
                    else:
                        PL_NEW  = Internal.getNodeFromName(ID,'PointList')
                        PL_ORIG = Internal.getNodeFromName(ID_orig,'PointList')
                        PL_ORIG[1] = numpy.concatenate((PL_ORIG[1],PL_NEW[1]))

                        PLD_NEW = Internal.getNodeFromName(ID,'PointListDonor')
                        PLD_ORIG = Internal.getNodeFromName(ID_orig,'PointListDonor')
                        PLD_ORIG[1] = numpy.concatenate((PLD_ORIG[1],PLD_NEW[1]))

                        COEF_NEW = Internal.getNodeFromName(ID,'InterpolantsDonor')
                        COEF_ORIG = Internal.getNodeFromName(ID_orig,'InterpolantsDonor')
                        COEF_ORIG[1] = numpy.concatenate((COEF_ORIG[1],COEF_NEW[1]))

                        TYPE_NEW = Internal.getNodeFromName(ID,'InterpolantsType')
                        TYPE_ORIG = Internal.getNodeFromName(ID_orig,'InterpolantsType')
                        TYPE_ORIG[1] = numpy.concatenate((TYPE_ORIG[1],TYPE_NEW[1]))

                del parentz[2][noz]

    C._rmBCOfType(t,"BCOverlap")
    Internal._rmNodesFromType(tc,"GridCoordinates_t")

    if isinstance(tc_out, str):
        C.convertPyTree2File(tc, tc_out)
    if isinstance(t_out, str):
        C.convertPyTree2File(t, t_out)
    return t, tc

#====================================================================================
class LBM(Common):
    """Preparation et caculs avec le module FastLBM."""
    def __init__(self, format=None, numb=None, numz=None):
        Common.__init__(self, format, numb, numz)
        self.__version__ = "0.0"
        self.authors = ["stephanie.peron@onera.fr"]
        self.cartesian = True
        
    # Prepare 
    def prepare(self, t_case, t_out=None, tc_out=None, NP=0, translation=[0.,0.,0.],NG=1):
        #if NP is None: NP = Cmpi.size
        #if NP == 0: print('Preparing for a sequential computation.')
        #else: print('Preparing for a computation on %d processors.'%NP)
        ret = prepare(t_case, t_out, tc_out, translation=translation, NP=NP, format=self.data['format'],NG=NG)
        return ret
