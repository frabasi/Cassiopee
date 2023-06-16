"""Connector for IBM preprocessing"""
import numpy
from . import PyTree as X
from . import OversetData as XOD
from . import Connector
from . import connector

import Converter.PyTree as C
import Converter.Mpi as Cmpi
import Connector.Mpi as Xmpi
import Post.Mpi as Pmpi
import Generator.PyTree as G
import Initiator.PyTree as I
import Geom.PyTree as D
import Converter.Distributed as Distributed
import Generator.IBMmodelHeight as G_IBM_Height
import Transform.PyTree as T
import Converter.Internal as Internal
import Compressor.PyTree as Compressor
import Dist2Walls.PyTree as DTW
import Distributor2.PyTree as D2
import Post.PyTree as P
import Converter
import Generator
import Transform
import Converter.GhostCells as CGC
import KCore
import numpy
import math

varsn       = ['gradxTurbulentDistance','gradyTurbulentDistance','gradzTurbulentDistance']
varsnDouble = ['gradxTurbulentDistanceDouble','gradyTurbulentDistanceDouble','gradzTurbulentDistanceDouble']
TOLDIST  = 1.e-14
SHIFTF   = 1.e-10
EPSCART  = 1.e-6
TOLCELLN = 0.01

##from param_solver.h
LBM_IBC_NUM        = 113

TypesOfIBC = XOD.TypesOfIBC

# computes the friction velocity
def _computeFrictionVelocity(a):
    for z in Internal.getZones(a):
        connector._computeFrictionVelocity(z,
                                           Internal.__GridCoordinates__,
                                           Internal.__FlowSolutionNodes__,
                                           Internal.__FlowSolutionCenters__)
    return None

RENAMEIBCNODES=False

__IBCNameServer__={}

def getIBCDName(proposedName):
    global __IBCNameServer__
    (ibcname,__IBCNameServer__)=C.getUniqueName(proposedName, __IBCNameServer__)
    return ibcname

####################################################################################
# BMJC ##
####################################################################################

# GLOBAL
def prepareIBMDataPara(t_case, t_out, tc_out, t_in=None, to=None, snears=0.01, dfar=10., dfarList=[],
             tbox=None, snearsf=None, yplus=100., Lref=1.,
             vmin=21, check=False, NP=0, interpDataType=0, order=2,
             frontType=1, extrusion=None, balancing=False, recomputeDist=False,
             distrib=True, expand=3, tinit=None, initWithBBox=-1., wallAdapt=None, dfarDir=0, 
             correctionMultiCorpsF42=False, blankingF42=False, twoFronts=False, redistribute=False, IBCType=1,
             height_in=-1.0, isFilamentOnly=False, isWireModel=False, cleanCellN=True):
    
    import Generator.IBM as G_IBM

    DEPTH = 2
    #===================
    # STEP 0 : INIT
    #===================
    if True:
        cartesian = True if extrusion is None else False
        
        if isinstance(t_case, str): tb = C.convertFile2PyTree(t_case)
        else: tb = Internal.copyTree(t_case)
        
        if t_in is not None:
            refState=Internal.getNodeFromName(tb,'ReferenceState')
            flowEqn =Internal.getNodeFromName(tb,'FlowEquationSet')
            for b in Internal.getBases(t_in):
                Internal.addChild(b, refState, pos=0)
                Internal.addChild(b, flowEqn , pos=0)

        filamentBases= []
        len_tb = len(Internal.getBases(tb))
        for b in Internal.getBases(tb):
            if "IBCFil" in b[0]: filamentBases.append(b[0])

        isFilamentOnly = False
        if len(filamentBases)==len_tb:           
            isFilamentOnly=True
        isOrthoProjectFirst = isFilamentOnly
        ## if tb has both a closed solid and filaments
        
        tbFilament = Internal.getBases(tb)
        if not isFilamentOnly:
            tbFilament = []
            for b in filamentBases:
                node_local = Internal.getNodeFromNameAndType(tb, b, 'CGNSBase_t')
                tbFilament.append(node_local)
                Internal._rmNode(tb,node_local)     
                isOrthoProjectFirst = True        
        
        tbFilament = C.newPyTree(tbFilament)  

        # reference state
        refstate = C.getState(tb)
        Reynolds = Internal.getNodeFromName(tb, 'Reynolds')
        if Reynolds is not None:
            Reynolds = Internal.getValue(Reynolds)
            if Reynolds < 1.e5: frontType = 1
        else:
            Reynolds = 1.e6
        
        # dimension du pb
        dimPb = Internal.getNodeFromName(tb, 'EquationDimension')
        dimPb = Internal.getValue(dimPb)
        
        model = Internal.getNodeFromName(tb, 'GoverningEquations')
        if model is None: raise ValueError('GoverningEquations is missing in input geometry tree.')
        model = Internal.getValue(model)    # model : Euler, NSLaminar, NSTurbulent
        
        # check Euler non consistant avec Musker
        if model == 'Euler':
            ibctype = Internal.getNodesFromName(tb, 'ibctype')
            if ibctype is not None:
                if 'Musker' in ibctype or 'Log' in ibctype:
                    raise ValueError("In tb: governing equations (Euler) not consistent with ibc type (%s)"%(ibctype))

        #Get the types of IBCs in tb
        ibctypes = set()
        for node in Internal.getNodesFromName(tb,'ibctype'):
            ibctypes.add(Internal.getValue(node))
        ibctypes = list(ibctypes)    

    #===================
    # STEP 1 : GENERATE MESH
    #===================
    if dimPb == 2 and cleanCellN == False: C._initVars(tb, 'CoordinateZ', 0.) # forced
    if t_in is None:
        t = G_IBM.generateIBMMeshPara(tb, vmin=vmin, snears=snears, dimPb=dimPb, dfar=dfar, dfarList=dfarList, tbox=tbox,
                    snearsf=snearsf, check=check, symmetry=0, to=to, ext=3,
                    expand=expand, dfarDir=dfarDir, check_snear=False)           

        C._addState(t, state=refstate)
        C._addState(t, 'GoverningEquations', model)
        C._addState(t, 'EquationDimension', dimPb)

    else: 
        t = t_in

    if balancing:
        import Distributor2.Mpi as D2mpi
        ts     = Cmpi.allgatherTree(Cmpi.convert2SkeletonTree(t))
        stats  = D2._distribute(ts, NP, algorithm='graph')
        D2._copyDistribution(t , ts)
        D2mpi._redispatch(t)
        del ts
            
    if extrusion == 'cyl':
        T._cart2Cyl(t, (0,0,0),(1,0,0))
        T._cart2Cyl(tb, (0,0,0),(1,0,0))

    #===================
    # STEP 2 : DIST2WALL
    #===================
    _dist2wallIBM(t, tb, tbFilament=tbFilament, extrusion=extrusion, cleanCellN=cleanCellN, dimPb=dimPb, dz=0.01, filamentBases=filamentBases, 
                correctionMultiCorpsF42=correctionMultiCorpsF42, frontType=frontType, yplus=yplus, Reynolds=Reynolds, Lref=Lref, height_in=height_in)

    #===================
    # STEP 3 : BLANKING IBM
    #===================
    _blankingIBM(t, tb, dimPb=dimPb, extrusion=extrusion, isFilamentOnly=isFilamentOnly, filamentBases=filamentBases, 
                    cleanCellN=cleanCellN, frontType=frontType, IBCType=IBCType, DEPTH=DEPTH, yplus=yplus, 
                    Reynolds=Reynolds, Lref=Lref, height_in=height_in, correctionMultiCorpsF42=correctionMultiCorpsF42, 
                    wallAdapt=wallAdapt, blankingF42=blankingF42, twoFronts=twoFronts)

    #===================
    # STEP 4 : INTERP DATA CHIM
    #===================
    C._initVars(t,'{centers:cellN}=maximum(0.,{centers:cellNChim})') # vaut -3, 0, 1, 2 initialement
    tc = C.node2Center(t)

    # ajouter argument cartesian dans setInterpData
    if Internal.getNodeFromType(t, "GridConnectivity1to1_t") is not None:
        Xmpi._setInterpData(t, tc, nature=1, loc='centers', storage='inverse', sameName=1, dim=3, itype='abutting', order=order)
    Xmpi._setInterpData(t, tc, nature=1, loc='centers', storage='inverse', sameName=1, dim=3, itype='chimera', order=order)
    
    #===================
    # STEP 4 : BUILD FRONT
    #===================
    t, tc, front, front2 = buildFrontIBM(t, tc, dimPb=dimPb, frontType=frontType, interpDataType=interpDataType, 
                                        cartesian=cartesian, twoFronts=twoFronts, check=check)

    #===================
    # STEP 5 : INTERP DATA IBM
    #===================
    _setInterpDataIBM(t, tc, tb, front, front2=front2, dimPb=dimPb, NP=NP, frontType=frontType, DEPTH=DEPTH, 
                        IBCType=IBCType, interpDataType=interpDataType, Reynolds=Reynolds, yplus=yplus, Lref=Lref, 
                        twoFronts=twoFronts, isOrthoProjectFirst=isOrthoProjectFirst, isWireModel=isWireModel, cartesian=cartesian)

    #===================
    # STEP 6 : DIST2WALL FOR VISCOUS WALL
    #===================
    _recomputeDistForViscousWall__(t, tb, tbFilament=tbFilament, model=model, dimPb=dimPb, dz=0.01, filamentBases=filamentBases, 
                                extrusion=extrusion, recomputeDist=recomputeDist, cleanCellN=cleanCellN, ibctypes=ibctypes)
    
    #===================
    # STEP 7 : DISTRIBUTE
    #===================
    if distrib:
        if NP == 0: NP = Cmpi.size
        tbbc = Cmpi.createBBoxTree(tc)
        stats = D2._distribute(tbbc, NP, algorithm='graph', useCom='ID')
        D2._copyDistribution(tc, tbbc)
        D2._copyDistribution(t, tbbc)
        tbbc = None
        
    if redistribute:
        import Distributor2.Mpi as D2mpi
        tcs    = Cmpi.allgatherTree(Cmpi.convert2SkeletonTree(tc))
        stats  = D2._distribute(tcs, NP, algorithm='graph')
        D2._copyDistribution(tc, tcs)
        D2._copyDistribution(t , tcs)
        D2mpi._redispatch(tc)
        D2mpi._redispatch(t)

    #===================
    # STEP 8 : INIT TC AND SAVE
    #===================
    if check:
        tibm = extractIBMInfo(tc, IBCNames="IBCD_*")
        Cmpi.convertPyTree2File(tibm, 'IBMInfo.cgns')
        del tibm

        if twoFronts or isWireModel:
            tibm2 = extractIBMInfo(tc, IBCNames="2_IBCD_*")
            Cmpi.convertPyTree2File(tibm2, 'IBMInfo2.cgns')
            del tibm2

    if twoFronts or isWireModel: 
        tc2 = Internal.copyTree(tc)
    else: 
        tc2 = None
    _tcInitialize__(tc, tc2=tc2, twoFronts=twoFronts, isWireModel=isWireModel, ibctypes=ibctypes)

    if isinstance(tc_out, str):
        tcp = Compressor.compressCartesian(tc)
        Cmpi.convertPyTree2File(tcp, tc_out, ignoreProcNodes=True)
        
        if twoFronts:
            tcp2 = Compressor.compressCartesian(tc2)
            Cmpi.convertPyTree2File(tcp2, 'tc2.cgns', ignoreProcNodes=True)
            del tc2

    #===================
    # STEP 9 : INIT T AND SAVE
    #===================
    _tInitialize__(t, tb, tinit=tinit, model=model, isWireModel=isWireModel, initWithBBox=initWithBBox)

    if extrusion is not None:
        vars = ['centers:TurbulentDistanceAllBC','centers:TurbulentDistanceWallBC', 'centers:cellNIBC_hole']
        C._rmVars(t, vars)
        
    if isinstance(t_out, str):
        tp = Compressor.compressCartesian(t)
        Cmpi.convertPyTree2File(tp, t_out, ignoreProcNodes=True)
        
    if Cmpi.size > 1: Cmpi.barrier()

    return t,tc

# DIST2WALL ##############################
def _recomputeDistForViscousWall__(t, tb, tbFilament=None, model='NSTurbulent', dimPb=3, dz=0.01, 
                            filamentBases=None, extrusion=None, recomputeDist=False, cleanCellN=False, ibctypes=[]):
    if model != 'Euler' and recomputeDist and (extrusion!='cyl' and extrusion !='cart'):
        if 'outpress' in ibctypes or 'inj' in ibctypes or 'slip' in ibctypes:
            for z in Internal.getZones(tb):
                ibc = Internal.getNodeFromName(z,'ibctype')
                if Internal.getValue(ibc)=='outpress' or Internal.getValue(ibc)=='inj' or Internal.getValue(ibc)=='slip':
                    Internal._rmNode(tb,z)
            if dimPb == 2:
                tb2 = C.initVars(tb, 'CoordinateZ', dz*0.5)
                DTW._distance2Walls(t,tb2,type='ortho', signed=0, dim=dimPb, loc='centers')
                if filamentBases:
                    tb2 = C.initVars(tbFilament, 'CoordinateZ', dz*0.5)
                    DTW._distance2Walls(t,tb2,type='ortho', signed=0, dim=dimPb, loc='centers')
                DTW._distance2Walls(t,tb2,type='ortho', signed=0, dim=dimPb, loc='centers')
            else:
                DTW._distance2Walls(t,tb,type='ortho', signed=0, dim=dimPb, loc='centers')
    
            if dimPb == 2 and cleanCellN == False: C._initVars(t, '{centers:TurbulentDistanceWallBC}={centers:TurbulentDistance}')
    else:
        for z in Internal.getZones(t):
            dist = Internal.getNodeFromName2(z,'TurbulentDistanceWallBC')
            if dist is not None:  C._initVars(t, '{centers:TurbulentDistance}={centers:TurbulentDistanceWallBC}')

    return None

def _dist2wallNearBody__(t, tb, type='ortho', signed=0, dim=3, loc='centers'):
    DTW._distance2Walls(t, tb, type=type, signed=signed, dim=dim, loc=loc)
    #list_final_zones=[]
    #for z in Internal.getZones(t):
    #    list_final_zones.append(z[0])
    #
    #tBB =G.BB(t)
    #tbBB=G.BB(tb)
    #
    #interDict = X.getIntersectingDomains(tBB, tbBB)    
    #
    ##FULL TB
    #zt       = []
    #zt_names = []
    #for i in interDict:
    #    if interDict[i]:
    #        zt.append(Internal.getNodeByName(t,i))
    #        zt_names.append(i)
    #
    #if zt_names:
    #    DTW._distance2Walls(zt, tb, type=type, signed=signed, dim=dim, loc=loc)
    #
    ###PRT1
    #list_additional_zones = getZonesScaleUpDown__(tbBB,tBB,zt_names,dim=dim)
    #
    ####PRT2
    #if list_additional_zones:        
    #    zt=[]
    #    for i in list_additional_zones:
    #        zt.append(Internal.getNodeByName(t,i))
    #    
    #    DTW._distance2Walls(zt, tb, type=type, signed=signed, dim=dim, loc=loc)
    return None

#=========================================================================
# Calcul de la distance a paroi pour les IBMs
# IN: t: arbre du calcul
# IN: tb: arbre du body IBM
# OUT: centers:TurbulentDistance field
#=========================================================================
def _dist2wallIBM(t, tb, dimPb=3, 
                  correctionMultiCorpsF42=False, frontType=1, yplus=100, 
                  Reynolds=1.e6, Lref=1., height_in=1.,
                  tbFilament=None, filamentBases=None):
    
    if dimPb == 2:
        # Set CoordinateZ to a fixed value
        dz = 0.01
        tb2 = C.initVars(tb, 'CoordinateZ', dz*0.5)
        if tbFilament is not None: tbFilament2 = C.initVars(tbFilament, 'CoordinateZ', dz*0.5)
        else: tbFilament2 = None
    else:
        tb2 = tb; tbFilament2 = tbFilament

    # Compute distance to bodies
    DTW._distance2Walls(t, tb2, type='ortho', signed=0, dim=dimPb, loc='centers')

    if filamentBases is not None:
        C._initVars(t,'{centers:TurbulentDistanceSolid}={centers:TurbulentDistance}')
        C._initVars(t,'{centers:TurbulentDistance}=1e06')
                
        DTW._distance2Walls(t, tbFilament2, type='ortho', signed=0, dim=dimPb, loc='centers')

        C._initVars(t,'{centers:TurbulentDistanceFilament}={centers:TurbulentDistance}')
        C._initVars(t,'{centers:TurbulentDistance}=minimum({centers:TurbulentDistanceSolid},{centers:TurbulentDistanceFilament})')
        #C._initVars(t,"{centers:TurbulentDistanceSolid}=({centers:TurbulentDistanceSolid}>1e03)*0+({centers:TurbulentDistanceSolid}<1e03)*{centers:TurbulentDistanceSolid}")
        #C._initVars(t,"{centers:TurbulentDistanceFilament}=({centers:TurbulentDistanceFilament}>1e03)*0+({centers:TurbulentDistanceFilament}<1e03)*{centers:TurbulentDistanceFilament}")
    
    #C._initVars(t,"{centers:TurbulentDistance}=({centers:TurbulentDistance}>1e03)*0+({centers:TurbulentDistance}<1e03)*{centers:TurbulentDistance}")

    # Compute turbulentdistance wrt each body that is not a sym plan (centers:TurbulentDistance_bodyX)
    if correctionMultiCorpsF42 and frontType == 42:
        # Keep track of the general turbulentDistance
        C._initVars(t,'{centers:TurbulentDistance_ori}={centers:TurbulentDistance}')

        if yplus > 0.:
            shiftDist = G_IBM_Height.computeModelisationHeight(Re=Reynolds, yplus=yplus, L=Lref)
        else:
            snears    = Internal.getNodesFromName(tb, 'snear')
            h         = max(snears, key=lambda x: x[1])[1]
            shiftDist = G_IBM_Height.computeBestModelisationHeight(Re=Reynolds, h=h) # meilleur compromis entre hauteur entre le snear et la hauteur de modelisation

        if height_in > 0.:
            if shiftDist > height_in: shiftDist = height_in
                
        for z in Internal.getZones(t):
            cptBody = 1
            if dimPb == 3: tb2 = tb
            for body in Internal.getNodesFromType(tb2, 'Zone_t'):
                if body[0] != "sym" and ("closure" not in body[0]):
                    # Create extanded BBox around each body
                    bboxBody = G.BB(body)
                    coordX = Internal.getNodeFromName(bboxBody, 'CoordinateX')[1]
                    coordX[0] = coordX[0] - shiftDist
                    coordX[1] = coordX[1] + shiftDist
                    Internal.getNodeFromName(bboxBody, 'CoordinateX')[1] = coordX
                    coordY = Internal.getNodeFromName(bboxBody, 'CoordinateY')[1]
                    coordY[0][0] = coordY[0][0] - shiftDist
                    coordY[1][0] = coordY[1][0] - shiftDist
                    coordY[0][1] = coordY[0][1] + shiftDist
                    coordY[1][1] = coordY[1][1] + shiftDist
                    Internal.getNodeFromName(bboxBody, 'CoordinateY')[1] = coordY
                    coordZ = Internal.getNodeFromName(bboxBody, 'CoordinateZ')[1] 
                    coordZ[0][0][0] = coordZ[0][0][0] - shiftDist
                    coordZ[0][1][0] = coordZ[0][1][0] - shiftDist
                    coordZ[1][0][0] = coordZ[1][0][0] - shiftDist
                    coordZ[1][1][0] = coordZ[1][1][0] - shiftDist
                    coordZ[0][0][1] = coordZ[0][0][1] + shiftDist
                    coordZ[0][1][1] = coordZ[0][1][1] + shiftDist
                    coordZ[1][0][1] = coordZ[1][0][1] + shiftDist
                    coordZ[1][1][1] = coordZ[1][1][1] + shiftDist
                    Internal.getNodeFromName(bboxBody, 'CoordinateZ')[1] = coordZ
                    bboxZone = G.BB(z)

                    # Compute new individual turbulentDistance when blocks are close enough
                    if G.bboxIntersection(bboxBody, bboxZone, isBB=True):
                        DTW._distance2Walls(z, body, type='ortho', signed=0, dim=dimPb, loc='centers')
                        C._initVars(z,'{centers:TurbulentDistance_body%i={centers:TurbulentDistance}'%cptBody)
                    else:
                        C._initVars(z,'{centers:TurbulentDistance_body%i=1000'%cptBody)
                    cptBody += 1
            if dimPb == 3: del tb2

        C._initVars(t, '{centers:TurbulentDistance}={centers:TurbulentDistance_ori}')
        C._rmVars(t, ['centers:TurbulentDistance_ori'])

    return None

# BLANKING IBM  ##########################
def _blankingIBM__(t, tb, dimPb=3, extrusion=False, isFilamentOnly=False, filamentBases=[], cleanCellN=True, frontType=1, 
                    IBCType=1, DEPTH=2, yplus=100, Reynolds=1.e6, Lref=1., height_in=1., correctionMultiCorpsF42=False, 
                    wallAdapt=None, blankingF42=False):
    
    snear_min = 10e10
    for z in Internal.getZones(tb):
        sdd = Internal.getNodeFromName1(z, ".Solver#define")
        if sdd is not None:
            snearl = Internal.getNodeFromName1(sdd, "snear")
            if snearl is not None:
                snearl = Internal.getValue(snearl)
        print(snearl)
        if snearl is not None:  snear_min = min(snear_min,snearl)
    snear_min = Cmpi.allreduce(snear_min, op=Cmpi.MIN)
    
    if extrusion is None:
        if not isFilamentOnly:
            t = blankByIBCBodies(t, tb, 'centers', dimPb)
        if dimPb == 2 and cleanCellN == False:
            C._initVars(t, '{centers:cellNIBC_blank}={centers:cellN}')
    else:
        C._initVars(t, '{centers:cellN}={centers:cellNIBC_blank}')

    C._initVars(t, '{centers:cellNIBC}={centers:cellN}')
        
    if not isFilamentOnly: _signDistance(t)

    if extrusion is not None:
        C._initVars(t,'{centers:cellN}={centers:cellNIBC_blank}')
    else:
        C._initVars(t,'{centers:cellN}={centers:cellNIBC}')
    
    if filamentBases or isFilamentOnly:
        if isFilamentOnly:
            C._initVars(t,'{centers:TurbulentDistanceFilament}={centers:TurbulentDistance}')
            maxy = C.getMaxValue(tb, ['CoordinateY'])
            miny = C.getMinValue(tb, ['CoordinateY'])
        if filamentBases:
            maxy = C.getMaxValue(filamentBases, ['CoordinateY'])
            miny = C.getMinValue(filamentBases, ['CoordinateY'])
        for z in Internal.getZones(t):
            if C.getMaxValue(z, 'centers:TurbulentDistanceFilament')> 1e-05:
                sol   = Internal.getNodeByName(z,"FlowSolution#Centers")
                cellN = Internal.getNodeByName(sol,'cellN')[1]
                dist  = Internal.getNodeByName(sol,'TurbulentDistanceFilament')[1]
                ycord = Internal.getNodeByName(z,'CoordinateY')[1]
                h     = abs(C.getValue(z,'CoordinateX',4)-C.getValue(z,'CoordinateX',5))
                sh    = numpy.shape(dist)
                for k in range(sh[2]):
                    for j in range(sh[1]):
                        for i in range(sh[0]):
                            valy = 0.5*(ycord[i,j,k]+ycord[i,j+1,k])
                            if dist[i,j] < numpy.sqrt(8.)*h and valy<maxy and valy>miny:
                                cellN[i,j] = 2
        C._rmVars(t,['centers:TurbulentDistanceFilament'])
        if filamentBases: C._rmVars(t,['centers:TurbulentDistanceSolid'])

    # determination des pts IBC
    if extrusion is None:
        if frontType != 42:
            if IBCType == -1: X._setHoleInterpolatedPoints(t,depth=-DEPTH,dir=0,loc='centers',cellNName='cellN',addGC=False)
            elif IBCType == 1:
                X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellN',addGC=False) # pour les gradients
                if frontType < 2:
                    X._setHoleInterpolatedPoints(t,depth=DEPTH,dir=0,loc='centers',cellNName='cellN',addGC=False)
                else:
                    DEPTHL = DEPTH+1
                    X._setHoleInterpolatedPoints(t,depth=DEPTHL,dir=0,loc='centers',cellNName='cellN',addGC=False)
                    #cree des pts extrapoles supplementaires
                    # _blankClosestTargetCells(t,cellNName='cellN', depth=DEPTHL)
            else:
                raise ValueError('prepareIBMData: not valid IBCType. Check model.')
            
        else:
            # F42: tracking of IB points using distance information
            # the classical algorithm (front 1) is first used to ensure a minimum of two rows of target points around the geometry
            C._initVars(t,'{centers:cellNMin}={centers:cellNIBC}')
            if IBCType == -1: X._setHoleInterpolatedPoints(t,depth=-DEPTH,dir=0,loc='centers',cellNName='cellNMin',addGC=False)
            elif IBCType == 1: X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellNMin',addGC=False) # pour les gradients
            X._setHoleInterpolatedPoints(t,depth=DEPTH,dir=0,loc='centers',cellNName='cellNMin',addGC=False)

            for z in Internal.getZones(t):
                h = abs(C.getValue(z,'CoordinateX',0)-C.getValue(z,'CoordinateX',1))
                if yplus > 0.:
                    height = G_IBM_Height.computeModelisationHeight(Re=Reynolds, yplus=yplus, L=Lref)
                else:
                    height = G_IBM_Height.computeBestModelisationHeight(Re=Reynolds, h=h) # meilleur compromis entre hauteur entre le snear et la hauteur de modelisation
                    yplus  = G_IBM_Height.computeYplus(Re=Reynolds, height=height, L=Lref)
                if height_in > 0.:
                    if height > height_in:
                        height = height_in
                        #print("Snear min (SM) = %g || Wall Modeling Height (WMH) = %g || WMH/SM = %g"%(snear_min,height,height/snear_min))
                C._initVars(z,'{centers:cellN}=({centers:TurbulentDistance}>%20.16g)+(2*({centers:TurbulentDistance}<=%20.16g)*({centers:TurbulentDistance}>0))'%(height,height))

                if correctionMultiCorpsF42:
                    # Prevent different body modeling from overlapping -> good projection of image points in the wall normal direction

                    cptBody = 1
                    for body in Internal.getNodesFromType(tb,'Zone_t'):
                        if body[0] != "sym" and ("closure" not in body[0]):
                            cptBody += 1

                    epsilon_dist = 2*(abs(C.getValue(z,'CoordinateX',1)-C.getValue(z,'CoordinateX',0)))
                    max_dist     = 2*0.1*Lref

                    # Try to find the best route between two adjacent bodies by finding optimal iso distances
                    def correctionMultiCorps(cellN, cellNF):
                        if cellN == 2 and cellNF == 2: return 1
                        return cellN

                    def findIsoFront(cellNFront, Dist_1, Dist_2):
                        if Dist_1 < max_dist and Dist_2 < max_dist:
                            if abs(Dist_1-Dist_2) < epsilon_dist: return 2
                        return max(cellNFront,1)

                    for i in range(1, cptBody):
                        for j in range(1, cptBody):
                            if j != i:
                                C._initVars(z,'centers:cellNFrontIso', findIsoFront, ['centers:cellNFrontIso', 'centers:TurbulentDistance_body%i'%i, 'centers:TurbulentDistance_body%i'%j])

                    C._initVars(z,'centers:cellN', correctionMultiCorps, ['centers:cellN', 'centers:cellNFrontIso'])

                    for i in range(1, cptBody):
                        C._rmVars(z,['centers:cellN_body%i'%i, 'centers:TurbulentDistance_body%i'%i])

            if wallAdapt is not None:
                # Use previous computation to adapt the positioning of IB points around the geometry (impose y+PC <= y+ref)
                # Warning: the wallAdapt file has to be obtained with TIBM.createWallAdapt(tc)
                C._initVars(t,'{centers:yplus}=100000.')
                w = C.convertFile2PyTree(wallAdapt)
                total = len(Internal.getZones(t))
                cpt = 1
                for z in Internal.getZones(t):
                    print("{} / {}".format(cpt, total))
                    cellN = Internal.getNodeFromName(z,'cellN')[1]
                    if 2 in cellN:
                        zname = z[0]
                        zd = Internal.getNodeFromName(w, zname)
                        if zd is not None:
                            yplus_w = Internal.getNodeFromName(zd, 'yplus')[1]
                            listIndices = Internal.getNodeFromName(zd, 'PointListDonor')[1]
                        
                            n = numpy.shape(yplus_w)[0]
                            yplusA = Converter.array('yplus', n, 1, 1)
                            yplusA[1][:] = yplus_w

                            C._setPartialFields(z, [yplusA], [listIndices], loc='centers')

                    cpt += 1

                C.convertPyTree2File(t, 'toto.cgns')
                
                C._initVars(t,'{centers:cellN}=({centers:cellN}>0) * ( (({centers:cellN}) * ({centers:yplus}<=%20.16g)) + ({centers:yplus}>%20.16g) )'%(yplus,yplus))
            
            # final security gate, we ensure that we have at least to layers of target points
            C._initVars(t, '{centers:cellN} = maximum({centers:cellN}, {centers:cellNMin})')
            C._rmVars(t,['centers:yplus', 'centers:cellNMin'])

            # propagate max yplus between procs
            yplus = numpy.array([float(yplus)])
            yplus = Cmpi.allreduce(yplus, op=Cmpi.MAX)[0]

            # Only keep the layer of target points useful for solver iterations, particularly useful in 3D
            if blankingF42: X._maximizeBlankedCells(t, depth=2, addGC=False)

        if dimPb == 2 and cleanCellN == False: C._initVars(t, '{centers:cellNIBC_hole}={centers:cellN}')

    else:  # extrusion
        C._initVars(t, '{centers:cellN}={centers:cellNIBC_hole}')

    if extrusion is None:
        if not isFilamentOnly:_removeBlankedGrids(t, loc='centers')
            
    return None

def _blankingIBM(t, tb, dimPb=3, extrusion=False, isFilamentOnly=False, filamentBases=[], cleanCellN=True, frontType=1, IBCType=1, 
                DEPTH=2, yplus=100, Reynolds=1.e6, Lref=1., height_in=1., correctionMultiCorpsF42=False, 
                wallAdapt=None, blankingF42=False, twoFronts=False):
   
    if dimPb == 2:
        T._addkplane(tb)
        T._contract(tb, (0,0,0), (1,0,0), (0,1,0), 0.01)
    
    X._applyBCOverlaps(t, depth=DEPTH, loc='centers', val=2, cellNName='cellN')
    C._initVars(t,'{centers:cellNChim}={centers:cellN}')
    C._initVars(t, 'centers:cellN', 1.)
    
    _blankingIBM__(t, tb, dimPb=dimPb, extrusion=extrusion, isFilamentOnly=isFilamentOnly, filamentBases=filamentBases, 
                    cleanCellN=cleanCellN, frontType=frontType, IBCType=IBCType, DEPTH=DEPTH, yplus=yplus, Reynolds=Reynolds, 
                    Lref=Lref, height_in=height_in, correctionMultiCorpsF42=correctionMultiCorpsF42, wallAdapt=wallAdapt, blankingF42=blankingF42)

    C._initVars(t, '{centers:cellNIBC}={centers:cellN}')
    
    if IBCType==-1:
        C._initVars(t,'{centers:cellNDummy}=({centers:cellNIBC}>0.5)*({centers:cellNIBC}<1.5)')
        X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellNDummy',addGC=False)
        C._initVars(t,'{centers:cellNFront}=logical_and({centers:cellNDummy}>0.5, {centers:cellNDummy}<1.5)')
        C._rmVars(t, ['centers:cellNDummy'])
        for z in Internal.getZones(t):
            connector._updateNatureForIBM(z, IBCType,
                                            Internal.__GridCoordinates__,
                                            Internal.__FlowSolutionNodes__,
                                            Internal.__FlowSolutionCenters__)
    else:
        C._initVars(t,'{centers:cellNFront}=logical_and({centers:cellNIBC}>0.5, {centers:cellNIBC}<1.5)')
        
        for z in Internal.getZones(t):
            if twoFronts:
                epsilon_dist = abs(C.getValue(z,'CoordinateX',1)-C.getValue(z,'CoordinateX',0))
                dmin = math.sqrt(3)*4*epsilon_dist
                if frontType == 42:
                    SHIFTB = G_IBM_Height.computeModelisationHeight(Re=Reynolds, yplus=yplus, L=Lref)
                    dmin = max(dmin, SHIFTB+math.sqrt(3)*2*epsilon_dist) # where shiftb = hmod
                C._initVars(z,'{centers:cellNIBC_2}=({centers:TurbulentDistance}>%20.16g)+(2*({centers:TurbulentDistance}<=%20.16g)*({centers:TurbulentDistance}>0))'%(dmin,dmin))
                C._initVars(z,'{centers:cellNFront_2}=logical_and({centers:cellNIBC_2}>0.5, {centers:cellNIBC_2}<1.5)')
            
            connector._updateNatureForIBM(z, IBCType,
                                            Internal.__GridCoordinates__,
                                            Internal.__FlowSolutionNodes__,
                                            Internal.__FlowSolutionCenters__)

    ##Ghost kmin et kmax donneuse potentiel
    if extrusion is not None:
        listvars_local = ['cellNChim','cellNIBC']
        for z in Internal.getZones(t):
            sol = Internal.getNodeFromName(z,'FlowSolution#Centers')
            for var in listvars_local:
                cellN = Internal.getNodeFromName(sol,var)[1]
                sh = numpy.shape(cellN)
                for k in [0,1, sh[2]-2, sh[2]-1]:
                    for j in range(sh[1]):
                        for i in range(sh[0]):
                            if cellN[i,j,k] != 0:  cellN[i,j,k] =1

    return None

# BUILD FRONT  ###########################
def _pushBackImageFront2__(t, tc, tbbc, interpDataType=0, cartesian=True):    
    # bboxDict needed for optimised AddXZones (i.e. "layers" not None)
    # Return a dict with the zones of t as keys and their specific bboxes as key values
    bboxDict  = Cmpi.createBboxDict(t)
    interDict = X.getIntersectingDomains(tbbc)
    graph     = Cmpi.computeGraph(tbbc, type='bbox', intersectionsDict=interDict, reduction=False)
    
    # if subr, the tree subregions are kept during the exchange
    # if layers not None, only communicate the desired number of layers
    Cmpi._addLXZones(tc, graph, variables=['cellNIBC','cellNChim','cellNFront'], cartesian=cartesian,
                        interDict=interDict,bboxDict=bboxDict, layers=4, subr=False)
    Cmpi._addLXZones(t, graph, variables=['centers:cellNIBC', 'centers:cellNChim', 'centers:cellNFront'], cartesian=cartesian,
                        interDict=interDict, bboxDict=bboxDict, layers=4, subr=False)
    
    # Zones of tc are modified after addXZones, new tbbc, interDict and intersectionDict
    tbbcx             = G.BB(tc)
    interDict         = X.getIntersectingDomains(tbbcx)
    intersectionsDict = X.getIntersectingDomains(tbbcx, method='AABB', taabb=tbbcx)

    # Reconstruction of cellNFront and cellN from cellNIBC (reduce the communications)
    # cellNFront_origin and cellNIBC_origin are initialised to store the Data of cellNFront and cellNIBC before the transfers
    C._initVars(t, '{centers:cellN}={centers:cellNIBC}')
    C._initVars(t, '{centers:cellNFront_origin}={centers:cellNFront}')
    C._initVars(t, '{centers:cellNIBC_origin}={centers:cellNIBC}')
    C._initVars(t, '{centers:cellN_interp}=maximum(0.,{centers:cellNChim})') # Second way of building the cellN field, see above

    C._cpVars(t,'centers:cellNFront',tc,'cellNFront')
    C._cpVars(t,'centers:cellNIBC',tc,'cellNIBC')
    C._cpVars(t,'centers:cellN',tc,'cellN')
    C._cpVars(t,'centers:cellN_interp',tc,'cellN_interp')
    C._cpVars(t,'centers:cellNFront_origin',tc,'cellNFront_origin')
    C._cpVars(t,'centers:cellNIBC_origin',tc,'cellNIBC_origin')

    # Find each zone that require the specific treatment
    C._initVars(t,'{centers:cellNFront2}=1.-({centers:cellNFront}<1.)*(abs({centers:cellNChim})>1.)')
    # i.e. if cellNFront_origin == 2 and cellNFront == 1 ou -3 => cellNFront2 = 1

    # Transfers the information at each grid connection
    for z in Internal.getZones(t):
        cellNFront = Internal.getNodeFromName2(z,'cellNFront2')
        if cellNFront != []:
            cellNFront = cellNFront[1]
            sizeTot = cellNFront.shape[0]*cellNFront.shape[1]*cellNFront.shape[2]
            sizeOne =  int(numpy.sum(cellNFront))
            if sizeOne < sizeTot:
                X._setHoleInterpolatedPoints(z, depth=1, dir=0, loc='centers', cellNName='cellNFront2', addGC=False)
                res = X.getInterpolatedPoints(z, loc='centers', cellNName='cellNFront2') # indices,X,Y,Z
                if res is not None:
                    indicesI = res[0]
                    XI = res[1]; YI = res[2]; ZI = res[3]
                    allInterpFields=[]
                    for zc in Internal.getZones(tc):
                        if zc[0] in intersectionsDict[z[0]]:
                            C._cpVars(zc,'cellN_interp',zc,'cellN')
                            fields = X.transferFields(zc, XI, YI, ZI, hook=None, variables=['cellNFront_origin','cellNIBC_origin'],
                                                        interpDataType=interpDataType, nature=1)
                            allInterpFields.append(fields)
                    if allInterpFields!=[]:
                        C._filterPartialFields(z, allInterpFields, indicesI, loc='centers', startFrom=0, filterName='donorVol',
                                                verbose=False)

    Cmpi._rmXZones(tc)
    Cmpi._rmXZones(t)

    # Update the cellNFront, cellNIBC and cellNIBCDnr fields
    for z in Internal.getZones(t):
        cellNFront = Internal.getNodeFromName2(z,'cellNFront2')
        if cellNFront != []:
            cellNFront = cellNFront[1]
            sizeTot = cellNFront.shape[0]*cellNFront.shape[1]*cellNFront.shape[2]
            sizeOne =  int(numpy.sum(cellNFront))
            if sizeOne < sizeTot:
                # Modification du Front uniquement lorsque celui-ci est repousse
                C._initVars(z,'{centers:cellNFront}={centers:cellNFront}*({centers:cellNFront_origin}>0.5)') 
                # i.e. if cellNFront_origin == 0 and cellNFront == 1 => cellNfront = 0

                C._initVars(z,'{centers:cellNIBC}={centers:cellNIBC}*(1.-({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)) \
                    + 2.*({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)')
                # i.e. if cellNChim == 1 and cellNIBC_origin == 2 => cellNIBC = 2

                C._initVars(z,'{centers:cellNIBCDnr}={centers:cellNIBCDnr}*(1.-({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)) \
                    + 2.*({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)')

    C._cpVars(t,'centers:cellNIBC',tc,'cellNIBC')
    C._cpVars(t,'centers:cellNIBC',t,'centers:cellN')
    C._cpVars(t,'centers:cellN',tc,'cellN')

    C._rmVars(t,['centers:cellNFront2'])
    C._rmVars(t,['centers:cellNFront_origin'])
    C._rmVars(t,['centers:cellNIBC_origin'])
    C._rmVars(t,['centers:cellN_interp'])

    C._cpVars(t,'centers:cellNFront',tc,'cellNFront')

    Xmpi._setInterpTransfers(t,tc,variables=['cellNFront'], cellNVariable='cellNFront', compact=0)

    return None

def buildFrontIBM(t, tc, dimPb=3, frontType=1, interpDataType=0, cartesian=False, twoFronts=False, check=False):

    tbbc = Cmpi.createBBoxTree(tc)
    
    C._initVars(t,'{centers:cellNIBCDnr}=minimum(2.,abs({centers:cellNIBC}))')
    C._initVars(t,'{centers:cellNIBC}=maximum(0.,{centers:cellNIBC})')# vaut -3, 0, 1, 2, 3 initialement
    C._initVars(t,'{centers:cellNIBC}={centers:cellNIBC}*({centers:cellNIBC}<2.5)')
    C._cpVars(t,'centers:cellNIBC',t,'centers:cellN')
    C._cpVars(t,'centers:cellN',tc,'cellN')

    # Transfert du cellNFront
    C._cpVars(t,'centers:cellNFront',tc,'cellNFront')
    
    # propager cellNVariable='cellNFront'
    Xmpi._setInterpTransfers(t, tc, variables=['cellNFront'], cellNVariable='cellNFront', compact=0)

    if twoFronts:
        C._cpVars(t,'centers:cellNFront_2',tc,'cellNFront_2')
        Xmpi._setInterpTransfers(t, tc, variables=['cellNFront_2'], cellNVariable='cellNFront_2', compact=0)

    if frontType == 2: _pushBackImageFront2__(t, tc, tbbc, interpDataType=interpDataType, cartesian=cartesian)
        
    C._rmVars(t,['centers:cellNFront'])
    if twoFronts: C._rmVars(t,['centers:cellNFront_2', 'centers:cellNIBC_2'])
    
    C._cpVars(t,'centers:TurbulentDistance',tc,'TurbulentDistance')
    
    print('Minimum distance: %f.'%C.getMinValue(t,'centers:TurbulentDistance'))
    P._computeGrad2(t, 'centers:TurbulentDistance', ghostCells=True, withCellN=False)

    front = getIBMFront(tc, 'cellNFront', dim=dimPb, frontType=frontType)
    front = gatherFront(front)

    if twoFronts:
        front2 = getIBMFront(tc, 'cellNFront_2', dim=dimPb, frontType=frontType)
        front2 = gatherFront(front2)
    else:
        front2 = None

    if check and Cmpi.rank == 0:
        C.convertPyTree2File(front, 'front.cgns')
        if twoFronts: C.convertPyTree2File(front2, 'front2.cgns')
        
    return t, tc, front, front2

# INTERP DATA IBM  #######################
def _setInterpDataIBM(t, tc, tb, front, front2=None, dimPb=3, NP=0, frontType=1, DEPTH=2, IBCType=1, interpDataType=0, Reynolds=1.e6, 
                    yplus=100, Lref=1., twoFronts=False, isOrthoProjectFirst=False, isWireModel=False, cartesian=False): 

    tbbc = Cmpi.createBBoxTree(tc)

    zonesRIBC = []
    for zrcv in Internal.getZones(t):
        if C.getMaxValue(zrcv, 'centers:cellNIBC')==2.:
            zonesRIBC.append(zrcv)
    
    nbZonesIBC = len(zonesRIBC)
    if nbZonesIBC == 0:
        res = [{},{},{}]
        if twoFronts: res2 = [{},{},{}]
    else:
        res = getAllIBMPoints(zonesRIBC, loc='centers',tb=tb, tfront=front, frontType=frontType,
                              cellNName='cellNIBC', depth=DEPTH, IBCType=IBCType, Reynolds=Reynolds, yplus=yplus, Lref=Lref, isOrthoFirst=isOrthoProjectFirst)
        if twoFronts:
            res2 = getAllIBMPoints(zonesRIBC, loc='centers',tb=tb, tfront=front2, frontType=frontType,
                                   cellNName='cellNIBC', depth=DEPTH, IBCType=IBCType, Reynolds=Reynolds, yplus=yplus, Lref=Lref)
        if isWireModel:
            res2 = getAllIBMPoints(zonesRIBC, loc='centers',tb=tb, tfront=front, frontType=frontType,
                                    cellNName='cellNIBC', depth=DEPTH, IBCType=IBCType, Reynolds=Reynolds, yplus=yplus, Lref=Lref,isWireModel=isWireModel,
                                    isOrthoFirst=isOrthoProjectFirst)
    
    # cleaning
    C._rmVars(tc,['cellNChim','cellNIBC','TurbulentDistance','cellNFront'])
    # dans t, il faut cellNChim et cellNIBCDnr pour recalculer le cellN a la fin
    varsRM = ['centers:gradxTurbulentDistance','centers:gradyTurbulentDistance','centers:gradzTurbulentDistance','centers:cellNFront','centers:cellNIBC']
    C._rmVars(t, varsRM)
    front = None
    if twoFronts: front2 = None

    graph = {}; datas = {}
    procDict = Cmpi.getProcDict(tc)

    # graph d'intersection des pts images de ce proc et des zones de tbbc
    zones  = Internal.getZones(tbbc)
    allBBs = []
    dictOfCorrectedPtsByIBCType = res[0]
    dictOfWallPtsByIBCType      = res[1]
    dictOfInterpPtsByIBCType    = res[2]
    interDictIBM={}

    if twoFronts or isWireModel:
        dictOfCorrectedPtsByIBCType2 = res2[0]
        dictOfWallPtsByIBCType2      = res2[1]
        dictOfInterpPtsByIBCType2    = res2[2]
    else:
        dictOfCorrectedPtsByIBCType2={}
        dictOfWallPtsByIBCType2     ={}
        dictOfInterpPtsByIBCType2   ={}
    interDictIBM2={}    

    if dictOfCorrectedPtsByIBCType!={}:
        for ibcTypeL in dictOfCorrectedPtsByIBCType:
            allCorrectedPts = dictOfCorrectedPtsByIBCType[ibcTypeL]
            allWallPts      = dictOfWallPtsByIBCType[ibcTypeL]
            allInterpPts    = dictOfInterpPtsByIBCType[ibcTypeL]
            for nozr in range(nbZonesIBC):
                if allCorrectedPts[nozr] != []:
                    zrname = zonesRIBC[nozr][0]
                    interpPtsBB = Generator.BB(allInterpPts[nozr])
                    for z in zones:
                        bba = C.getFields('GridCoordinates', z)[0]
                        if Generator.bboxIntersection(interpPtsBB, bba, isBB=True):
                            zname = z[0]
                            popp  = Cmpi.getProc(z)
                            if NP>1:
                                Distributed.updateGraph__(graph, popp, Cmpi.rank, zname)
                            
                            if zrname not in interDictIBM: interDictIBM[zrname]=[zname]
                            else:
                                if zname not in interDictIBM[zrname]: interDictIBM[zrname].append(zname)
        if twoFronts or isWireModel:
            for ibcTypeL in dictOfCorrectedPtsByIBCType2:
                    allCorrectedPts2 = dictOfCorrectedPtsByIBCType2[ibcTypeL]
                    allWallPts2      = dictOfWallPtsByIBCType2[ibcTypeL]
                    allInterpPts2    = dictOfInterpPtsByIBCType2[ibcTypeL]
                    for nozr in range(nbZonesIBC):
                        if allCorrectedPts2[nozr] != []:
                            zrname = zonesRIBC[nozr][0]
                            interpPtsBB2 = Generator.BB(allInterpPts2[nozr])
                            for z in zones:
                                bba = C.getFields('GridCoordinates', z)[0]
                                if Generator.bboxIntersection(interpPtsBB2,bba,isBB=True):
                                    zname = z[0]
                                    popp  = Cmpi.getProc(z)
                                    if NP > 1:
                                        Distributed.updateGraph__(graph, popp, Cmpi.rank, zname)
                                    if zrname not in interDictIBM2: interDictIBM2[zrname]=[zname]
                                    else:
                                        if zname not in interDictIBM2[zrname]: interDictIBM2[zrname].append(zname)
    else: graph={}
    allGraph = Cmpi.KCOMM.allgather(graph)

    graph = {}
    for i in allGraph:
        for k in i:
            if not k in graph: graph[k] = {}
            for j in i[k]:
                if not j in graph[k]: graph[k][j] = []
                graph[k][j] += i[k][j]
                graph[k][j] = list(set(graph[k][j])) # pas utile?

    # keyword subr=False to avoid memory overflow
    Cmpi._addXZones(tc, graph, variables=['cellN'], cartesian=cartesian, subr=False)

    ReferenceState = Internal.getNodeFromType2(t, 'ReferenceState_t')   

    for i in range(Cmpi.size): datas[i] = [] # force

    if dictOfCorrectedPtsByIBCType!={}:
        for ibcTypeL in dictOfCorrectedPtsByIBCType:
            allCorrectedPts = dictOfCorrectedPtsByIBCType[ibcTypeL]
            allWallPts = dictOfWallPtsByIBCType[ibcTypeL]
            allInterpPts = dictOfInterpPtsByIBCType[ibcTypeL]
            for nozr in range(nbZonesIBC):
                if allCorrectedPts[nozr] != []:
                    zrcv = zonesRIBC[nozr]
                    zrname = zrcv[0]
                    dnrZones = []
                    for zdname in interDictIBM[zrname]:
                        zd = Internal.getNodeFromName2(tc, zdname)
                        if zd is None: print('!!!Zone None', zrname, zdname)
                        else: dnrZones.append(zd)
                    XOD._setIBCDataForZone__(zrcv, dnrZones, allCorrectedPts[nozr], allWallPts[nozr], allInterpPts[nozr],
                                                nature=1, penalty=1, loc='centers', storage='inverse', dim=dimPb,
                                                interpDataType=interpDataType, ReferenceState=ReferenceState, bcType=ibcTypeL)

                    nozr += 1
                    for zd in dnrZones:
                        zdname = zd[0]
                        destProc = procDict[zdname]

                        IDs = []
                        for i in zd[2]:
                            if i[0][0:4] == 'IBCD':
                                if Internal.getValue(i)==zrname: IDs.append(i)

                        if IDs != []:
                            if destProc == Cmpi.rank:
                                zD = Internal.getNodeFromName2(tc,zdname)
                                zD[2] += IDs
                            else:
                                if destProc not in datas: datas[destProc]=[[zdname,IDs]]
                                else: datas[destProc].append([zdname,IDs])
                        else:
                            if destProc not in datas: datas[destProc] = []

    if dictOfCorrectedPtsByIBCType2!={}:
        for ibcTypeL in dictOfCorrectedPtsByIBCType2:
            allCorrectedPts2 = dictOfCorrectedPtsByIBCType2[ibcTypeL]
            allWallPts2      = dictOfWallPtsByIBCType2[ibcTypeL]
            allInterpPts2    = dictOfInterpPtsByIBCType2[ibcTypeL]
            for nozr in range(nbZonesIBC):
                if allCorrectedPts2[nozr] != []:
                    zrcv     = zonesRIBC[nozr]
                    zrname   = zrcv[0]
                    dnrZones = []
                    for zdname in interDictIBM2[zrname]:
                        zd = Internal.getNodeFromName2(tc, zdname)
                        #if zd is not None: dnrZones.append(zd)
                        if zd is None: print('!!!Zone None', zrname, zdname)
                        else: dnrZones.append(zd)
                    XOD._setIBCDataForZone2__(zrcv, dnrZones, allCorrectedPts2[nozr], allWallPts2[nozr], None, allInterpPts2[nozr],
                                                nature=1, penalty=1, loc='centers', storage='inverse', dim=dimPb,
                                                interpDataType=interpDataType, ReferenceState=ReferenceState, bcType=ibcTypeL)
        
                    nozr += 1
                    for zd in dnrZones:
                        zdname = zd[0]
                        destProc = procDict[zdname]
        
                        IDs = []
                        for i in zd[2]:
                            if i[0][0:6] == '2_IBCD':
                                if Internal.getValue(i)==zrname: IDs.append(i)
        
                        if IDs != []:
                            if destProc == Cmpi.rank:
                                zD = Internal.getNodeFromName2(tc,zdname)
                                zD[2] += IDs
                            else:
                                if destProc not in datas: datas[destProc]=[[zdname,IDs]]
                                else: datas[destProc].append([zdname,IDs])
                        else:
                            if destProc not in datas: datas[destProc] = []

    Cmpi._rmXZones(tc)
    dictOfCorrectedPtsByIBCType = None
    dictOfWallPtsByIBCType      = None
    dictOfInterpPtsByIBCType    = None
    interDictIBM = None
    if twoFronts or isWireModel:
        dictOfCorrectedPtsByIBCType2 = None
        dictOfWallPtsByIBCType2      = None
        dictOfInterpPtsByIBCType2    = None
        interDictIBM2 = None

    Internal._rmNodesByName(tc, Internal.__FlowSolutionNodes__)
    destDatas = Cmpi.sendRecv(datas, graph)
    for i in destDatas:
        for n in destDatas[i]:
            zname = n[0]
            IBCDs = n[1]
            if IBCDs != []:
                zD = Internal.getNodeFromName2(tc, zname)
                zD[2] += IBCDs

    model = Internal.getNodeFromName(tb, 'GoverningEquations')
    model = Internal.getValue(model)

    C._initVars(t,'{centers:cellN}=minimum({centers:cellNChim}*{centers:cellNIBCDnr},2.)')
    varsRM = ['centers:cellNChim','centers:cellNIBCDnr']
    if model == 'Euler': varsRM += ['centers:TurbulentDistance']
    C._rmVars(t, varsRM)

    return None

# INITIALiZE  ############################
def _changeNameIBCD__(tc,NewIBCD):
    ZsubR = Internal.getNodesByType(tc, 'ZoneSubRegion_t')
    for z in ZsubR:
        zsplit = z[0].split('_')
        if zsplit[0] == 'IBCD':
            zsplit[1] = str(NewIBCD)
            znew = '_'.join(zsplit)
            Internal.setName(z, znew)
    return None   

def _tcInitialize__(tc, tc2=None, twoFronts=False, isWireModel=False, ibctypes=[]):
    import Geom.IBM as D_IBM

    if twoFronts or isWireModel:
        tc2 = Internal.copyTree(tc)
        tc2 = Internal.rmNodesByName(tc2, 'IBCD*')
        tc  = Internal.rmNodesByName(tc, '2_IBCD*')
        
        if isWireModel:
            tc2 = Internal.rmNodesByName(tc2, 'ID*')
            tc2 = Internal.rmNodesByName(tc2, 'gradxPressure')
            tc2 = Internal.rmNodesByName(tc2, 'gradyPressure')
            tc2 = Internal.rmNodesByName(tc2, 'gradzPressure')
    
            D_IBM._transformTc2(tc2)
            _changeNameIBCD__(tc,140)
            _changeNameIBCD__(tc2,141)
            tc=Internal.merge([tc,tc2])
            del tc2

        else:
            D_IBM._transformTc2(tc2)
    
    if RENAMEIBCNODES:
        for zc in Internal.getZones(tc):
            for ibcd in Internal.getNodesFromName1(zc,'IBCD_*'):            
                proposedName = Internal.getName(ibcd)[0:6]+'_X%d'%(Cmpi.rank)
                ibcd[0] = getIBCDName(proposedName)
        
        if twoFronts:
            for zc in Internal.getZones(tc2):
                for ibcd in Internal.getNodesFromName1(zc,'2_IBCD_*'):            
                    proposedName = Internal.getName(ibcd)[0:8]+'_X%d'%(Cmpi.rank)
                    ibcd[0] = getIBCDName(proposedName)

    ##Adding a userdefined node to the tc tree for the IBC conditions that are provided
    ##to FastS solver to reduce the number of input arguments and to make a clear distinction
    ##of the solver parameters and the IBC parameters
    base = Internal.getBases(tc)[0]
    Internal._createUniqueChild(base, '.Solver#IBCdefine', 'UserDefinedData_t')
    solverIBC = Internal.getNodeFromName1(base, '.Solver#IBCdefine')
    
    Internal._createUniqueChild(solverIBC, 'Reref', 'DataArray_t', -1)
    Internal._createUniqueChild(solverIBC, 'Lref' , 'DataArray_t',  1)
    
    Internal._createUniqueChild(solverIBC, 'isgradP'    , 'DataArray_t', 'False')
    Internal._createUniqueChild(solverIBC, 'isWireModel', 'DataArray_t', 'False')
    Internal._createUniqueChild(solverIBC, 'isTBLE'     , 'DataArray_t', 'False')
    
    ##note: here alphagrad is the corrected nomenclature for alghagradp found in param_solver.h (B.Constant confirmed)
    ##      changed some other variables names to be consistent with other options/coding "guidelines"
    if 'Mafzal' in ibctypes:
        Internal._createUniqueChild(solverIBC, 'isgradP'   , 'DataArray_t', 'True')
        Internal._createUniqueChild(solverIBC, 'mafzalMode', 'DataArray_t', 0)
        Internal._createUniqueChild(solverIBC, 'alphaGrad' , 'DataArray_t', 0)            
    if twoFronts:
        Internal._createUniqueChild(solverIBC, 'isgradP'    , 'DataArray_t', 'True')
        Internal._createUniqueChild(solverIBC, 'alphaGrad'  , 'DataArray_t', 0)
    if isWireModel:
        Internal._createUniqueChild(solverIBC, 'isWireModel' , 'DataArray_t', 'True')
        Internal._createUniqueChild(solverIBC, 'DeltaVWire'  , 'DataArray_t', 0)
        Internal._createUniqueChild(solverIBC, 'KWire'       , 'DataArray_t', 0)
        Internal._createUniqueChild(solverIBC, 'DiameterWire', 'DataArray_t', 0)
        Internal._createUniqueChild(solverIBC, 'CtWire'      , 'DataArray_t', 0) 
    if 'TBLE' in ibctypes or 'TBLE_FULL' in ibctypes:
        Internal._createUniqueChild(solverIBC, 'isTBLE'        , 'DataArray_t', 'True')
        Internal._createUniqueChild(solverIBC, 'alphaGrad'     , 'DataArray_t', 0)
        Internal._createUniqueChild(solverIBC, 'NbPtsLinelits' , 'DataArray_t', 0)       

    return None

def _tInitialize__(t, tb, tinit=None, model='NSTurbulent', isWireModel=False, initWithBBox=-1):
    if tinit is None: I._initConst(t, loc='centers')
    else:
        t = Pmpi.extractMesh(tinit, t, mode='accurate')
    if model != "Euler": C._initVars(t, 'centers:ViscosityEddy', 0.)
    
    if isWireModel:
        vars_wm = ['Density','VelocityX','VelocityY','VelocityZ','Temperature']
        if model == 'NSTurbulent': vars_wm.append('TurbulentSANuTilde')     
        for z in Internal.getZones(t):
            for v_local in vars_wm:
                C._initVars(z,'{centers:'+v_local+'_WM}=0.')
                
    # Init with BBox
    if initWithBBox>0.:
        print('INFO: Initialisation par bounding box')
        bodybb = C.newPyTree(['Base'])
        for base in Internal.getBases(tb):
            bbox = G.bbox(base)
            bodybbz = D.box(tuple(bbox[:3]),tuple(bbox[3:]), N=2, ntype='STRUCT')
            Internal._append(bodybb,bodybbz,'Base')
        T._scale(bodybb, factor=(initWithBBox,initWithBBox,initWithBBox))
        tbb = G.BB(t)
        interDict = X.getIntersectingDomains(tbb,bodybb,taabb=tbb,taabb2=bodybb)
        for zone in Internal.getZones(t):
            zname = Internal.getName(zone)
            if interDict[zname] != []:
                C._initVars(zone, 'centers:MomentumX', 0.)
                C._initVars(zone, 'centers:MomentumY', 0.)
                C._initVars(zone, 'centers:MomentumZ', 0.)
    return None

####################################################################################

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
## BLANKING
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
def _blankClosestTargetCells(t, cellNName='cellN', depth=3):
    """Blank target points."""
    for z in Internal.getZones(t):
        connector._blankClosestTargetCells(z, depth, cellNName,
                                           Internal.__GridCoordinates__,
                                           Internal.__FlowSolutionNodes__,
                                           Internal.__FlowSolutionCenters__)
    return None

# Remove fully blanked grids considering cellNNIBC and cellNChim
def _removeBlankedGrids(t, loc='centers'):
    """Delete zones when fully blanked considering cellNNIBC and cellNChim."""
    vari = 'cellNIBC'
    varc = 'cellNChim'
    flag = 'flag'
    if loc == 'centers':
        vari = 'centers:'+vari
        varc = 'centers:'+varc
        flag = 'centers:'+flag
    
    C._initVars(t,'{%s}=abs(1.-{%s}*{%s})<0.5'%(flag,vari,varc))
    
    #poschim = C.isNamePresent(t,loc+varc)
    #posibc = C.isNamePresent(t,loc+vari)
    #if poschim != -1 and posibc != -1: C._initVars(t,'{%s}=abs(1.-{%s}*{%s})<0.5'%(flag,vari,varc))
    #elif poschim != -1 and posibc==-1: flag=varc
    #elif poschim == -1 and posibc!=-1: flag=vari
    #else: return None

    for z in Internal.getZones(t):
        if C.getMaxValue(z,flag) < 0.5:
            (parent,noz) = Internal.getParentOfNode(t, z)
            del parent[2][noz]
        else:
            #if poschim != -1 and posibc != -1: C._rmVars(z,[flag])
            C._rmVars(z,[flag])
    return None

#==============================================================================
# masquage par les corps IBC
# gridType = single or composite - composite means that an off body grid exists
# IN: tb: immersed bodies
# IN: loc: "centers" or "nodes"
# IN: dim: 2 or 3
#==============================================================================
def blankByIBCBodies(t, tb, loc, dim, cellNName='cellN'):
    """Blank by immersed bodies."""
    DIM = dim
    blankalgo='tri'
    #blankalgo='xray'
    if DIM == 2: blankalgo = 'xray'

    bodies = []
    for b in Internal.getBases(tb):
        wallsl = Internal.getNodesFromType1(b, 'Zone_t')
        #soldef = Internal.getNodeFromName(wallsl,'.Solver#define')
        bodies.append(wallsl)
        # if wallsl != []:
        #     try:
        #         wallsl = C.convertArray2Tetra(wallsl)
        #         wallsl = T.join(wallsl)
        #         wallsl = G.close(wallsl)
        #         Internal.addChild(wallsl,soldef)
        #         bodies.append([wallsl])
        #         # if DIM == 3:
        #         #     try: P.exteriorFaces(wallsl)
        #         #     except: pass
        #         #     bodies.append([wallsl])
        #         # else: bodies.append([wallsl])
        #     except:
        #         wallsl = C.convertArray2Tetra(wallsl)
        #         bodies.append(wallsl)

    nbodies = len(bodies)
    if nbodies == 0:
        print("Warning: blankByIBCBodies: no body defined.")
        return t

    print('Info: blankByIBCBodies: blanking mesh by %d bodies'%nbodies)
    if loc == 'centers': typeb = 'center_in'
    else: typeb = 'node_in'
    nbases = len(Internal.getBases(t))

    bodiesInv=[]
    for body in bodies:
        inv = Internal.getNodeFromName(body,'inv')
        if inv is not None: inv = Internal.getValue(inv)
        else: inv = 0
        if inv == 1:
            bodies.remove(body)
            bodiesInv.append(body)

    if blankalgo == 'xray' or DIM == 2:
        BM = numpy.ones((nbases,nbodies),dtype=numpy.int32)
        dh_min = G_IBM_Height.getMinimumCartesianSpacing(t)
        XRAYDIM1 = 2000; XRAYDIM2 = XRAYDIM1
        if dh_min > 0.:
            bb = G.bbox(tb)
            Lxref = bb[3]-bb[0]
            Lyref = bb[4]-bb[1]
            XRAYDIM1 = max(XRAYDIM1,int(Lxref/(0.15*dh_min)))
            XRAYDIM2 = max(XRAYDIM2,int(Lyref/(0.15*dh_min)))
        if DIM == 2: XRAYDIM2 = 2

        if loc == 'centers':
            tc = C.node2Center(t)
            for body in bodiesInv:
                print('Info: blankByIBCBodies: reverse blanking for body.')
                tc = X.blankCells(tc, [body], BM, blankingType='node_in', XRaydim1=XRAYDIM1, XRaydim2=XRAYDIM2, dim=DIM, cellNName=cellNName)
                C._initVars(tc,'{%s}=1.-{%s}'%(cellNName,cellNName)) # ecoulement interne

            for body in bodies:
                tc = X.blankCells(tc, [body], BM, blankingType='node_in', XRaydim1=XRAYDIM1, XRaydim2=XRAYDIM2, dim=DIM, cellNName=cellNName)
            C._cpVars(tc,'%s'%cellNName,t,'centers:%s'%cellNName)
        else:
            t = X.blankCells(t, bodies, BM, blankingType=typeb, delta=TOLDIST, XRaydim1=XRAYDIM1, XRaydim2=XRAYDIM2, dim=DIM, cellNName=cellNName)
    else:
        BM2 = numpy.ones((nbases,1),dtype=numpy.int32)
        for body in bodiesInv:
            print('Info: blankByIBCBodies: reverse blanking for body.')
            t = X.blankCellsTri(t, [body], BM2, blankingType=typeb, cellNName=cellNName)
            C._initVars(t,'{centers:%s}=1-{centers:%s}'%(cellNName,cellNName)) # ecoulement interne
        for body in bodies:
            t = X.blankCellsTri(t, [body], BM2, blankingType=typeb, cellNName=cellNName)
    return t


#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
##BC
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# IN: bbox: bbox des frontieres exterieures
def _addBCOverlaps(t, bbox):
    """Add BCOverlap boundary condition to BCs entirely inside bbox."""
    xmin = bbox[0]; ymin = bbox[1]; zmin = bbox[2]
    xmax = bbox[3]; ymax = bbox[4]; zmax = bbox[5]
    for z in Internal.getZones(t):
        # [x1,y1,z1,x2,y2,z2] = G.bbox(z)
        dimZ = Internal.getZoneDim(z)
        niz = dimZ[1]; njz = dimZ[2]; nkz = dimZ[3]
        indM = niz-1+(njz-1)*niz+(nkz-1)*niz*njz
        x1 = C.getValue(z,'CoordinateX',0)
        y1 = C.getValue(z,'CoordinateY',0)
        z1 = C.getValue(z,'CoordinateZ',0)
        x2 = C.getValue(z,'CoordinateX',indM)
        y2 = C.getValue(z,'CoordinateY',indM)
        z2 = C.getValue(z,'CoordinateZ',indM)
        if x1 > xmin+EPSCART: C._addBC2Zone(z,'overlap1','BCOverlap','imin')
        if x2 < xmax-EPSCART: C._addBC2Zone(z,'overlap2','BCOverlap','imax')
        if y1 > ymin+EPSCART: C._addBC2Zone(z,'overlap3','BCOverlap','jmin')
        if y2 < ymax-EPSCART: C._addBC2Zone(z,'overlap4','BCOverlap','jmax')
        if z1 > zmin+EPSCART: C._addBC2Zone(z,'overlap5','BCOverlap','kmin')
        if z2 < zmax-EPSCART: C._addBC2Zone(z,'overlap6','BCOverlap','kmax')
    return None


def _addExternalBCs(t, bbox, DEPTH=2, externalBCType='BCFarfield', dimPb=3):
    """Add external BCs of given type to BCs out of or on bbox."""
    dirs = [0,1,2,3,4,5]
    rangeDir=['imin','jmin','kmin','imax','jmax','kmax']
    if dimPb == 2: dirs = [0,1,3,4]
    nptsTot = 0
    for zp in Internal.getZones(t):
        dimZ = Internal.getZoneDim(zp)
        niz = dimZ[1]; njz = dimZ[2]; nkz = dimZ[3]
        nptsTot += niz*njz*nkz
        indM = niz-1+(njz-1)*niz+(nkz-1)*niz*njz
        x1 = C.getValue(zp,'CoordinateX',0)
        y1 = C.getValue(zp,'CoordinateY',0)
        z1 = C.getValue(zp,'CoordinateZ',0)
        x2 = C.getValue(zp,'CoordinateX',indM)
        y2 = C.getValue(zp,'CoordinateY',indM)
        z2 = C.getValue(zp,'CoordinateZ',indM)
        bbz=[x1,y1,z1,x2,y2,z2]
        external = False
        for idir in dirs:
            if abs(bbz[idir]-bbox[idir])< 1.e-6:
                C._addBC2Zone(zp, 'external', externalBCType, rangeDir[idir])
                external = True
        if externalBCType != 'BCOverlap' and externalBCType != 'BCDummy':
            if external: _modifPhysicalBCs__(zp, depth=DEPTH, dimPb=dimPb)
    return None


# Reduction de la taille des fenetres des BC physiques pour qu'elles soient
# traitees comme des ghost cells
def _modifPhysicalBCs__(zp, depth=2, dimPb=3):
    dimZone = Internal.getZoneDim(zp)

    # Physical BCs
    bclist = Internal.getNodesFromType2(zp, 'BC_t')
    for bc in bclist:
        prange = Internal.getNodesFromName1(bc, 'PointRange')
        if prange != []:
            direction = CGC.getDirection__(dimPb, prange)
            # change PointRange for extended mesh
            pr = numpy.copy(prange[0][1])
            ijk = int(direction/2)
            minmax = direction%2
            for dirl in range(dimZone[4]):
                if dirl != ijk:
                    if dimPb == 2 and dirl == 2: pass
                    else:
                        if dirl == 0: N = dimZone[1]
                        elif dirl == 1: N = dimZone[2]
                        else: N = dimZone[3]
                        pr[dirl][0] += depth
                        pr[dirl][1] -= depth
            prange[0][1] = pr
    return None



#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
## FRONT
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#=============================================================================
# Returns the front defining the image points
# IN: frontvar: 
# IN: dim: 2 or 3
# IN: frontType: 0 (equal distance), 1 (minimum distance), 42 (modelling height)
#=============================================================================
def getIBMFront(tc, frontvar, dim, frontType, isFront2=False, SHIFTB=0.):
    """Returns the front used to define the location of the IBM image points."""

    # if frontType == 1 or frontType == 2 : front = getIBMFrontType1__(tc,frontvar,dim)
    if frontType == 0:
        front = getIBMFrontType0__(tc,frontvar,dim,isFront2,frontType,SHIFTB)
    else:
        front = getIBMFrontType1__(tc, frontvar, dim)
    front = C.deleteEmptyZones(front)
    Internal._rmNodesFromName(front, "ID_*")
    if frontType == 0: return front

    dxmin = 1.e12
    if frontType>0:
        front = Internal.getZones(front)
        dxmax = 1.e-12
        dht = [[]]*len(front)
        nof = 0
        for f in front:
            subf = T.subzone(f, [0], type='elements')
            dx = C.getMaxValue(subf,"CoordinateX")-C.getMinValue(subf,"CoordinateX")
            dy = C.getMaxValue(subf,"CoordinateY")-C.getMinValue(subf,"CoordinateY")
            if dim == 3:
                dz = C.getMaxValue(subf,"CoordinateZ")-C.getMinValue(subf,"CoordinateZ")
                dht[nof] = max(dx,dy,dz)
            else:
                dht[nof] = max(dx,dy)

            if dht[nof] < dxmin and dht[nof] > 1.e-12:
                dxmin = dht[nof]
            if dht[nof] > dxmax:
                dxmax = dht[nof]
            nof += 1

        nlevels = int(math.log(dxmax/dxmin)/math.log(2)+1)

        dictOfLevels={}
        for l in range(nlevels): dictOfLevels[l]=[]
        for nof in range(len(dht)):
            if dht[nof]>1.e-12:
                nolvl = int(math.log(dht[nof]/dxmin)/math.log(2))
                dictOfLevels[nolvl].append(front[nof])

        front=[]
        for nol in range(nlevels):
            if len(dictOfLevels[nol])>0:
                front.append(T.join(dictOfLevels[nol]))

    return front


# front of first computed cells - with overlapping
def getIBMFrontType1__(tc, frontvar, dim):
    if dim == 2:
        z0 = Internal.getNodeFromType2(tc, 'Zone_t')
        if z0 is not None:
            zmean = C.getValue(z0, 'CoordinateZ', 0)
            dz = 2*zmean
        else: zmean=0.5; dz = 1.
    else: dz = 0.
    front = []
    for z in Internal.getZones(tc):
        if C.getMinValue(z,frontvar)<0.2 and C.getMaxValue(z,frontvar)>0.8:
            X._maximizeBlankedCells(z,depth=1,dir=1,loc='nodes', cellNName='cellNChim')
            C._initVars(z,'{cellNChim}=minimum(1.,{cellNChim})')
            f = P.frontFaces(z, frontvar)
            if Internal.getZoneDim(f)[1]>0:
                Internal._rmNodesByName(f,'ID_*')
                front.append(f)
    C._initVars(front,'{tag}=({cellNChim}>0.5)*({cellNChim}<1.5)')
    front = P.selectCells2(front, 'tag', strict=1)
    Internal._rmNodesByName(front,Internal.__FlowSolutionNodes__)
    Internal._rmNodesByName(front,Internal.__FlowSolutionCenters__)
    if dim == 2:
        T._addkplane(front)
        T._translate(front,(0,0,-zmean))
        T._contract(front, (0,0,0), (1,0,0), (0,1,0), 0.9*dz)
    return front


# isosurface of max dist of the first interpolable cells
def getIBMFrontType0__(tc, frontvar, dim, isFront2=False, frontType=0, SHIFTB=0.):
    import Converter.Mpi as Cmpi
    
    if dim == 2:
        z0 = Internal.getNodeFromType2(tc, 'Zone_t')
        zmean = C.getValue(z0, 'CoordinateZ', 0)
        dz = 2*zmean
    else: dz = 0.

    SHIFTD = 1.+SHIFTF
    # SHIFTD *= SHIFTB
    front = []
    tf = Internal.copyRef(tc)
    C._initVars(tf,'{%s}={%s}-2.*({%s}>1.5)'%(frontvar,frontvar,frontvar))
    if isFront2:
        if dim == 2:
            front = []
            # Creation du corps 2D pour le preprocessing IBC
            for z in Internal.getZones(tf):
                epsilon_dist = abs(C.getValue(z,'CoordinateX',1)-C.getValue(z,'CoordinateX',0))
                dmin = math.sqrt(2)*5*epsilon_dist
                if frontType == 42:
                    dmin = max(dmin, SHIFTB+math.sqrt(2)*3*epsilon_dist) # where shiftb = hmod
                tcl = T.addkplane(z)
                T._translate(tcl,(0,0,-zmean))
                T._contract(tcl, (0,0,0), (1,0,0), (0,1,0), dz)
                front.append(P.isoSurfMC(tcl,'TurbulentDistance',dmin*SHIFTD))
                del tcl
            front = C.newPyTree(['Base']+front)
        else:
            front = []
            # Creation du corps 2D pour le preprocessing IBC
            for z in Internal.getZones(tf):
                epsilon_dist = abs(C.getValue(z,'CoordinateX',1)-C.getValue(z,'CoordinateX',0))
                dmin = math.sqrt(3)*5*epsilon_dist
                if frontType == 42:
                    dmin = max(dmin, SHIFTB+math.sqrt(3)*3*epsilon_dist) # where shiftb = hmod
                front.append(P.isoSurfMC(z,'TurbulentDistance',dmin*SHIFTD))
            front = C.newPyTree(['Base']+front)

    else:
        for z in Internal.getZones(tf):
            if C.getMinValue(z,frontvar)==0. and C.getMaxValue(z,frontvar)==1.:
                f = P.frontFaces(z, frontvar)
                if Internal.getZoneDim(f)[1]>0:
                    Internal._rmNodesByName(f,'ID_*')
                    front.append(f)
        if dim == 2:
            dmin = C.getMaxValue(front, 'TurbulentDistance')

            if Cmpi.KCOMM is not None:
                dmin = numpy.array([dmin])
                dmin_max = numpy.zeros(1)
                Cmpi.KCOMM.Allreduce(dmin, dmin_max, Cmpi.MAX)
                dmin = dmin_max[0]

            tcl = T.addkplane(tc)
            T._translate(tcl,(0,0,-zmean))
            T._contract(tcl, (0,0,0), (1,0,0), (0,1,0), dz)
            front = P.isoSurfMC(tcl,'TurbulentDistance',dmin*SHIFTD)
            del tcl
        else:
            dmin = C.getMaxValue(front, 'TurbulentDistance')

            if Cmpi.KCOMM is not None:
                dmin = numpy.array([dmin])
                dmin_max = numpy.zeros(1)
                Cmpi.KCOMM.Allreduce(dmin, dmin_max, Cmpi.MAX)
                dmin = dmin_max[0]

            front = P.isoSurfMC(tc, 'TurbulentDistance', dmin*SHIFTD)
    return front


# isosurface of max dist of the first interpolable cells
def getIBMFrontType0Old__(tc, frontvar, dim, isFront2=False, frontType=0, SHIFTB=0.):
    import Converter.Mpi as Cmpi
    
    if dim == 2:
        z0 = Internal.getNodeFromType2(tc, 'Zone_t')
        zmean = C.getValue(z0, 'CoordinateZ', 0)
        dz = 2*zmean
    else: dz = 0.

    SHIFTD = 1.+SHIFTF
    # SHIFTD *= SHIFTB
    front = []
    tf = Internal.copyRef(tc)
    C._initVars(tf,'{%s}={%s}-2.*({%s}>1.5)'%(frontvar,frontvar,frontvar))
    if isFront2:
        for z in Internal.getZones(tf):
            epsilon_dist = abs(C.getValue(z,'CoordinateX',1)-C.getValue(z,'CoordinateX',0))
            if epsilon_dist < SHIFTB:
                if C.getMinValue(z,frontvar)==0. and C.getMaxValue(z,frontvar)==1.:
                    f = P.frontFaces(z, frontvar)
                    if Internal.getZoneDim(f)[1]>0:
                        Internal._rmNodesByName(f,'ID_*')
                        front.append(f)

        if dim == 2:
            if frontType != 1:
                dmin = C.getMaxValue(front, 'TurbulentDistance')

                if Cmpi.KCOMM is not None:
                    dmin = numpy.array([dmin])
                    dmin_max = numpy.zeros(1)
                    Cmpi.KCOMM.Allreduce(dmin, dmin_max, Cmpi.MAX)
                    dmin = dmin_max[0]
            else:
                dmin = 2*SHIFTB

            print("before : {}".format(dmin*SHIFTD*1.125))
            if frontType == 42: dmin += SHIFTB
            print('after : {}'.format(dmin*SHIFTD))
            front = []
            # Creation du corps 2D pour le preprocessing IBC
            for z in Internal.getZones(tf):
                epsilon_dist = abs(C.getValue(z,'CoordinateX',1)-C.getValue(z,'CoordinateX',0))
                if epsilon_dist < SHIFTB:
                    tcl = T.addkplane(z)
                    T._translate(tcl,(0,0,-zmean))
                    T._contract(tcl, (0,0,0), (1,0,0), (0,1,0), dz)
                    front.append(P.isoSurfMC(tcl,'TurbulentDistance',dmin*SHIFTD))
                    del tcl
            front = C.newPyTree(['Base']+front)
        else:
            dmin = C.getMaxValue(front, 'TurbulentDistance')

            if Cmpi.KCOMM is not None:
                dmin = numpy.array([dmin])
                dmin_max = numpy.zeros(1)
                Cmpi.KCOMM.Allreduce(dmin, dmin_max, Cmpi.MAX)
                dmin = dmin_max[0]

            print("before : {}".format(dmin*SHIFTD*1.125))
            if frontType == 42: dmin += SHIFTB
            print('after : {}'.format(dmin*SHIFTD))
            front = []
            # Creation du corps 2D pour le preprocessing IBC
            for z in Internal.getZones(tf):
                epsilon_dist = abs(C.getValue(z,'CoordinateX',1)-C.getValue(z,'CoordinateX',0))
                if epsilon_dist < SHIFTB:
                    front.append(P.isoSurfMC(z,'TurbulentDistance',dmin*SHIFTD))
            front = C.newPyTree(['Base']+front)

    else:
        for z in Internal.getZones(tf):
            if C.getMinValue(z,frontvar)==0. and C.getMaxValue(z,frontvar)==1.:
                f = P.frontFaces(z, frontvar)
                if Internal.getZoneDim(f)[1]>0:
                    Internal._rmNodesByName(f,'ID_*')
                    front.append(f)
        if dim == 2:
            dmin = C.getMaxValue(front, 'TurbulentDistance')

            if Cmpi.KCOMM is not None:
                dmin = numpy.array([dmin])
                dmin_max = numpy.zeros(1)
                Cmpi.KCOMM.Allreduce(dmin, dmin_max, Cmpi.MAX)
                dmin = dmin_max[0]

            print("before : {}".format(dmin*SHIFTD*1.125))
            if frontType == 42: dmin += SHIFTB
            print('after : {}'.format(dmin*SHIFTD))
            # Creation du corps 2D pour le preprocessing IBC
            tcl = T.addkplane(tc)
            T._translate(tcl,(0,0,-zmean))
            T._contract(tcl, (0,0,0), (1,0,0), (0,1,0), dz)
            front = P.isoSurfMC(tcl,'TurbulentDistance',dmin*SHIFTD)
            del tcl
        else:
            dmin = C.getMaxValue(front, 'TurbulentDistance')

            if Cmpi.KCOMM is not None:
                dmin = numpy.array([dmin])
                dmin_max = numpy.zeros(1)
                Cmpi.KCOMM.Allreduce(dmin, dmin_max, Cmpi.MAX)
                dmin = dmin_max[0]

            print("before : {}".format(dmin*SHIFTD*1.125))
            if frontType == 42: dmin += SHIFTB
            print('after : {}'.format(dmin*SHIFTD))
            front = P.isoSurfMC(tc, 'TurbulentDistance', dmin*SHIFTD)
    return front


#=============================================================================
# Performs specific treatment for frontType=2
# Modify/add target points in matching boundary regions
#=============================================================================
def _pushBackImageFront2(t, tc, tbb, interpDataType=1):
    """Modify/add target points in matching boundary regions for frontType=2."""
    intersectionsDict = X.getIntersectingDomains(tbb, method='AABB', taabb=tbb)
    C._initVars(t,'{centers:cellNFront_origin}={centers:cellNFront}')
    C._initVars(t,'{centers:cellNIBC_origin}={centers:cellNIBC}')
    C._cpVars(t,'centers:cellNFront',tc,'cellNFront')
    C._cpVars(t,'centers:cellNFront_origin',tc,'cellNFront_origin')
    C._cpVars(t,'centers:cellNIBC_origin',tc,'cellNIBC_origin')

    C._initVars(t,'{centers:cellNFront2}=1.-({centers:cellNFront}<1.)*(abs({centers:cellNChim})>1.)')

    for z in Internal.getZones(t):
        cellNFront = Internal.getNodeFromName2(z,'cellNFront2')
        if cellNFront != []:
            cellNFront = cellNFront[1]
            sizeTot = cellNFront.shape[0]*cellNFront.shape[1]*cellNFront.shape[2]
            sizeOne =  int(numpy.sum(cellNFront))
            if sizeOne < sizeTot:
                X._setHoleInterpolatedPoints(z, depth=1, dir=0, loc='centers',cellNName='cellNFront2',addGC=False)
                res = X.getInterpolatedPoints(z,loc='centers', cellNName='cellNFront2') #indices,X,Y,Z
                if res is not None:
                    indicesI = res[0]
                    XI = res[1]; YI = res[2]; ZI = res[3]
                    allInterpFields=[]
                    for zc in Internal.getZones(tc):
                        if zc[0] in intersectionsDict[z[0]]:
                            if interpDataType==1: HOOKADT = C.createHook(zc, 'adt')
                            else: HOOKADT = None
                            fields = X.transferFields(zc, XI, YI, ZI, hook=HOOKADT, variables=['cellNFront_origin','cellNIBC_origin'], interpDataType=interpDataType)
                            allInterpFields.append(fields)
                            if interpDataType == 1: C.freeHook(HOOKADT)
                    if allInterpFields != []:
                        C._filterPartialFields(z, allInterpFields, indicesI, loc='centers', startFrom=0, filterName='donorVol',verbose=False)
                        # C._initVars(z,'{centers:cellNFront}=({centers:cellNFront}>0.5)') #ancienne version
                        C._initVars(z,'{centers:cellNFront}={centers:cellNFront}*({centers:cellNFront_origin}>0.5)') # Modification du Front uniquement lorsque celui-ci est repousse
                        # i.e. if cellNFront_origin == 0 and cellNFront == 1 => cellNfront = 0

                        C._initVars(z,'{centers:cellNIBC}={centers:cellNIBC}*(1.-({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)) \
                            + 2.*({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)')
                        # i.e. if cellNChim == 1 and cellNIBC_origin == 2 => cellNIBC = 2

                        C._initVars(z,'{centers:cellNIBCDnr}={centers:cellNIBCDnr}*(1.-({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)) \
                            + 2.*({centers:cellNChim}==1.)*({centers:cellNIBC_origin}>1.5)*({centers:cellNIBC_origin}<2.5)')
                        # le front donneur doit etre egalement maj sur l'exemple du champ cellNIBC

        C._rmVars(z,['centers:cellNFront2'])
        C._rmVars(z,['centers:cellNFront_origin'])
        C._rmVars(z,['centers:cellNIBC_origin'])

    C._cpVars(t,'centers:cellNIBC',tc,'cellNIBC')
    C._cpVars(t,'centers:cellNIBC',t,'centers:cellN')
    C._cpVars(t,'centers:cellN',tc,'cellN')

    return None

#=============================================================================
# Performs smoothing after _pushBackImageFront2
# Only for 2D
#=============================================================================
def _smoothImageFront(t, tc, dimPb=2):
    """Add target points to smooth the front."""
    for z in Internal.getZones(t):
        cellNFront  = Internal.getNodeFromName(z,'cellNFront')
        cellNIBC    = Internal.getNodeFromName(z,'cellNIBC')
        cellNIBCDnr = Internal.getNodeFromName(z,'cellNIBCDnr')
        dim         = Internal.getZoneDim(z)
        k           = 0
        if cellNIBC != []:
                cellNIBC = cellNIBC[1]
                sizeTot = cellNIBC.shape[0]*cellNIBC.shape[1]*cellNIBC.shape[2]
                sizeOne =  int(numpy.sum(cellNIBC))
                # if sizeOne < sizeTot:
                cellNFront  = cellNFront[1]
                cellNIBCDnr = cellNIBCDnr[1]
                for i in range(1, int(dim[1])-2):
                    for j in range(1, int(dim[2])-2):
                        if cellNIBC[i,j,k] == 1 and cellNIBC[i,j-1,k] == 2:
                            if cellNIBC[i-1,j+1,k] == 2:
                                cellNFront[i,j,k]    = 0
                                cellNIBC[i,j,k]      = 2
                                cellNIBCDnr[i,j,k]   = 2

                    for j in range(int(dim[2])-3, 0, -1):
                        if cellNIBC[i,j,k] == 1 and cellNIBC[i,j+1,k] == 2:
                            if cellNIBC[i-1,j-1,k] == 2:
                                cellNFront[i,j,k]    = 0
                                cellNIBC[i,j,k]      = 2
                                cellNIBCDnr[i,j,k]   = 2

                for i in range(int(dim[1])-3, 0, -1):
                    for j in range(1, int(dim[2])-2):
                        if cellNIBC[i,j,k] == 1 and cellNIBC[i,j-1,k] == 2:
                            if cellNIBC[i+1,j+1,k] == 2:
                                cellNFront[i,j,k]    = 0
                                cellNIBC[i,j,k]      = 2
                                cellNIBCDnr[i,j,k]   = 2

                    for j in range(int(dim[2])-3, 0, -1):
                        if cellNIBC[i,j,k] == 1 and cellNIBC[i,j+1,k] == 2:
                            if cellNIBC[i+1,j-1,k] == 2:
                                cellNFront[i,j,k]    = 0
                                cellNIBC[i,j,k]      = 2
                                cellNIBCDnr[i,j,k]   = 2

    C._cpVars(t,'centers:cellNIBC',tc,'cellNIBC')
    C._cpVars(t,'centers:cellNIBC',t,'centers:cellN')
    C._cpVars(t,'centers:cellN',tc,'cellN')

    return None


#=============================================================================
# Gather front
# Si un front est calcule par morceau sur chaque proc, ramene le meme front
# sur tous les procs
#=============================================================================
def gatherFront(front):
    """Gather front for mpi."""
    import Converter.Mpi as Cmpi
    zones = Internal.getNodesFromType1(front, 'Zone_t')
    for z in zones: z[0] += '_'+str(Cmpi.rank)

    if Cmpi.KCOMM is not None:
        #allFront = Cmpi.KCOMM.allgather(front)
        #front = []
        #for f in allFront: front += f
        front = Cmpi.allgatherZones(front)
        return front
    else: return front


#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
##INTERPOLATIONS
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# setInterpDataIBM, modifie tc
# IN: tbb: bboundingbox tree
# IN: tb: immersed bodies
# IN: typeI: type of Connectivity to do (ID=Chimere; IBCD=IBM)
# IN: dim: dimension
# IN: dictofADT: dictionary of alternative directivity tree (?)
# IN: front: front of upon which IBM image points are placed
# IN: frontType: type of front= 0,1,42
# IN: depth : depth of overlaps & some IBM points
# IN: IBCType: type of IBM method: -1 points inside solid; 1 point in fluid
# IN: interDataType: type of interpolation beteween zones: 0=Cartesian; 1=Chimere
# IN: Reynolds: Reynolds Number. Need as this will be used to get the modelisation height for frontType 42
# IN: yplus: target y+. Need as this will be used to get the modelisation height for frontType 42
# IN: Lref: reference length: Need as this will be used to get the modelisation height for frontType 42
# IN: isLBM: is the case LBM?
# IN: isIbmAle: are the IBMs moving? 
# Note [AJ]: isIbmAle : set to false - will be used in subsequent commits for moving IBMs
def doInterp(t, tc, tbb, tb=None, typeI='ID', dim=3, dictOfADT=None, front=None, frontType=0, depth=2, IBCType=1, interpDataType=1, Reynolds=6.e6, yplus=100., Lref=1., isLBM=False,isIbmAle=False):
    ReferenceState = Internal.getNodeFromType2(t, 'ReferenceState_t')
    if typeI == 'ID':
        # toutes les zones sont interpolables en Chimere
        intersectionsDict = X.getIntersectingDomains(tbb, method='AABB', taabb=tbb)
        rcvZones = []
        for zrcv in Internal.getZones(t):
            if C.getMaxValue(zrcv,'centers:cellN')==2.:
                zrcvname = zrcv[0]; rcvZones.append(zrcv)
        nozr = 0
        nbZonesChim = len(rcvZones)
        for nozr in range(nbZonesChim):
            zrcv = rcvZones[nozr]
            zrcvname = zrcv[0]
            nozr += 1; hook0 = []
            nobOfDnrBases = []; nobOfDnrZones=[]; dnrZones=[]
            for nobd in range(len(tc[2])):
                if tc[2][nobd][3] == 'CGNSBase_t':
                    for nozd in range(len(tc[2][nobd][2])):
                        zdnr = tc[2][nobd][2][nozd]
                        if zdnr[3] == 'Zone_t':
                            zdnrname = zdnr[0]
                            if zdnrname in intersectionsDict[zrcvname]:
                                nobOfDnrBases.append(nobd)
                                nobOfDnrZones.append(nozd)
                                dnrZones.append(zdnr)
                                if interpDataType==1 and dictOfADT is not None:
                                    hook0.append(dictOfADT[zdnrname])
            if interpDataType == 0: hook0 = None

            X._setInterpData(zrcv, dnrZones, nature=1,penalty=1,loc='centers',storage='inverse',sameName=1,\
                             interpDataType=interpDataType, itype='chimera')
            for nod in range(len(dnrZones)):
                nobd = nobOfDnrBases[nod]
                nozd = nobOfDnrZones[nod]
                tc[2][nobd][2][nozd] = dnrZones[nod]

    elif typeI == 'IBCD':
        # detection des zones IBC
        zonesRIBC = []
        for zrcv in Internal.getZones(t):
            if C.getMaxValue(zrcv,'centers:cellNIBC')==2.:
                zrcvname = zrcv[0]; zonesRIBC.append(zrcv)

        if zonesRIBC == []: return tc

        res = getAllIBMPoints(zonesRIBC, loc='centers', tb=tb, tfront=front, frontType=frontType, \
                              cellNName='cellNIBC', depth=depth, IBCType=IBCType, Reynolds=Reynolds, yplus=yplus, Lref=Lref, isLBM=isLBM)
        nbZonesIBC = len(zonesRIBC)
        dictOfADT = {}
        dictOfCorrectedPtsByIBCType = res[0]
        dictOfWallPtsByIBCType = res[1]
        dictOfInterpPtsByIBCType = res[2]
        for ibcTypeL in  dictOfCorrectedPtsByIBCType:
            allCorrectedPts = dictOfCorrectedPtsByIBCType[ibcTypeL]
            allWallPts = dictOfWallPtsByIBCType[ibcTypeL]
            allInterpPts = dictOfInterpPtsByIBCType[ibcTypeL]
            for nozr in range(nbZonesIBC):
                if allCorrectedPts[nozr] != []:
                    interpPtsBB=Generator.BB(allInterpPts[nozr])
                    zrcv = zonesRIBC[nozr]
                    zrcvname = zrcv[0]
                    nobOfDnrBases = []; nobOfDnrZones=[]; dnrZones=[]
                    if interpDataType == 1: hook0 = []
                    else: hook0 = None
                    for nobd in range(len(tc[2])):
                        if tc[2][nobd][3] == 'CGNSBase_t':
                            for nozd in range(len(tc[2][nobd][2])):
                                zdnr = tc[2][nobd][2][nozd]
                                if zdnr[3] == 'Zone_t':
                                    zdnrname = zdnr[0]
                                    zbb = tbb[2][nobd][2][nozd]
                                    bba = C.getFields(Internal.__GridCoordinates__,zbb)[0]
                                    if Generator.bboxIntersection(interpPtsBB,bba,isBB=True) == 1:
                                        if interpDataType == 1:
                                            if zdnrname not in dictOfADT:
                                                HOOKADT = C.createHook(zdnr, 'adt')
                                                dictOfADT[zdnrname] = HOOKADT
                                            hook0.append(dictOfADT[zdnrname])

                                        dnrZones.append(zdnr)
                                        nobOfDnrBases.append(nobd)
                                        nobOfDnrZones.append(nozd)

                    XOD._setIBCDataForZone__(zrcv, dnrZones, allCorrectedPts[nozr], allWallPts[nozr], allInterpPts[nozr], \
                                             nature=1, penalty=1, loc='centers', storage='inverse',
                                             interpDataType=interpDataType, hook=hook0, dim=dim, \
                                             ReferenceState=ReferenceState, bcType=ibcTypeL)
                    nozr += 1
                    for nod in range(len(dnrZones)):
                        nobd = nobOfDnrBases[nod]
                        nozd = nobOfDnrZones[nod]
                        tc[2][nobd][2][nozd] = dnrZones[nod]

        if dictOfADT is not None:
            for dnrname in dictOfADT: C.freeHook(dictOfADT[dnrname])

    return tc

#=============================================================================
# Version pour l'explicite local
def doInterp2(t, tc, tbb, tb=None, typeI='ID', dim=3, dictOfADT=None, front=None, frontType=0, depth=2, IBCType=1, interpDataType=1, Reynolds=6.e6, yplus=100., Lref=1.):
    ReferenceState = Internal.getNodeFromType2(t, 'ReferenceState_t')

    bases  = Internal.getNodesFromType1(t     , 'CGNSBase_t')
    dimmm  = Internal.getNodeFromName2(bases[0], 'EquationDimension')
    dimPb   = Internal.getValue(dimmm)
    dxmax = 0.0


    zones = Internal.getZones(t)

    dico_dx = {}
    dico_dy = {}
    dico_dz = {}

    for z in zones:
        nodes = Internal.getNodesFromName(z, 'GridCoordinates')
        coordx = nodes[0][2][0][1]
        coordy = nodes[0][2][1][1]
        coordz = nodes[0][2][2][1]

        dxx  = abs(coordx[1,0,0]   - coordx[0,0,0])
        dyy  = abs(coordy[0,1,0]   - coordy[0,0,0])
        dzz  = abs(coordz[0,0,1]   - coordz[0,0,0])

        dico_dx[z[0]] = dxx
        dico_dy[z[0]] = dyy
        if dimPb == 2:dico_dz[z[0]] = 1
        else : dico_dz[z[0]] = dzz

        if dimPb == 2:dzz=max(dxx,dyy)

        dx = min(dxx,dyy,dzz)
        if dx > dxmax:dxmax=dx

    niveaux_temps = {}
    cx = {}

    for z in zones:
        nodes = Internal.getNodesFromName(z, 'GridCoordinates')
        coordx = nodes[0][2][0][1]
        coordy = nodes[0][2][1][1]
        coordz = nodes[0][2][2][1]

        dxx  = abs(coordx[1,0,0]   - coordx[0,0,0])
        dyy  = abs(coordy[0,1,0]   - coordy[0,0,0])
        dzz  = abs(coordz[0,0,1]   - coordz[0,0,0])

        if dimPb == 2:dzz=max(dxx,dyy)


        dx = min(dxx,dyy,dzz)

        #cx[z[0]]= coordx[1,0,0]

        N = math.log(dxmax/dx)/math.log(2.0)
        N = round(N) - 2
        if N < 0:
            niveaux_temps[z[0]] = 2**0
        else :
            niveaux_temps[z[0]] = 2**N
        ##if (N < 6):N=0
        ##else:N=1
        #if (cx[z[0]] < 0.98): niveaux_temps[z[0]] = 2**N
        #else:  niveaux_temps[z[0]] = 1
        #niveaux_temps['cart.264'] = 2
        #niveaux_temps['cart.294'] = 2

        #niveaux_temps[z[0]] = 2**N

        print(niveaux_temps[z[0]])
        #print(round(dxmax/dx))


    if typeI == 'ID':
        # toutes les zones sont interpolables en Chimere
        intersectionsDict = X.getIntersectingDomains(tbb, method='AABB', taabb=tbb)
        rcvZones = []
        for zrcv in Internal.getZones(t):
            if C.getMaxValue(zrcv,'centers:cellN')==2.:
                zrcvname = zrcv[0]; rcvZones.append(zrcv)
        nozr = 0
        nbZonesChim = len(rcvZones)
        for nozr in range(nbZonesChim):
            zrcv = rcvZones[nozr]
            zrcvname = zrcv[0]
            nozr += 1; hook0 = []
            nobOfDnrBases = []; nobOfDnrZones=[]; dnrZones=[]
            for nobd in range(len(tc[2])):
                if tc[2][nobd][3] == 'CGNSBase_t':
                    for nozd in range(len(tc[2][nobd][2])):
                        zdnr = tc[2][nobd][2][nozd]
                        if zdnr[3] == 'Zone_t':
                            zdnrname = zdnr[0]
                            if zdnrname in intersectionsDict[zrcvname]:
                                nobOfDnrBases.append(nobd)
                                nobOfDnrZones.append(nozd)
                                dnrZones.append(zdnr)
                                if interpDataType==1 and dictOfADT is not None:
                                    hook0.append(dictOfADT[zdnrname])
            if interpDataType == 0: hook0 = None

            X._setInterpData(zrcv, dnrZones, nature=1,penalty=1,loc='centers',storage='inverse',sameName=1,\
                             interpDataType=interpDataType, itype='chimera')


            levelrcv = niveaux_temps[zrcv[0]]


            for nod in range(len(dnrZones)):

                print('zdnr= ', dnrZones[nod][0])

                dim__ = Internal.getZoneDim(dnrZones[nod])
                prange = numpy.zeros(6,dtype=numpy.int32)
                prangedonor = numpy.zeros(6,dtype=numpy.int32)
                profondeur=numpy.zeros(1,dtype=numpy.int32)
                dirD=numpy.zeros(1,dtype=numpy.int32)
                dirR=numpy.zeros(1,dtype=numpy.int32)

                plist = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][2][1]
                plistdnr = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][3][1]
                coeff = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][4][1]
                typ = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][5][1]

                leveldnr = niveaux_temps[dnrZones[nod][0]]

                nobd = nobOfDnrBases[nod]
                nozd = nobOfDnrZones[nod]
                tc[2][nobd][2][nozd] = dnrZones[nod]

                prangebis=numpy.reshape(prange,6)

                info = dnrZones[nod][2][len(dnrZones[nod][2])-1]
                info[2].append(['PointRange', prangebis , [], 'IndexArray_t'])

                transfo=numpy.zeros(3,dtype=numpy.int32)#XOD.getTransfo(dnrZones[nod],zrcv)

                connector.indiceToCoord2(plist,prangedonor,transfo,profondeur,dirD,typ,dirR,plist.size,dim__[1]+1,dim__[2]+1,dim__[3]+1)


                #connector.correctCoeffList(plist, coeff, typ, plist.size , dim__[1]+1 , dim__[2]+1 , dim__[3]+1)

                NMratio = numpy.zeros(3,dtype=numpy.int32)
                NMratio[0]=1
                NMratio[1]=1
                NMratio[2]=1

                info[2].append(['PointRangeDonor', prangedonor , [], 'IndexArray_t'])
                info[2].append(['DirReceveur', dirR , [], 'IndexArray_t'])
                info[2].append(['DirDonneur', dirD , [], 'IndexArray_t'])
                info[2].append(['Transform', transfo , [], 'IndexArray_t'])
                info[2].append(['Profondeur', profondeur , [], 'IndexArray_t'])
                info[2].append(['PointPivot', transfo , [], 'IndexArray_t'])
                info[2].append(['NMratio', NMratio , [], 'IndexArray_t'])
                info[2].append(['LevelZRcv', levelrcv , [], 'IndexArray_t'])
                info[2].append(['LevelZDnr', leveldnr , [], 'IndexArray_t'])



    elif typeI == 'IBCD':
        # detection des zones IBC
        zonesRIBC = []
        for zrcv in Internal.getZones(t):
            if C.getMaxValue(zrcv,'centers:cellNIBC')==2.:
                zrcvname = zrcv[0]; zonesRIBC.append(zrcv)

        if zonesRIBC == []: return tc

        res = getAllIBMPoints(zonesRIBC, loc='centers',tb=tb, tfront=front, frontType=frontType, \
                              cellNName='cellNIBC', depth=depth, IBCType=IBCType, Reynolds=Reynolds, yplus=yplus, Lref=Lref)
        nbZonesIBC = len(zonesRIBC)
        dictOfADT = {}
        dictOfCorrectedPtsByIBCType = res[0]
        dictOfWallPtsByIBCType = res[1]
        dictOfInterpPtsByIBCType = res[2]
        for ibcTypeL in  dictOfCorrectedPtsByIBCType:
            allCorrectedPts = dictOfCorrectedPtsByIBCType[ibcTypeL]
            allWallPts = dictOfWallPtsByIBCType[ibcTypeL]
            allInterpPts = dictOfInterpPtsByIBCType[ibcTypeL]
            for nozr in range(nbZonesIBC):
                if allCorrectedPts[nozr] != []:
                    interpPtsBB=Generator.BB(allInterpPts[nozr])
                    zrcv = zonesRIBC[nozr]
                    zrcvname = zrcv[0]
                    nobOfDnrBases = []; nobOfDnrZones=[]; dnrZones=[]
                    if interpDataType == 1: hook0 = []
                    else: hook0 = None
                    for nobd in range(len(tc[2])):
                        if tc[2][nobd][3] == 'CGNSBase_t':
                            for nozd in range(len(tc[2][nobd][2])):
                                zdnr = tc[2][nobd][2][nozd]
                                if zdnr[3] == 'Zone_t':
                                    zdnrname = zdnr[0]
                                    zbb = tbb[2][nobd][2][nozd]
                                    bba = C.getFields(Internal.__GridCoordinates__,zbb)[0]
                                    if Generator.bboxIntersection(interpPtsBB,bba,isBB=True) == 1:
                                        if interpDataType == 1:
                                            if zdnrname not in dictOfADT:
                                                HOOKADT = C.createHook(zdnr, 'adt')
                                                dictOfADT[zdnrname] = HOOKADT
                                            hook0.append(dictOfADT[zdnrname])

                                        dnrZones.append(zdnr)
                                        nobOfDnrBases.append(nobd)
                                        nobOfDnrZones.append(nozd)

                    XOD._setIBCDataForZone__(zrcv, dnrZones, allCorrectedPts[nozr], allWallPts[nozr], allInterpPts[nozr], \
                                             nature=1, penalty=1, loc='centers', storage='inverse',
                                             interpDataType=interpDataType, hook=hook0, dim=dim, \
                                             ReferenceState=ReferenceState, bcType=ibcTypeL)

                    nozr += 1

                    levelrcv = niveaux_temps[zrcv[0]]

                    for nod in range(len(dnrZones)):

                        dim__ = Internal.getZoneDim(dnrZones[nod])
                        prange = numpy.zeros(6,dtype=numpy.int32)
                        prangedonor = numpy.zeros(6,dtype=numpy.int32)
                        profondeur=numpy.zeros(1,dtype=numpy.int32)
                        dirD=numpy.zeros(1,dtype=numpy.int32)
                        dirR=numpy.zeros(1,dtype=numpy.int32)

                        plist = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][2][1]
                        plistdnr = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][3][1]
                        coeff = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][4][1]
                        typ = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][5][1]

                        leveldnr = niveaux_temps[dnrZones[nod][0]]

                        nobd = nobOfDnrBases[nod]
                        nozd = nobOfDnrZones[nod]

                        tc[2][nobd][2][nozd] = dnrZones[nod]


                        prangebis=numpy.reshape(prange,6)

                        info = dnrZones[nod][2][len(dnrZones[nod][2])-1]
                        info[2].append(['PointRange', prangebis , [], 'IndexArray_t'])

                        transfo=numpy.zeros(3,dtype=numpy.int32)#XOD.getTransfo(dnrZones[nod],zrcv)

                        connector.indiceToCoord2(plist,prangedonor,transfo,profondeur,dirD,typ,dirR,plist.size,dim__[1]+1,dim__[2]+1,dim__[3]+1)

                        #connector.correctCoeffList(plist, coeff, typ, plist.size , dim__[1]+1 , dim__[2]+1 , dim__[3]+1)

                        NMratio = numpy.zeros(3,dtype=numpy.int32)
                        NMratio[0]=1
                        NMratio[1]=1
                        NMratio[2]=1

                        info[2].append(['PointRangeDonor', prangedonor , [], 'IndexArray_t'])
                        info[2].append(['DirReceveur', dirR , [], 'IndexArray_t'])
                        info[2].append(['DirDonneur', dirD , [], 'IndexArray_t'])
                        info[2].append(['Transform', transfo , [], 'IndexArray_t'])
                        info[2].append(['Profondeur', profondeur , [], 'IndexArray_t'])
                        info[2].append(['PointPivot', transfo , [], 'IndexArray_t'])
                        info[2].append(['NMratio', NMratio , [], 'IndexArray_t'])
                        info[2].append(['LevelZRcv', levelrcv , [], 'IndexArray_t'])
                        info[2].append(['LevelZDnr', leveldnr , [], 'IndexArray_t'])

                        print('LEVELS= ', levelrcv, leveldnr)


        if dictOfADT is not None:
            for dnrname in dictOfADT: C.freeHook(dictOfADT[dnrname])

    return tc

#=============================================================================
def doInterp3(t, tc, tbb, tb=None, typeI='ID', dim=3, dictOfADT=None, frontType=0, depth=2, IBCType=1, interpDataType=1, Reynolds=6.e6, yplus=100., Lref=1.):


    ReferenceState = Internal.getNodeFromType2(t,'ReferenceState_t')

    bases  = Internal.getNodesFromType1(t     , 'CGNSBase_t')
    dimmm  = Internal.getNodeFromName2(bases[0], 'EquationDimension')
    dimPb   = Internal.getValue(dimmm)
    dxmax = 0.0


    zones = Internal.getZones(t)

    dico_dx = {}
    dico_dy = {}
    dico_dz = {}

    for z in zones:
        nodes = Internal.getNodesFromName(z, 'GridCoordinates')
        coordx = nodes[0][2][0][1]
        coordy = nodes[0][2][1][1]
        coordz = nodes[0][2][2][1]

        dxx  = abs(coordx[1,0,0]   - coordx[0,0,0])
        dyy  = abs(coordy[0,1,0]   - coordy[0,0,0])
        dzz  = abs(coordz[0,0,1]   - coordz[0,0,0])

        dico_dx[z[0]] = dxx
        dico_dy[z[0]] = dyy
        if dimPb == 2: dico_dz[z[0]] = 1
        else: dico_dz[z[0]] = dzz

        if dimPb == 2:dzz=max(dxx,dyy)

        dx = min(dxx,dyy,dzz)
        if dx > dxmax:dxmax=dx

    niveaux_temps = {}
    cx = {}

    for z in zones:
        nodes = Internal.getNodesFromName(z, 'GridCoordinates')
        coordx = nodes[0][2][0][1]
        coordy = nodes[0][2][1][1]
        coordz = nodes[0][2][2][1]

        dxx  = abs(coordx[1,0,0]   - coordx[0,0,0])
        dyy  = abs(coordy[0,1,0]   - coordy[0,0,0])
        dzz  = abs(coordz[0,0,1]   - coordz[0,0,0])

        if dimPb == 2:dzz=max(dxx,dyy)

        dx = min(dxx,dyy,dzz)

        #cx[z[0]]= coordx[1,0,0]

        N = math.log(dxmax/dx)/math.log(2.0)
        N = round(N)
        if N < 6: N=0
        else: N=1
        #if (cx[z[0]] < 0.98): niveaux_temps[z[0]] = 2**N
        #else:  niveaux_temps[z[0]] = 1
        #niveaux_temps['cart.264'] = 2
        #niveaux_temps['cart.294'] = 2

        niveaux_temps[z[0]] = 2**N


        print(niveaux_temps[z[0]])
        #print(round(dxmax/dx))


    if typeI == 'ID':
        # toutes les zones sont interpolables en Chimere
        intersectionsDict = X.getIntersectingDomains(tbb, method='AABB', taabb=tbb)


        rcvZones = []
        for zrcv in Internal.getZones(t):
            if C.getMaxValue(zrcv,'centers:cellN')==2.:
               zrcvname = zrcv[0]; rcvZones.append(zrcv)

        #dico={}
        #for zrcv in Internal.getZones(t):
              # listofjoins = Internal.getNodesFromType2(zrcv, 'GridConnectivity_t')
              # if listofjoins is not None:
               #    prange_list=[]
               #    dir_list=[]
               #    for join in listofjoins:
               #        prange_ = Internal.getNodeFromName1(join,'PointRange')[1]
                       #dirR = CGC.getDirBorderStruct__(prange_,dimPb)
                       #dir_list.append(dirR)
                       #print 'prange_= ', prange_
                #       for i in range(3):
                #           if prange_[i,1] == prange_[i,0] and prange_[i,1] != 1:
                #               prange_[i,1] =  prange_[i,1]-1
                #               prange_[i,0] =  prange_[i,0]-1
                #           elif prange_[i,1] != prange_[i,0] and prange_[i,1] != 1 :
                 #              prange_[i,1] =  prange_[i,1]-1
                 #      prange_=numpy.reshape(prange_,6)
                 #      prange_list.append(prange_)
                 #  dico[zrcv[0]]=prange_list
                   #dico[zrcv[0]]=dir_list
                   # print prange_, zrcv[0]


        nozr = 0
        nbZonesChim = len(rcvZones)
        for nozr in range(nbZonesChim):
            zrcv = rcvZones[nozr]
            dim_ = Internal.getZoneDim(zrcv)
            zrcvname = zrcv[0]
            nozr += 1; hook0 = []
            nobOfDnrBases = []; nobOfDnrZones=[]; dnrZones=[]
            for nobd in range(len(tc[2])):
                if tc[2][nobd][3] == 'CGNSBase_t':
                    for nozd in range(len(tc[2][nobd][2])):
                        zdnr = tc[2][nobd][2][nozd]
                        if zdnr[3] == 'Zone_t':
                            zdnrname = zdnr[0]
                            if zdnrname in intersectionsDict[zrcvname]:
                                nobOfDnrBases.append(nobd)
                                nobOfDnrZones.append(nozd)
                                dnrZones.append(zdnr)
                                hook0.append(dictOfADT[zdnrname])

            dnrZones = X.setInterpData(zrcv,dnrZones,nature=1,penalty=1,loc='centers',storage='inverse',sameName=1,\
                                       hook=hook0, itype='chimera')


            levelrcv = niveaux_temps[zrcv[0]]

            for nod in range(len(dnrZones)):

                dim__ = Internal.getZoneDim(dnrZones[nod])
                prange = numpy.zeros(6,dtype=numpy.int32)
                prangedonor = numpy.zeros(6,dtype=numpy.int32)
                profondeur=numpy.zeros(1,dtype=numpy.int32)
                dirD=numpy.zeros(1,dtype=numpy.int32)
                dirR=numpy.zeros(1,dtype=numpy.int32)

                plist = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][2][1]
                plistdnr = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][3][1]
                coeff = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][4][1]
                typ = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][5][1]

                leveldnr = niveaux_temps[dnrZones[nod][0]]

                nobd = nobOfDnrBases[nod]
                nozd = nobOfDnrZones[nod]

                tc[2][nobd][2][nozd] = dnrZones[nod]


                prangebis=numpy.reshape(prange,6)

                info = dnrZones[nod][2][len(dnrZones[nod][2])-1]
                info[2].append(['PointRange', prangebis , [], 'IndexArray_t'])

                transfo = XOD.getTransfo(dnrZones[nod],zrcv)

                connector.indiceToCoord2(plist,prangedonor,transfo,profondeur,dirD,typ,dirR,plist.size,dim__[1]+1,dim__[2]+1,dim__[3]+1)


                #connector.correctCoeffList(plist, coeff, typ, plist.size , dim__[1]+1 , dim__[2]+1 , dim__[3]+1)

                NMratio = numpy.zeros(3,dtype=numpy.int32)
                NMratio[0]=1
                NMratio[1]=1
                NMratio[2]=1

                info[2].append(['PointRangeDonor', prangedonor , [], 'IndexArray_t'])
                info[2].append(['DirReceveur', dirR , [], 'IndexArray_t'])
                info[2].append(['DirDonneur', dirD , [], 'IndexArray_t'])
                info[2].append(['Transform', transfo , [], 'IndexArray_t'])
                info[2].append(['Profondeur', profondeur , [], 'IndexArray_t'])
                info[2].append(['PointPivot', transfo , [], 'IndexArray_t'])
                info[2].append(['NMratio', NMratio , [], 'IndexArray_t'])
                info[2].append(['LevelZRcv', levelrcv , [], 'IndexArray_t'])
                info[2].append(['LevelZDnr', leveldnr , [], 'IndexArray_t'])



    elif typeI == 'IBCD':
        # detection des zones IBC
        zonesRIBC = []
        for zrcv in Internal.getZones(t):
            if C.getMaxValue(zrcv,'centers:cellNIBC')==2.:
                zrcvname = zrcv[0]; zonesRIBC.append(zrcv)

        if zonesRIBC == []: return tc

        print('Building the IBM front.')

        front = getIBMFront(tc, 'cellNFront', dim, frontType)
        # Sortie du front pour debug
        C.convertPyTree2File(front, 'front.cgns')

        res = getAllIBMPoints(zonesRIBC, loc='centers',tb=tb, tfront=front, frontType=frontType, \
                              cellNName='cellNIBC', depth=depth, IBCType=IBCType, Reynolds=Reynolds, yplus=yplus, Lref=Lref)
        nbZonesIBC = len(zonesRIBC)
        dictOfADT = {}
        dictOfCorrectedPtsByIBCType = res[0]
        dictOfWallPtsByIBCType = res[1]
        dictOfInterpPtsByIBCType = res[2]
        for ibcTypeL in dictOfCorrectedPtsByIBCType:
            allCorrectedPts = dictOfCorrectedPtsByIBCType[ibcTypeL]
            allWallPts = dictOfWallPtsByIBCType[ibcTypeL]
            allInterpPts = dictOfInterpPtsByIBCType[ibcTypeL]
            for nozr in range(nbZonesIBC):
                if allCorrectedPts[nozr] != []:
                    interpPtsBB=Generator.BB(allInterpPts[nozr])

                    zrcv = zonesRIBC[nozr]
                    zrcvname = zrcv[0]
                    nobOfDnrBases = []; nobOfDnrZones=[]; dnrZones=[]; hook0 = []
                    for nobd in range(len(tc[2])):
                        if tc[2][nobd][3] == 'CGNSBase_t':
                            for nozd in range(len(tc[2][nobd][2])):
                                zdnr = tc[2][nobd][2][nozd]
                                if zdnr[3] == 'Zone_t':
                                    zdnrname = zdnr[0]
                                    zbb = tbb[2][nobd][2][nozd]
                                    bba = C.getFields(Internal.__GridCoordinates__,zbb)[0]
                                    if Generator.bboxIntersection(interpPtsBB,bba,isBB=True) == 1:
                                        if zdnrname not in dictOfADT:
                                            HOOKADT = C.createHook(zdnr, 'adt')
                                            dictOfADT[zdnrname] = HOOKADT
                                        dnrZones.append(zdnr)
                                        hook0.append(dictOfADT[zdnrname])
                                        nobOfDnrBases.append(nobd)
                                        nobOfDnrZones.append(nozd)

                    XOD._setIBCDataForZone__(zrcv, dnrZones, allCorrectedPts[nozr], allWallPts[nozr], allInterpPts[nozr], \
                                             loc='centers', storage='inverse',  hook=hook0, dim=dim, ReferenceState=ReferenceState, bcType=ibcTypeL)
                    nozr += 1

                    levelrcv = niveaux_temps[zrcv[0]]

                    for nod in range(len(dnrZones)):

                        dim__ = Internal.getZoneDim(dnrZones[nod])
                        prange = numpy.zeros(6,dtype=numpy.int32)
                        prangedonor = numpy.zeros(6,dtype=numpy.int32)
                        profondeur=numpy.zeros(1,dtype=numpy.int32)
                        dirD=numpy.zeros(1,dtype=numpy.int32)
                        dirR=numpy.zeros(1,dtype=numpy.int32)

                        plist = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][2][1]
                        plistdnr = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][3][1]
                        coeff = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][4][1]
                        typ = dnrZones[nod][2][len(dnrZones[nod][2])-1][2][5][1]

                        leveldnr = niveaux_temps[dnrZones[nod][0]]

                        nobd = nobOfDnrBases[nod]
                        nozd = nobOfDnrZones[nod]

                        tc[2][nobd][2][nozd] = dnrZones[nod]


                        prangebis=numpy.reshape(prange,6)

                        info = dnrZones[nod][2][len(dnrZones[nod][2])-1]
                        info[2].append(['PointRange', prangebis , [], 'IndexArray_t'])

                        transfo=XOD.getTransfo(dnrZones[nod],zrcv)

                        connector.indiceToCoord2(plist,prangedonor,transfo,profondeur,dirD,typ,dirR,plist.size,dim__[1]+1,dim__[2]+1,dim__[3]+1)

                        #connector.correctCoeffList(plist, coeff, typ, plist.size , dim__[1]+1 , dim__[2]+1 , dim__[3]+1)

                        NMratio = numpy.zeros(3,dtype=numpy.int32)
                        NMratio[0]=1
                        NMratio[1]=1
                        NMratio[2]=1

                        info[2].append(['PointRangeDonor', prangedonor , [], 'IndexArray_t'])
                        info[2].append(['DirReceveur', dirR , [], 'IndexArray_t'])
                        info[2].append(['DirDonneur', dirD , [], 'IndexArray_t'])
                        info[2].append(['Transform', transfo , [], 'IndexArray_t'])
                        info[2].append(['Profondeur', profondeur , [], 'IndexArray_t'])
                        info[2].append(['PointPivot', transfo , [], 'IndexArray_t'])
                        info[2].append(['NMratio', NMratio , [], 'IndexArray_t'])
                        info[2].append(['LevelZRcv', levelrcv , [], 'IndexArray_t'])
                        info[2].append(['LevelZDnr', leveldnr , [], 'IndexArray_t'])

                        print('LEVELS= ', levelrcv, leveldnr)



        for dnrname in dictOfADT.keys(): C.freeHook(dictOfADT[dnrname])

        for dnrname in dictOfADT: C.freeHook(dictOfADT[dnrname])

    return tc



#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
## IBM INFO
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# For FastLBM with IBMs. As FastLBM/IBC will evolve making this function decrepit
# no additional information will be currently added [May 12, 2023].
# Get total number of IBM points & puts in param_int. Need to FastLBM/IBC that
# applies the IBMs on the distribution functions.
def _extractIBMInfo_param(t, tc):
    """Extract IBM informations from tc."""
    XPC   ={}
    Zones = []
    for z in Internal.getZones(tc):
        allIBCD = Internal.getNodesFromName(z, "IBCD_*")
        for IBCD in allIBCD:
            znames = Internal.getValue(IBCD)
            Zones.append(znames)
            xPC = Internal.getNodesFromName(IBCD,"CoordinateX_PC")[0][1]

            if znames in XPC:
                a = numpy.concatenate((XPC[znames][0],xPC))
                XPC[znames] = [a]
            else:
                XPC[znames] = [xPC]
    Zones = list(set(Zones))
    for zname in Zones:
        xPC = XPC[zname];
        size = xPC[0].shape[0]
        z          = Internal.getNodeFromName ( t, zname)
        o          = Internal.getNodeFromName1( z, '.Solver#ownData')
        param_int  = Internal.getNodeFromName1( o, 'Parameter_int')
        param_real = Internal.getNodeFromName1( o, 'Parameter_real')
        param_int[1][LBM_IBC_NUM] = size
    return None


#=============================================================================
# Extraction des pts IBM: retourne un arbre avec les coordonnees des
# pts IBM a corriger, paroi, miroirs
#=============================================================================
def extractIBMInfo(tc, IBCNames="IBCD_*"):
    """Extract IBM informations in a pyTree."""
    XPC={}; YPC={}; ZPC={}
    XPW={}; YPW={}; ZPW={}
    XPI={}; YPI={}; ZPI={}
    Zones = []
    for z in Internal.getZones(tc):
        allIBCD = Internal.getNodesFromName(z, IBCNames)
        for IBCD in allIBCD:
            znames = Internal.getValue(IBCD)
            Zones.append(znames)
            xPC = Internal.getNodesFromName(IBCD,"CoordinateX_PC")[0][1]
            yPC = Internal.getNodesFromName(IBCD,"CoordinateY_PC")[0][1]
            zPC = Internal.getNodesFromName(IBCD,"CoordinateZ_PC")[0][1]
            xPI = Internal.getNodesFromName(IBCD,"CoordinateX_PI")[0][1]
            yPI = Internal.getNodesFromName(IBCD,"CoordinateY_PI")[0][1]
            zPI = Internal.getNodesFromName(IBCD,"CoordinateZ_PI")[0][1]
            xPW = Internal.getNodesFromName(IBCD,"CoordinateX_PW")[0][1]
            yPW = Internal.getNodesFromName(IBCD,"CoordinateY_PW")[0][1]
            zPW = Internal.getNodesFromName(IBCD,"CoordinateZ_PW")[0][1]
            if znames in XPW:
                a = numpy.concatenate((XPW[znames][0],xPW))
                XPW[znames] = [a]
                a = numpy.concatenate((YPW[znames][0],yPW))
                YPW[znames] = [a]
                a = numpy.concatenate((ZPW[znames][0],zPW))
                ZPW[znames] = [a]
                a = numpy.concatenate((XPI[znames][0],xPI))
                XPI[znames] = [a]
                a = numpy.concatenate((YPI[znames][0],yPI))
                YPI[znames] = [a]
                a = numpy.concatenate((ZPI[znames][0],zPI))
                ZPI[znames] = [a]
                a = numpy.concatenate((XPC[znames][0],xPC))
                XPC[znames] = [a]
                a = numpy.concatenate((YPC[znames][0],yPC))
                YPC[znames] = [a]
                a = numpy.concatenate((ZPC[znames][0],zPC))
                ZPC[znames] = [a]

            else:
                XPW[znames] = [xPW]
                YPW[znames] = [yPW]
                ZPW[znames] = [zPW]
                XPI[znames] = [xPI]
                YPI[znames] = [yPI]
                ZPI[znames] = [zPI]
                XPC[znames] = [xPC]
                YPC[znames] = [yPC]
                ZPC[znames] = [zPC]
    Zones = list(set(Zones))
    corrected = []; wall = []; interp = []
    t = C.newPyTree(['IBM','Wall','Image'])
    for zname in Zones:
        xPC = XPC[zname]; yPC = YPC[zname]; zPC = ZPC[zname]
        size = xPC[0].shape[0]
        coordxPC = ['CoordinateX',xPC[0],[],'DataArray_t']
        coordyPC = ['CoordinateY',yPC[0],[],'DataArray_t']
        coordzPC = ['CoordinateZ',zPC[0],[],'DataArray_t']
        zone = G.cart((0,0,0),(1,1,1),(size,1,1))
        zone[0] = 'correctedPts_'+zname

        XPC0 = Internal.getNodeFromName(zone,'CoordinateX')
        parent,d = Internal.getParentOfNode(zone, XPC0)
        parent[2][d] = coordxPC

        YPC0 = Internal.getNodeFromName(zone,'CoordinateY')
        parent,d = Internal.getParentOfNode(zone, YPC0)
        parent[2][d] = coordyPC

        ZPC0 = Internal.getNodeFromName(zone,'CoordinateZ')
        parent,d = Internal.getParentOfNode(zone, ZPC0)
        parent[2][d] = coordzPC
        corrected.append(zone)
        #
        xPI = XPI[zname]; yPI = YPI[zname]; zPI = ZPI[zname]
        size = xPI[0].shape[0]
        coordxPI = ['CoordinateX',xPI[0],[],'DataArray_t']
        coordyPI = ['CoordinateY',yPI[0],[],'DataArray_t']
        coordzPI = ['CoordinateZ',zPI[0],[],'DataArray_t']
        zone = G.cart((0,0,0),(1,1,1),(size,1,1))
        zone[0] = 'interpPts_'+zname

        XPI0 = Internal.getNodeFromName(zone,'CoordinateX')
        parent,d = Internal.getParentOfNode(zone, XPI0)
        parent[2][d] = coordxPI

        YPI0 = Internal.getNodeFromName(zone,'CoordinateY')
        parent,d = Internal.getParentOfNode(zone, YPI0)
        parent[2][d] = coordyPI

        ZPI0 = Internal.getNodeFromName(zone,'CoordinateZ')
        parent,d = Internal.getParentOfNode(zone, ZPI0)
        parent[2][d] = coordzPI
        interp.append(zone)

        xPW = XPW[zname];yPW = YPW[zname];zPW = ZPW[zname]
        size = xPW[0].shape[0]
        coordxPW = ['CoordinateX',xPW[0],[],'DataArray_t']
        coordyPW = ['CoordinateY',yPW[0],[],'DataArray_t']
        coordzPW = ['CoordinateZ',zPW[0],[],'DataArray_t']
        zone = G.cart((0,0,0),(1,1,1),(size,1,1))
        zone[0] = 'wallPts_'+zname

        XPW0 = Internal.getNodeFromName(zone,'CoordinateX')
        parent,d = Internal.getParentOfNode(zone, XPW0)
        parent[2][d] = coordxPW

        YPW0 = Internal.getNodeFromName(zone,'CoordinateY')
        parent,d = Internal.getParentOfNode(zone, YPW0)
        parent[2][d] = coordyPW

        ZPW0 = Internal.getNodeFromName(zone,'CoordinateZ')
        parent,d = Internal.getParentOfNode(zone, ZPW0)
        parent[2][d] = coordzPW
        wall.append(zone)

    t[2][1][2] = corrected; t[2][2][2] = wall; t[2][3][2] = interp
    t = C.convertArray2Node(t)

    # Avoid that two procs write the same information
    if Cmpi.size > 1:
        for z in Internal.getZones(t):
            if int(z[0][-1]) != Cmpi.rank:
                z[0] = z[0]+"%{}".format(Cmpi.rank)

    return t


# =============================================================================
# Compute the IBM points: wall, image, & target
# IN: loc: "centers" or "nodes"
# IN: hi:NOT SURE BUT USED WHEN tb IS NOT PROVIDED.
# IN: he:NOT SURE BUT USED WHEN tb IS NOT PROVIDED.
# IN: tb: immersed bodies
# IN: tFront: IBM front for first set of image points (always needed)
# IN: tFront2: IBM front for second set of image points
# IN: frontType: type of front= 0,1,42
# IN: cellNName: name of cellN used to get IBM point.
# IN: IBCType: type of IBM method: -1 points inside solid; 1 point in fluid
# IN: depth : depth of overlaps & some IBM points
# IN: Reynolds: Reynolds Number. Need as this will be used to get the modelisation height for frontType 42
# IN: yplus: target y+. Need as this will be used to get the modelisation height for frontType 42
# IN: Lref: reference length: Need as this will be used to get the modelisation height for frontType 42
# IN: hmod: [AJ] not sure. It is not used. see [BC].
# IN: isLBM: is it an LBM run?
# IN: isWireModel: is the Wire Mesh Model used?
# IN: isOrthoFirst: Apply orthogonal projection first and level set normal decscent second. Needed for IBC filaments only.
# =============================================================================
def getAllIBMPoints(t, loc='nodes', hi=0., he=0., tb=None, tfront=None, tfront2=None, frontType=0, cellNName='cellN', IBCType=1, depth=2, Reynolds=6.e6, yplus=100., Lref=1., hmod=0.1, isLBM=False,isWireModel=False, isOrthoFirst=False):
    """Returns the dictionary of IBM points."""
    if IBCType == -1: signOfDistCorrected = -1
    else: signOfDistCorrected=1 # signe de la distance aux points corriges

    allCorrectedPts = []; allWallPts = []; allInterpPts = []
    #allInterpPts_layer2 = []; # [AJ] Keep for now
    #-------------------------------------------
    # 1. Get the list of IBC corrected pts
    #-------------------------------------------
    listOfSnearsLoc=[]
    listOfModelisationHeightsLoc = []
    if loc == 'nodes':
        for z in Internal.getZones(t):
            an = C.getFields(Internal.__GridCoordinates__,z)[0]
            ac1 = C.getField(cellNName,z)[0]
            ac1[0] = 'cellN'
            ac2 = C.getField('TurbulentDistance',z)[0]
            ac3 = C.getField('gradxTurbulentDistance',z)[0]
            ac4 = C.getField('gradyTurbulentDistance',z)[0]
            ac5 = C.getField('gradzTurbulentDistance',z)[0]
            an = Converter.addVars([an,ac1,ac2,ac3,ac4,ac5])
            ah = C.getField('hi',z)[0]
            if ah != []: an = Converter.addVars([an,ah])
            ah = C.getField('he',z)[0]
            if ah != []: an = Converter.addVars([an,ah])
            correctedPts = Connector.getInterpolatedPoints__(an)
            xt = C.getField('CoordinateX',z)[0][1][0]
            snearl = xt[1]-xt[0]
            listOfSnearsLoc.append(snearl)
            allCorrectedPts.append(correctedPts)
            if frontType == 42: listOfModelisationHeightsLoc.append(G_IBM_Height.computeModelisationHeight(Re=Reynolds, yplus=yplus, L=Lref))
            else:
                listOfModelisationHeightsLoc.append(0.)
                # if tfront2 is not None:
                #     listOfModelisationHeightsLoc.append(hmod)
                # else:
                #     listOfModelisationHeightsLoc.append(0.)
    else:        
        for z in Internal.getZones(t):
            an = C.getFields(Internal.__GridCoordinates__,z)[0]
            an = Converter.node2Center(an)
            ac1 = C.getField('centers:'+cellNName,z)[0]
            ac1[0] = 'cellN'
            ac2 = C.getField('centers:TurbulentDistance',z)[0]
            ac3 = C.getField('centers:gradxTurbulentDistance',z)[0]
            ac4 = C.getField('centers:gradyTurbulentDistance',z)[0]
            ac5 = C.getField('centers:gradzTurbulentDistance',z)[0]
            an = Converter.addVars([an,ac1,ac2,ac3,ac4,ac5])
            ah = C.getField('centers:hi',z)[0]
            if ah != []: an = Converter.addVars([an,ah])
            ah = C.getField('centers:he',z)[0]
            if ah != []: an = Converter.addVars([an,ah])
            correctedPts = Connector.getInterpolatedPoints__(an)
            allCorrectedPts.append(correctedPts)
            xt = C.getField('CoordinateX',z)[0][1][0]
            snearl = xt[1]-xt[0]
            
            listOfSnearsLoc.append(snearl)
            if frontType == 42: listOfModelisationHeightsLoc.append(G_IBM_Height.computeModelisationHeight(Re=Reynolds, yplus=yplus, L=Lref))
            else:
                listOfModelisationHeightsLoc.append(0.)
                # if tfront2 is not None:
                #     listOfModelisationHeightsLoc.append(hmod)
                # else:
                #     listOfModelisationHeightsLoc.append(0.)
    #-------------------------------------------
    # 2. Get the list of IBC wall and interp pts
    #-------------------------------------------
    indcell = Converter.extractVars(allCorrectedPts,['indcell'])
    if tb is None or tfront is None: # constant hi, he
        for nozc in range(len(allCorrectedPts)):
            poshe = KCore.isNamePresent(allCorrectedPts[nozc],'he')
            if poshe == -1: allCorrectedPts[nozc] = Converter.initVars(allCorrectedPts[nozc],'he',he)
            poshi = KCore.isNamePresent(allCorrectedPts[nozc],'hi')
            if poshi == -1: allCorrectedPts[nozc] = Converter.initVars(allCorrectedPts[nozc],'hi',hi)
        res = connector.getIBMPtsBasic(allCorrectedPts, varsn, 'TurbulentDistance')
    else:
        dictOfBodiesByIBCType={}
        for s in Internal.getZones(tb):
            sdd = Internal.getNodeFromName1(s,".Solver#define")
            if sdd is not None: # check consistency of ibc type with flow equations
                ibctype = Internal.getNodeFromName1(sdd, "ibctype")
                if ibctype is not None:
                    ibctype = Internal.getValue(ibctype)
                else:
                    if IBCType == -1: ibctype = 'slip'
                    else: ibctype = 'Musker'
            else: # type of IBC not found: Euler -> slip, other : Musker
                if IBCType == -1: ibctype = 'slip'
                else: ibctype = 'Musker'
	    #Over ride as LBM only supports no slip at the moment
            if isLBM==True:
                ibctype = 'noslip'
            famName = Internal.getNodeFromType1(s,'FamilyName_t')
            if famName is not None:
               famName = Internal.getValue(famName)

            ibctypeI = TypesOfIBC[ibctype]
            if famName is not None: ibctype2 = str(ibctypeI)+"#"+famName
            else: ibctype2 = str(ibctypeI)
            if ibctype2 not in dictOfBodiesByIBCType: dictOfBodiesByIBCType[ibctype2]=[s]
            else: dictOfBodiesByIBCType[ibctype2]+=[s]
        
        # Regroupement des corps par type de BC - optimise les projections ensuite
        bodies = []; listOfIBCTypes=[]
        for itype in dictOfBodiesByIBCType:
            s = dictOfBodiesByIBCType.get(itype)
            body = C.getFields(Internal.__GridCoordinates__,s)
            body = Converter.convertArray2Tetra(body)
            body = Transform.join(body)
            bodies.append(body)
            listOfIBCTypes.append(itype)

        if tfront2 is not None:
            # premier essai de projection orthogonal sur tfront
            # si echec, second essai sur tfront2 en suivant les etapes de Stephanie
            front = C.getFields(Internal.__GridCoordinates__,tfront)
            front = Converter.convertArray2Tetra(front)
            front2 = C.getFields(Internal.__GridCoordinates__,tfront2)
            front2 = Converter.convertArray2Tetra(front2)
            allCorrectedPts = Converter.extractVars(allCorrectedPts,['CoordinateX','CoordinateY','CoordinateZ']+varsn)
            res = connector.getIBMPtsWithTwoFronts(allCorrectedPts, listOfSnearsLoc, listOfModelisationHeightsLoc, bodies,
                                               front, front2, varsn, signOfDistCorrected, depth)

        else:
            if frontType == 0:
                dmin = C.getMaxValue(tfront, 'TurbulentDistance')
                allCorrectedPts = Converter.initVars(allCorrectedPts,'dist',dmin)
                res = connector.getIBMPtsWithoutFront(allCorrectedPts, bodies, varsn, 'dist', signOfDistCorrected)
            else:
                front = C.getFields(Internal.__GridCoordinates__,tfront)
                
                front = Converter.convertArray2Tetra(front)
                allCorrectedPts = Converter.extractVars(allCorrectedPts,['CoordinateX','CoordinateY','CoordinateZ']+varsn)
                
                res = connector.getIBMPtsWithFront(allCorrectedPts, listOfSnearsLoc, listOfModelisationHeightsLoc, bodies,
                                                   front, varsn, signOfDistCorrected, depth, int(isWireModel), int(isOrthoFirst))

    allWallPts = res[0]
    allWallPts = Converter.extractVars(allWallPts,['CoordinateX','CoordinateY','CoordinateZ'])

    allInterpPts = res[1]
    allInterpPts = Converter.extractVars(allInterpPts,['CoordinateX','CoordinateY','CoordinateZ'])
    allInterpPts = Converter.addVars([allInterpPts,indcell])
    allCorrectedPts = Converter.extractVars(allCorrectedPts,['CoordinateX','CoordinateY','CoordinateZ'])

    dictOfInterpPtsByIBCType={}
    dictOfCorrectedPtsByIBCType={}
    dictOfWallPtsByIBCType={}
    nzonesR = len(allInterpPts)
    if len(res)==3:
        allIndicesByIBCType = res[2]
        for noz in range(nzonesR):
            indicesByTypeForZone = res[2][noz]
            nbTypes = len(indicesByTypeForZone)
            for nob in range(nbTypes):
                ibcTypeL = listOfIBCTypes[nob]
                indicesByTypeL = indicesByTypeForZone[nob]
                if indicesByTypeL.shape[0] > 0:
                    correctedPtsL = Transform.subzone(allCorrectedPts[noz], indicesByTypeL)
                    interpPtsL = Transform.subzone(allInterpPts[noz], indicesByTypeL)
                    wallPtsL = Transform.subzone(allWallPts[noz], indicesByTypeL)
                else:
                    correctedPtsL=[]; interpPtsL = []; wallPtsL = []
                if noz == 0:
                    dictOfCorrectedPtsByIBCType[ibcTypeL] = [correctedPtsL]
                    dictOfWallPtsByIBCType[ibcTypeL] = [wallPtsL]
                    dictOfInterpPtsByIBCType[ibcTypeL] = [interpPtsL]
                else:
                    dictOfCorrectedPtsByIBCType[ibcTypeL] += [correctedPtsL]
                    dictOfWallPtsByIBCType[ibcTypeL] += [wallPtsL]
                    dictOfInterpPtsByIBCType[ibcTypeL] += [interpPtsL]
    else:
        if IBCType == -1: ibcTypeL = 0
        else: ibcTypeL = 3
        for noz in range(nzonesR):
            if noz == 0:
                dictOfCorrectedPtsByIBCType[ibcTypeL] = [allCorrectedPts[noz]]
                dictOfWallPtsByIBCType[ibcTypeL] = [allWallPts[noz]]
                dictOfInterpPtsByIBCType[ibcTypeL] = [allInterpPts[noz]]
            else:
                dictOfCorrectedPtsByIBCType[ibcTypeL] += [allCorrectedPts[noz]]
                dictOfWallPtsByIBCType[ibcTypeL] += [allWallPts[noz]]
                dictOfInterpPtsByIBCType[ibcTypeL] += [allInterpPts[noz]]

    return dictOfCorrectedPtsByIBCType, dictOfWallPtsByIBCType, dictOfInterpPtsByIBCType


#=============================================================================
# Performs the full IBM preprocessing using overlapping Cartesian grids
# interpDataType = 1 : interpolation par preconditionnement par ADT
# interpDataType = 0 : interpolation optimisees sur grilles cartesiennes
# frontType 0, 1, 2
# Note [AJ]: isIbmAle : set to false - will be used in subsequent commits for moving IBMs
#=============================================================================
def prepareIBMData(t, tbody, DEPTH=2, loc='centers', frontType=1, interpDataType=0, smoothing=False, yplus=100., Lref=1., wallAdapt=None, blankingF42=False, isLBM=False,LBMQ=False,isPrintDebug=False,isIbmAle=False):
    tb =  Internal.copyRef(tbody)

    # tb: fournit model et dimension
    dimPb = Internal.getNodeFromName(tb,'EquationDimension')
    if dimPb is None: raise ValueError('prepareIBMData: EquationDimension is missing in input body tree.')
    dimPb = Internal.getValue(dimPb)

    # type de traitement paroi: pts interieurs ou externes
    model = Internal.getNodeFromName(tb, 'GoverningEquations')
    if model is None: raise ValueError('prepareIBMData: GoverningEquations is missing in input body tree.')
    # model: Euler, NSLaminar, NSTurbulent, LBMLaminar
    model = Internal.getValue(model)

    if model == 'Euler': IBCType =-1
    elif model == 'LBMLaminar':
        IBCType = -1
        if LBMQ: IBCType =1
    else: IBCType = 1 # Points cibles externes
    if loc == 'nodes':
        raise NotImplemented("prepareIBMData: prepareIBMData at nodes not yet implemented.")

    #------------------------
    # Ghost cells (overlaps)
    #------------------------
    X._applyBCOverlaps(t, depth=DEPTH,loc='centers',val=2, cellNName='cellN')
    C._initVars(t,'{centers:cellNChim}={centers:cellN}')

    #------------------------
    # Blanking IBM
    #------------------------
    C._initVars(t,'centers:cellN',1.)
    if dimPb == 2:
        z0 = Internal.getNodeFromType2(t, 'Zone_t')
        dims = Internal.getZoneDim(z0)
        npts = dims[1]*dims[2]*dims[3]
        zmin = C.getValue(z0,'CoordinateZ',0)
        zmax = C.getValue(z0,'CoordinateZ',npts-1)
        dz = zmax-zmin
        # Creation du corps 2D pour le preprocessing IBC
        T._addkplane(tb)
        T._contract(tb, (0,0,0), (1,0,0), (0,1,0), dz)

    t = blankByIBCBodies(t, tb, 'centers', dimPb)
    C._initVars(t,'{centers:cellNIBC}={centers:cellN}')

    #-----------------------------------------
    # calcul de la normale et distance signee
    #-----------------------------------------
    COMPDIST = False # distance deja calculee ou non
    if Internal.getNodeFromName(t, 'TurbulentDistance') is None: COMPDIST=True
    if COMPDIST:
        print('Computing distance field...')
        DTW._distance2Walls(t, tb, loc='centers', type='ortho', signed=0)
    else: pass
    _signDistance(t)

    #-----------------------------------------
    # Pts IBC
    #-----------------------------------------
    C._initVars(t,'{centers:cellN}={centers:cellNIBC}')
    # determination des pts IBC
    Reynolds = Internal.getNodeFromName(tb, 'Reynolds')
    if Reynolds is not None: Reynolds = Internal.getValue(Reynolds)
    else: Reynolds = 6.e6
    if Reynolds < 1.e5: frontType = 1
    if frontType != 42:
        if IBCType == -1: X._setHoleInterpolatedPoints(t,depth=-DEPTH,dir=0,loc='centers',cellNName='cellN',addGC=False)
        elif IBCType == 1:
            X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellN',addGC=False) # pour les gradients
            if frontType < 2:
                X._setHoleInterpolatedPoints(t,depth=DEPTH,dir=0,loc='centers',cellNName='cellN',addGC=False)
            else:
                DEPTHL=DEPTH+1
                X._setHoleInterpolatedPoints(t,depth=DEPTHL,dir=0,loc='centers',cellNName='cellN',addGC=False)
                #cree des pts extrapoles supplementaires
                # _blankClosestTargetCells(t,cellNName='cellN', depth=DEPTHL)
        else:
            raise ValueError('prepareIBMData: not valid IBCType. Check model.')
    else:
        # determination des pts IBC en fonction de la distance a la paroi
        # cree d'abord un front de type 1 pour assurer que l'adaptation ne descende pas en dessous de la limite...
        C._initVars(t,'{centers:cellNMin}={centers:cellNIBC}')
        if IBCType == -1: X._setHoleInterpolatedPoints(t,depth=-DEPTH,dir=0,loc='centers',cellNName='cellNMin',addGC=False)
        elif IBCType == 1: X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellNMin',addGC=False) # pour les gradients
        X._setHoleInterpolatedPoints(t,depth=DEPTH,dir=0,loc='centers',cellNName='cellNMin',addGC=False)

        for z in Internal.getZones(t):
            h = abs(C.getValue(z,'CoordinateX',0)-C.getValue(z,'CoordinateX',1))
            if yplus > 0.:
                height = G_IBM_Height.computeModelisationHeight(Re=Reynolds, yplus=yplus, L=Lref)
            else:
                height = G_IBM_Height.computeBestModelisationHeight(Re=Reynolds, h=h) # meilleur compromis entre hauteur entre le snear et la hauteur de modelisation
                yplus  = G_IBM_Height.computeYplus(Re=Reynolds, height=height, L=Lref)
            C._initVars(z,'{centers:cellN}=({centers:TurbulentDistance}>%20.16g)+(2*({centers:TurbulentDistance}<=%20.16g)*({centers:TurbulentDistance}>0))'%(height,height))

        # Si wallAdapt, utilisation de la solution precedente pour ne garder que les pts cibles tq y+PC <= y+ref : rapproche le front de la paroi (utile proche bord d'attaque)
        # Attention, le fichier d'adaptation doit etre un nuage de points dont les coordonnees correspondent aux points cibles (Cf modification dans extractIBMWallFields avec tb=None)
        if wallAdapt is not None:
            C._initVars(t,'{centers:yplus}=100000.')
            w = C.convertFile2PyTree(wallAdapt)
            total = len(Internal.getZones(t))
            cpt = 1
            for z in Internal.getZones(t):
                print("{} / {}".format(cpt, total))
                cellN = Internal.getNodeFromName(z,'cellN')[1]
                if 2 in cellN:
                    zname = z[0]
                    zd = Internal.getNodeFromName(w, zname)
                    if zd is not None:
                        yplus_w = Internal.getNodeFromName(zd, 'yplus')[1]
                        listIndices = Internal.getNodeFromName(zd, 'PointListDonor')[1]

                        n = numpy.shape(yplus_w)[0]
                        yplusA = Converter.array('yplus', n, 1, 1)
                        yplusA[1][:] = yplus_w

                        C._setPartialFields(z, [yplusA], [listIndices], loc='centers')

                cpt += 1

            C._initVars(t,'{centers:cellN}=({centers:cellN}>0) * ( (({centers:cellN}) * ({centers:yplus}<=%20.16g)) + ({centers:yplus}>%20.16g) )'%(yplus,yplus))

        # Securite finale, on aura au min deux rangees de points cibles
        C._initVars(t, '{centers:cellN} = maximum({centers:cellN}, {centers:cellNMin})')
        C._rmVars(t,['centers:yplus', 'centers:cellNMin'])

        # permet de ne garder que les deux rangees superieures (prises en compte par le solveur), mais complique l'adaptation suivante et la visualisation
        if blankingF42: X._maximizeBlankedCells(t, depth=2, addGC=False)

    _removeBlankedGrids(t, loc='centers')
    print('Nb of Cartesian grids=%d.'%len(Internal.getZones(t)))
    npts = 0
    for i in Internal.getZones(t):
       dims = Internal.getZoneDim(i)
       npts += dims[1]*dims[2]*dims[3]
    print('Final number of points=%5.4f millions.'%(npts/1000000.))

    C._initVars(t,'{centers:cellNIBC}={centers:cellN}')

    #------------------------------------------------------------------------
    # Nature des points en fonction de leur nature Chimere et leur nature IBC
    #------------------------------------------------------------------------
    # -3 : agit comme un point masque - non donneur pour le type de point
    #  3  : agit comme donneur uniquement
    # updateNatureForIBM: modifie cellNChim, cellNFront, cellNIBM
    # cellNChim=-3, si cellNIBC=0 (masque)
    if IBCType == 1: # Points corriges IBM externes
        C._initVars(t,'{centers:cellNFront}=logical_and({centers:cellNIBC}>0.5, {centers:cellNIBC}<1.5)')
        for z in Internal.getZones(t):
            connector._updateNatureForIBM(z, IBCType,
                                          Internal.__GridCoordinates__,
                                          Internal.__FlowSolutionNodes__,
                                          Internal.__FlowSolutionCenters__)

    else: # EN 2 PARTIES : NECESSITE LE TRANSFERT DU FRONT PAR INTERPOLATION, QUI EST CALCULEE APRES
        print('Euler: on repousse le front un peu plus loin.')
        C._initVars(t,'{centers:dummy}={centers:cellN}') # sauvegarde
        C._initVars(t,'{centers:cellN}=({centers:cellNIBC}>0.5)*({centers:cellNIBC}<1.5)')
        X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellN',addGC=False)
        C._initVars(t,'{centers:cellNFront}=logical_and({centers:cellN}>0.5, {centers:cellN}<1.5)')
        C._cpVars(t,'centers:dummy',t,'centers:cellN')
        C._rmVars(t, ['centers:dummy'])
        for z in Internal.getZones(t):
            connector._updateNatureForIBM(z, IBCType,
                                          Internal.__GridCoordinates__,
                                          Internal.__FlowSolutionNodes__,
                                          Internal.__FlowSolutionCenters__)
    #------------------------------------------------------------------------
    # setInterpData - Chimere
    C._initVars(t,'{centers:cellN}=maximum(0.,{centers:cellNChim})')# vaut -3, 0, 1, 2 initialement

    # maillage donneur: on MET les pts IBC comme donneurs
    tc = C.node2Center(t)
    FSN = Internal.getNodesFromName(tc, Internal.__FlowSolutionNodes__)
    Internal._rmNodesByName(FSN,'cellNFront')
    Internal._rmNodesByName(FSN,'cellNIBC')
    Internal._rmNodesByName(FSN, "TurbulentDistance")

    tbb = G.BB(tc)

    # Creation du dictionnaire des ADT pour les raccords
    if interpDataType == 1:
        dictOfADT = {}
        for zdnr in Internal.getZones(tc):
            zdnrname = zdnr[0]
            if zdnrname not in dictOfADT:
                HOOKADT = C.createHook(zdnr, 'adt')
                dictOfADT[zdnrname] = HOOKADT
    else: dictOfADT = None
    print('Interpolations Chimere.')
    tc = doInterp(t, tc, tbb, tb=None, typeI='ID', dim=dimPb,
                  interpDataType=interpDataType, dictOfADT=dictOfADT,isIbmAle=isIbmAle)
    if dictOfADT is not None:
        for dnrname in dictOfADT: C.freeHook(dictOfADT[dnrname])

    # setIBCData - IBC
    C._initVars(t,'{centers:cellNIBCDnr}=minimum(2.,abs({centers:cellNIBC}))')
    C._initVars(t,'{centers:cellNIBC}=maximum(0.,{centers:cellNIBC})')# vaut -3, 0, 1, 2, 3 initialement
    C._initVars(t,'{centers:cellNIBC}={centers:cellNIBC}*({centers:cellNIBC}<2.5)')
    C._cpVars(t,'centers:cellNIBC',t,'centers:cellN')
    C._cpVars(t,'centers:cellN',tc,'cellN')

    #-----------------------------------------------
    # Transfert du cellNFront
    C._cpVars(t,'centers:cellNFront',tc,'cellNFront')

    for zc in Internal.getZones(tc):
        cellNFront = Internal.getNodeFromName2(zc,'cellNFront')
        if cellNFront != []:
            cellNFront = cellNFront[1]
            sizeTot = cellNFront.shape[0]*cellNFront.shape[1]*cellNFront.shape[2]
            sizeOne =  int(numpy.sum(cellNFront))
            if sizeOne < sizeTot:
                XOD._setInterpTransfers(t,zc,variables=['cellNFront'],cellNVariable='cellNFront',compact=0)

    if frontType == 2:
        _pushBackImageFront2(t, tc, tbb, interpDataType=interpDataType)
        if smoothing and dimPb==2: _smoothImageFront(t, tc); _pushBackImageFront2(t, tc, tbb, interpDataType=interpDataType)

        C._cpVars(t,'centers:cellNFront',tc,'cellNFront')

        for zc in Internal.getZones(tc):
            cellNFront = Internal.getNodeFromName2(zc,'cellNFront')
            if cellNFront != []:
                cellNFront = cellNFront[1]
                sizeTot = cellNFront.shape[0]*cellNFront.shape[1]*cellNFront.shape[2]
                sizeOne =  int(numpy.sum(cellNFront))
                if sizeOne < sizeTot:
                    XOD._setInterpTransfers(t,zc,variables=['cellNFront'],cellNVariable='cellNFront',compact=0)

    ## Fin traitement specifique, vaut 0 ou 1 apres la ligne suivante
    # C._cpVars(t,'centers:cellNFront',tc,'cellNFront')
    C._rmVars(t,['centers:cellNFront'])
    C._cpVars(t,'centers:TurbulentDistance',tc,'TurbulentDistance')

    print('Minimum distance: %f.'%C.getMinValue(t,'centers:TurbulentDistance'))
    P._computeGrad2(t, 'centers:TurbulentDistance', withCellN=False)
    print('Building the IBM front.')
    front = getIBMFront(tc, 'cellNFront', dimPb, frontType)
    if isPrintDebug:
        C.convertPyTree2File(front, 'IB_front.cgns')
    # C.convertPyTree2File(front, 'front.cgns')
    print('Interpolations IBM')
    tc = doInterp(t, tc, tbb, tb=tb,typeI='IBCD', dim=dimPb, dictOfADT=None, front=front, frontType=frontType, depth=DEPTH, IBCType=IBCType, interpDataType=interpDataType, Reynolds=Reynolds, yplus=yplus, Lref=Lref, isLBM=isLBM,isIbmAle=isIbmAle)

    # cleaning...
    Internal._rmNodesByName(tc, Internal.__FlowSolutionNodes__)
    if not isIbmAle:
        Internal._rmNodesByName(tc, Internal.__GridCoordinates__)
    C._initVars(t,'{centers:cellN}=minimum({centers:cellNChim}*{centers:cellNIBCDnr},2.)')
    varsRM = ['centers:gradxTurbulentDistance','centers:gradyTurbulentDistance','centers:gradzTurbulentDistance','centers:cellNFront','centers:cellNIBCDnr']
    varsRM += ['centers:cellNChim','centers:cellNIBC']
    C._rmVars(t, varsRM)
    C._rmVars(tc,['cellNChim','cellNIBC','TurbulentDistance'])
    #----------
    # SORTIE
    #----------
    return t, tc

#=============================================================================
# Performs the full IBM preprocessing using overlapping Cartesian grids
# interpDataType = 1 : interpolation par preconditionnement par ADT
# interpDataType = 0 : interpolation optimisees sur grilles cartesiennes
# frontType 0, 1, 2
#=============================================================================
def prepareIBMData2(t, tbody, DEPTH=2, loc='centers', frontType=1, inv=False, interpDataType=1):
    tb =  Internal.copyRef(tbody)

    # tb: fournit model et dimension
    dimPb = Internal.getNodeFromName(tb,'EquationDimension')
    if dimPb is None: raise ValueError('prepareIBMData: EquationDimension is missing in input body tree.')
    dimPb = Internal.getValue(dimPb)

    # type de traitement paroi: pts interieurs ou externes
    model = Internal.getNodeFromName(tb, 'GoverningEquations')
    if model is None: raise ValueError('prepareIBMData: GoverningEquations is missing in input body tree.')
    # model: Euler, NSLaminar, NSTurbulent
    model = Internal.getValue(model)

    if model == 'Euler': IBCType =-1
    else: IBCType = 1 # Points cibles externes
    if loc == 'nodes':
        raise NotImplemented("prepareIBMData: prepareIBMData at nodes not yet implemented.")

    #------------------------
    # Ghost cells (overlaps)
    #------------------------
    X._applyBCOverlaps(t, depth=DEPTH,loc='centers',val=2, cellNName='cellN')
    C._initVars(t,'{centers:cellNChim}={centers:cellN}')

    #------------------------
    # Blanking IBM
    #------------------------
    C._initVars(t,'centers:cellN',1.)
    if dimPb == 2:
        z0 = Internal.getNodeFromType2(t, 'Zone_t')
        dims = Internal.getZoneDim(z0)
        npts = dims[1]*dims[2]*dims[3]
        zmin = C.getValue(z0,'CoordinateZ',0)
        zmax = C.getValue(z0,'CoordinateZ',npts-1)
        dz = zmax-zmin
        # Creation du corps 2D pour le preprocessing IBC
        T._addkplane(tb)
        T._contract(tb, (0,0,0), (1,0,0), (0,1,0), dz)

    t = blankByIBCBodies(t,tb,'centers',dimPb)
    if not inv: C._initVars(t,'{centers:cellNIBC}={centers:cellN}')
    if inv: C._initVars(t,'{centers:cellNIBC}=1-{centers:cellN}') # ecoulement interne



    #-----------------------------------------
    # calcul de la normale et distance signee
    #-----------------------------------------
    COMPDIST = False # distance deja calculee ou non
    if Internal.getNodeFromName(t, 'TurbulentDistance') is None: COMPDIST=True
    if COMPDIST:
        print('Computing distance field...')
        DTW._distance2Walls(t,tb,loc='centers',type='ortho',signed=0)
    else: pass
    _signDistance(t)

    #-----------------------------------------
    # Pts IBC
    #-----------------------------------------
    C._initVars(t,'{centers:cellN}={centers:cellNIBC}')
    # determination des pts IBC
    if IBCType == -1: X._setHoleInterpolatedPoints(t,depth=-DEPTH,dir=0,loc='centers',cellNName='cellN',addGC=False)
    elif IBCType == 1:
        X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellN',addGC=False) # pour les gradients
        if frontType < 2:
            X._setHoleInterpolatedPoints(t,depth=DEPTH,dir=0,loc='centers',cellNName='cellN',addGC=False)
        else:
            DEPTHL=DEPTH+1
            X._setHoleInterpolatedPoints(t,depth=DEPTHL,dir=0, loc='centers',cellNName='cellN',addGC=False)
            #cree des pts extrapoles supplementaires
            # _blankClosestTargetCells(t,cellNName='cellN', depth=DEPTHL)
    else:
        raise ValueError('prepareIBMData: not valid IBCType. Check model.')
    _removeBlankedGrids(t, loc='centers')
    print('Nb of Cartesian grids=%d.'%len(Internal.getZones(t)))
    npts = 0
    for i in Internal.getZones(t):
       dims = Internal.getZoneDim(i)
       npts += dims[1]*dims[2]*dims[3]
    print('Final number of points=%5.4f millions.'%(npts/1000000.))

    C._initVars(t,'{centers:cellNIBC}={centers:cellN}')

    #------------------------------------------------------------------------
    # Nature des points en fonction de leur nature Chimere et leur nature IBC
    #------------------------------------------------------------------------
    # -3 : agit comme un point masque - non donneur pour le type de point
    #  3  : agit comme donneur uniquement
    # updateNatureForIBM: modifie cellNChim, cellNFront, cellNIBM
    # cellNChim=-3, si cellNIBC=0 (masque)
    if IBCType == 1: # Points corriges IBM externes
        C._initVars(t,'{centers:cellNFront}=logical_and({centers:cellNIBC}>0.5, {centers:cellNIBC}<1.5)')
        for z in Internal.getZones(t):
            connector._updateNatureForIBM(z, IBCType,
                                          Internal.__GridCoordinates__,
                                          Internal.__FlowSolutionNodes__,
                                          Internal.__FlowSolutionCenters__)

    else: # EN 2 PARTIES : NECESSITE LE TRANSFERT DU FRONT PAR INTERPOLATION, QUI EST CALCULEE APRES
        print('Euler: on repousse le front un peu plus loin.')
        C._initVars(t,'{centers:dummy}={centers:cellN}') # sauvegarde
        C._initVars(t,'{centers:cellN}=({centers:cellNIBC}>0.5)*({centers:cellNIBC}<1.5)')
        X._setHoleInterpolatedPoints(t,depth=1,dir=1,loc='centers',cellNName='cellN',addGC=False)
        C._initVars(t,'{centers:cellNFront}=logical_and({centers:cellN}>0.5, {centers:cellN}<1.5)')
        C._cpVars(t,'centers:dummy',t,'centers:cellN')
        C._rmVars(t, ['centers:dummy'])
        for z in Internal.getZones(t):
            connector._updateNatureForIBM(z, IBCType,
                                          Internal.__GridCoordinates__,
                                          Internal.__FlowSolutionNodes__,
                                          Internal.__FlowSolutionCenters__)
    #------------------------------------------------------------------------
    # setInterpData - Chimere
    C._initVars(t,'{centers:cellN}=maximum(0.,{centers:cellNChim})')# vaut -3, 0, 1, 2 initialement

    # maillage donneur: on MET les pts IBC comme donneurs
    tc = C.node2Center(t)
    FSN = Internal.getNodesFromName(tc, Internal.__FlowSolutionNodes__)
    Internal._rmNodesByName(FSN,'cellNFront')
    Internal._rmNodesByName(FSN,'cellNIBC')
    Internal._rmNodesByName(FSN, "TurbulentDistance")

    tbb = G.BB(tc)

    # Creation du dictionnaire des ADT pour les raccords
    if interpDataType == 1:
        dictOfADT = {}
        for zdnr in Internal.getZones(tc):
            zdnrname = zdnr[0]
            if zdnrname not in dictOfADT:
                HOOKADT = C.createHook(zdnr, 'adt')
                dictOfADT[zdnrname] = HOOKADT
    else: dictOfADT = None
    print('Interpolations Chimere.')
    tc = doInterp2(t, tc, tbb, tb=None, typeI='ID', dim=dimPb,
                  interpDataType=interpDataType, dictOfADT=dictOfADT)
    if dictOfADT is not None:
        for dnrname in dictOfADT: C.freeHook(dictOfADT[dnrname])

    # setIBCData - IBC
    C._initVars(t,'{centers:cellNIBCDnr}=minimum(2.,abs({centers:cellNIBC}))')
    C._initVars(t,'{centers:cellNIBC}=maximum(0.,{centers:cellNIBC})')# vaut -3, 0, 1, 2, 3 initialement
    C._initVars(t,'{centers:cellNIBC}={centers:cellNIBC}*({centers:cellNIBC}<2.5)')
    C._cpVars(t,'centers:cellNIBC',t,'centers:cellN')
    C._cpVars(t,'centers:cellN',tc,'cellN')

    #-----------------------------------------------
    # Transfert du cellNFront
    C._cpVars(t,'centers:cellNFront',tc,'cellNFront')

    for zc in Internal.getZones(tc):
        cellNFront = Internal.getNodeFromName2(zc,'cellNFront')
        if cellNFront != []:
            cellNFront = cellNFront[1]
            sizeTot = cellNFront.shape[0]*cellNFront.shape[1]*cellNFront.shape[2]
            sizeOne =  int(numpy.sum(cellNFront))
            if sizeOne < sizeTot:
                XOD._setInterpTransfers(t,zc,variables=['cellNFront'],cellNVariable='cellNFront',compact=0)

    if frontType==2 or frontType==3: _pushBackImageFront2(t, tc, tbb, interpDataType=interpDataType)

    ## Fin traitement specifique, vaut 0 ou 1 apres la ligne suivante
    C._cpVars(t,'centers:cellNFront',tc,'cellNFront')
    C._rmVars(t,['centers:cellNFront'])
    C._cpVars(t,'centers:TurbulentDistance',tc,'TurbulentDistance')

    print('Minimum distance: %f.'%C.getMinValue(t,'centers:TurbulentDistance'))
    P._computeGrad2(t, 'centers:TurbulentDistance', withCellN=False)
    print('Building the IBM front.')
    front = getIBMFront(tc, 'cellNFront', dimPb, frontType)
    print('Interpolations IBM')
    tc = doInterp2(t,tc,tbb, tb=tb,typeI='IBCD',dim=dimPb, dictOfADT=None, front=front, frontType=frontType, depth=DEPTH, IBCType=IBCType, interpDataType=interpDataType)

    # cleaning...
    Internal._rmNodesByName(tc, Internal.__FlowSolutionNodes__)
    Internal._rmNodesByName(tc, Internal.__GridCoordinates__)
    C._initVars(t,'{centers:cellN}=minimum({centers:cellNChim}*{centers:cellNIBCDnr},2.)')
    varsRM = ['centers:gradxTurbulentDistance','centers:gradyTurbulentDistance','centers:gradzTurbulentDistance','centers:cellNFront','centers:cellNIBCDnr']
    varsRM += ['centers:cellNChim','centers:cellNIBC']
    C._rmVars(t, varsRM)
    C._rmVars(tc,['cellNChim','cellNIBC','TurbulentDistance'])
    #----------
    # SORTIE
    #----------
    return t, tc



#=============================================================================
# Extraction des infos pour adaptation de front
# [BC] to complete
#=============================================================================
def createWallAdapt(tc):
    """Create a cloud of target points from y+ information."""
    listOfZones = []
    DictOfZones = {}

    for zc in Internal.getZones(tc):
        subRegions = Internal.getNodesFromType1(zc, 'ZoneSubRegion_t')
        for s in subRegions:
            sname = s[0][0:4]
            zname = s[0].split('_')[-1]
            # zname = zname.split('X')[0]
            if sname == 'IBCD':
                if zname not in DictOfZones:
                    DictOfZones[zname] = {"CoordinateX_PC":[], "CoordinateY_PC":[], "CoordinateZ_PC":[], "yplus":[], "PointListDonor":[]}

                xPC = Internal.getNodeFromName1(s,"CoordinateX_PC")[1]
                yPC = Internal.getNodeFromName1(s,"CoordinateY_PC")[1]
                zPC = Internal.getNodeFromName1(s,"CoordinateZ_PC")[1]
                DictOfZones[zname]["CoordinateX_PC"].append(xPC)
                DictOfZones[zname]["CoordinateY_PC"].append(yPC)
                DictOfZones[zname]["CoordinateZ_PC"].append(zPC)

                yplus = Internal.getNodeFromName1(s, "yplus")[1]
                DictOfZones[zname]["yplus"].append(yplus)

                pointListDnr = Internal.getNodeFromName1(s, "PointListDonor")[1]
                DictOfZones[zname]["PointListDonor"].append(pointListDnr)

    for zname in DictOfZones:
        xcNP = numpy.concatenate(DictOfZones[zname]["CoordinateX_PC"])
        ycNP = numpy.concatenate(DictOfZones[zname]["CoordinateY_PC"])
        zcNP = numpy.concatenate(DictOfZones[zname]["CoordinateZ_PC"])
        yplusNP = numpy.concatenate(DictOfZones[zname]["yplus"])
        pointListNP = numpy.concatenate(DictOfZones[zname]["PointListDonor"])

        # Creation d une seule zone
        zsize = numpy.empty((1,3), numpy.int32, order='F')
        zsize[0,0] = xcNP.shape[0]; zsize[0,1] = 0; zsize[0,2] = 0
        z = Internal.newZone(name=zname,zsize=zsize,ztype='Unstructured')
        gc = Internal.newGridCoordinates(parent=z)
        coordx = ['CoordinateX',xcNP,[],'DataArray_t']
        coordy = ['CoordinateY',ycNP,[],'DataArray_t']
        coordz = ['CoordinateZ',zcNP,[],'DataArray_t']
        gc[2] = [coordx,coordy,coordz]
        n = Internal.createChild(z, 'GridElements', 'Elements_t', [2,0])
        Internal.createChild(n, 'ElementRange', 'IndexRange_t', [1,0])
        Internal.createChild(n, 'ElementConnectivity', 'DataArray_t', None)
        gridLocation = 'Vertex'
        FSN = Internal.newFlowSolution(name=Internal.__FlowSolutionNodes__,
                                       gridLocation=gridLocation, parent=z)

        FSN[2].append(["yplus",yplusNP, [],'DataArray_t'])
        FSN[2].append(["PointListDonor",pointListNP, [],'DataArray_t'])
        listOfZones.append(z)

    t = C.newPyTree(['Base', listOfZones])
    return t

#=============================================================================
# Creation des zones 1D de nom 'Zone#IBCD_*' pour pouvoir etre identifie en post
# retourne tw avec une zone 1D par IBCD de depart
# Sert au post traitement paroi en parallele et instationnaire
#=============================================================================
def createIBMWZones(tc, variables=[]):
    """Extract IBM informations from tc before post-processing."""
    import Converter.Mpi as Cmpi
    tw = C.newPyTree(['IBM_WALL'])
    for z in Internal.getZones(tc):
        ZSR = Internal.getNodesFromType2(z,'ZoneSubRegion_t')
        procNode = Internal.getNodeFromName(z,'proc')
        proc = -1
        if procNode is not None:
            proc = Internal.getValue(procNode)

        for IBCD in Internal.getNodesFromName(ZSR,"IBCD_*"):
            xPW = Internal.getNodesFromName(IBCD,"CoordinateX_PW")[0][1]
            yPW = Internal.getNodesFromName(IBCD,"CoordinateY_PW")[0][1]
            zPW = Internal.getNodesFromName(IBCD,"CoordinateZ_PW")[0][1]
            nptsW = xPW.shape[0]
            zw = G.cart((0,0,0),(1,1,1),(nptsW,1,1))
            COORDX = Internal.getNodeFromName2(zw,'CoordinateX'); COORDX[1]=xPW
            COORDY = Internal.getNodeFromName2(zw,'CoordinateY'); COORDY[1]=yPW
            COORDZ = Internal.getNodeFromName2(zw,'CoordinateZ'); COORDZ[1]=zPW
            if variables != [] and variables != None:
                FSN = Internal.newFlowSolution(parent=zw)
                for varo in variables:
                    fieldV = Internal.getNodeFromName2(IBCD,varo)
                    if fieldV is not None:
                        C._initVars(zw,varo,0.)
                        fieldW = Internal.getNodeFromName2(FSN,varo)
                        fieldW[1] = fieldV[1]

            zw[0]=z[0]+"#"+IBCD[0]
            if proc != -1: Cmpi._setProc(zw,proc)
            tw[2][1][2].append(zw)
    return tw


#=============================================================================
# Compute curvature parameter "K" from geom in 2D
#=============================================================================
def _computeKcurvParameter(tc, tb):
    """Compute curvature parameter for simple 2D geometry."""

    ################################
    ## EXTRADOS ##
    ################################

    tb_extrados = P.selectCells(tb, '{CoordinateY}>0.')
    x_extrados  = Internal.getNodeFromName(tb_extrados, 'CoordinateX')[1]
    y_extrados  = Internal.getNodeFromName(tb_extrados, 'CoordinateY')[1]

    inds_extrados = x_extrados.argsort()
    x_extrados    = x_extrados[inds_extrados]
    y_extrados    = y_extrados[inds_extrados]

    # evite les divisions par 0
    nPts = len(x_extrados)
    listOfPos = []
    for i in range(1,nPts):
        if x_extrados[i] == x_extrados[i-1]:
            listOfPos.append(i)

    for pos in listOfPos:
        x_extrados = numpy.delete(x_extrados, pos)
        y_extrados = numpy.delete(y_extrados, pos)

    nPts = len(x_extrados)
    y1_extrados = numpy.zeros(nPts)
    y2_extrados = numpy.zeros(nPts)

    # derivee premiere y1
    # schema decentre pour CL
    y1_extrados[0] = (y_extrados[1]-y_extrados[0])/((x_extrados[1]-x_extrados[0]))
    y1_extrados[-1] = (y_extrados[-2]-y_extrados[-1])/((x_extrados[-2]-x_extrados[-1]))

    # schema centre
    for i in range(1, nPts-1):
        y1_extrados[i] = (y_extrados[i+1]-y_extrados[i-1])/((x_extrados[i+1]-x_extrados[i-1]))

    # derivee seconde y2
    # schema decentre pour CL
    y2_extrados[0] = (y1_extrados[1]-y1_extrados[0])/((x_extrados[1]-x_extrados[0]))
    y2_extrados[-1] = (y1_extrados[-2]-y1_extrados[-1])/((x_extrados[-2]-x_extrados[-1]))

    # schema centre
    for i in range(1, nPts-1):
        y2_extrados[i] = (y1_extrados[i+1]-y1_extrados[i-1])/((x_extrados[i+1]-x_extrados[i-1]))

    k_extrados = y2_extrados/(numpy.power(1 + numpy.power(y1_extrados, 2), 1.5))
    ka_extrados = numpy.abs(y2_extrados)/(numpy.power(1 + numpy.power(y1_extrados, 2), 1.5))

    ################################
    ## INTRADOS ##
    ################################

    tb_intrados = P.selectCells(tb, '{CoordinateY}<0.')
    x_intrados = Internal.getNodeFromName(tb_intrados, 'CoordinateX')[1]
    y_intrados = numpy.abs(Internal.getNodeFromName(tb_intrados, 'CoordinateY')[1]) # abs pour avoir vraie valeur k

    inds_intrados = x_intrados.argsort()
    x_intrados = x_intrados[inds_intrados]
    y_intrados = y_intrados[inds_intrados]

    # evite les divisions par 0
    nPts = len(x_intrados)
    listOfPos = []
    for i in range(1,nPts):
        if x_intrados[i] == x_intrados[i-1]:
            listOfPos.append(i)

    for pos in listOfPos:
        x_intrados = numpy.delete(x_intrados, pos)
        y_intrados = numpy.delete(y_intrados, pos)

    nPts = len(x_intrados)
    y1_intrados = numpy.zeros(nPts)
    y2_intrados = numpy.zeros(nPts)

    # derivee premiere y1
    # schema decentre pour CL
    y1_intrados[0] = (y_intrados[1]-y_intrados[0])/((x_intrados[1]-x_intrados[0]))
    y1_intrados[-1] = (y_intrados[-2]-y_intrados[-1])/((x_intrados[-2]-x_intrados[-1]))

    # schema centre
    for i in range(1, nPts-1):
        y1_intrados[i] = (y_intrados[i]-y_intrados[i-1])/((x_intrados[i]-x_intrados[i-1]))

    # derivee seconde y2
    # schema decentre pour CL
    y2_intrados[0] = (y1_intrados[1]-y1_intrados[0])/((x_intrados[1]-x_intrados[0]))
    y2_intrados[-1] = (y1_intrados[-2]-y1_intrados[-1])/((x_intrados[-2]-x_intrados[-1]))

    # schema centre
    for i in range(1, nPts-1):
        y2_intrados[i] = (y1_intrados[i]-y1_intrados[i-1])/((x_intrados[i]-x_intrados[i-1]))

    k_intrados = y2_intrados/(numpy.power(1 + numpy.power(y1_intrados, 2), 1.5))
    ka_intrados = numpy.abs(y2_intrados)/(numpy.power(1 + numpy.power(y1_intrados, 2), 1.5))

    ################################
    ## MaJ tc ##
    ################################
    for z in Internal.getZones(tc):
        for zsr in Internal.getNodesFromType1(z, 'ZoneSubRegion_t'):
            nameSubRegion = zsr[0]
            if nameSubRegion[0:4]=='IBCD':
                ibctypeCR = nameSubRegion.split('_')[1]
                if ibctypeCR=='100':
                    KCurv = Internal.getNodeFromName(zsr, XOD.__KCURV__)[1]
                    coordX = Internal.getNodeFromName(zsr, 'CoordinateX_PW')[1]
                    coordY = Internal.getNodeFromName(zsr, 'CoordinateY_PW')[1]
                    nIBC = numpy.shape(coordX)[0]

                    for i in range(nIBC):
                        if coordY[i] > 0:
                            j = 0
                            while(coordX[i] > x_extrados[j] and j < numpy.shape(x_extrados)[0]-1):
                                j += 1
                            KCurv[i] = k_extrados[j]
                        else:
                            j = 0
                            while(coordX[i] > x_intrados[j] and j < numpy.shape(x_intrados)[0]-1):
                                j += 1
                            KCurv[i] = k_intrados[j]
                        KCurv[i] = min(KCurv[i], 100.)
                        KCurv[i] = max(KCurv[i], -100.)

                    Internal.getNodeFromName(zsr, XOD.__KCURV__)[1] = KCurv

    return None



# distance signee en fonction du masquage Chimere et IBC
#=============================================================================
def _signDistance(t):
    """Modify distance to be signed based on cellNIBC and cellNChim (outside fluid domain)."""
    C._initVars(t,'{centers:TurbulentDistance}=-1.*({centers:cellNIBC}*{centers:cellNChim}<1.)*{centers:TurbulentDistance}+({centers:cellNIBC}*{centers:cellNChim}>0.)*{centers:TurbulentDistance}')
    return None

