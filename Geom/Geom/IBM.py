"""Immersed boundary geometry definition module.
"""
import Converter.PyTree as C
import Converter.Internal as Internal
import Converter
import numpy

vars_delete_ibm=['utau','StagnationEnthalpy','StagnationPressure',
                 'dirx'          ,'diry'          ,'dirz',
                 'gradxPressure' ,'gradyPressure' ,'gradzPressure' ,
                 'gradxVelocityX','gradyVelocityX','gradzVelocityX',
                 'gradxVelocityY','gradyVelocityY','gradzVelocityY',
                 'gradxVelocityZ','gradyVelocityZ','gradzVelocityZ',
                 'KCurv','yplus']

# Set snear in zones
def setSnear(t, value):
    """Set the value of snear in a geometry pyTree.
    Usage: setSnear(t,value=X)"""
    tp = Internal.copyRef(t)
    _setSnear(tp, value)
    return tp


def _setSnear(z, value):
    """Set the value of snear in a geometry pyTree.
    Usage: _setSnear(t,value=X)"""
    zones = Internal.getZones(z)
    for z in zones:
        Internal._createUniqueChild(z, '.Solver#define', 'UserDefinedData_t')
        n = Internal.getNodeFromName1(z, '.Solver#define')
        Internal._createUniqueChild(n, 'snear', 'DataArray_t', value)
    return None


# Set dfar in zones
def setDfar(t, value):
    """Set the value of dfar in a geometry pytree.
    Usage: setDfar(t,value=X)"""
    tp = Internal.copyRef(t)
    _setDfar(tp, value)
    return tp


def _setDfar(z, value):
    """Set the value of dfar in a geometry pytree.
        Usage: _setDfar(t,value=X)"""
    zones = Internal.getZones(z)
    for z in zones:
        Internal._createUniqueChild(z, '.Solver#define', 'UserDefinedData_t')
        n = Internal.getNodeFromName1(z, '.Solver#define')
        Internal._createUniqueChild(n, 'dfar', 'DataArray_t', value)
    return None


# Multiply the snear by factors XX in zones
def snearFactor(t, sfactor):
    """Mulitply the value of snear in a geometry pyTree by a sfactor.
    Usage: snearFactor(t,sfactor)"""
    tp = Internal.copyRef(t)
    _snearFactor(tp, sfactor)
    return tp


def _snearFactor(t, sfactor):
    """Mulitply the value of snear in a geometry pyTree by a sfactor.
    Usage: _snearFactor(t,sfactor)"""
    zones = Internal.getZones(t)
    for z in zones:
        nodes = Internal.getNodesFromName2(z, 'snear')
        for n in nodes:
            Internal._setValue(n, sfactor*Internal.getValue(n))
    return None


# Set the IBC type in zones
def setIBCType(t, value):
    """Set the IBC type in a geometry pyTree.
    Usage: setIBCType(t,value=X)"""
    tp = Internal.copyRef(t)
    _setIBCType(tp, value)
    return tp


def _setIBCType(z, value):
    """Set the IBC type in a geometry pyTree.
    Usage: _setIBCType(t,value=X)"""
    zones = Internal.getZones(z)
    for z in zones:
        Internal._createUniqueChild(z, '.Solver#define', 'UserDefinedData_t')
        n = Internal.getNodeFromName1(z, '.Solver#define')
        Internal._createUniqueChild(n, 'ibctype', 'DataArray_t', value)
    return None


# Set the IBC type outpress for zones in familyName
def initOutflow(tc, familyName, P_static):
    """Set the value of static pressure P_static for the outflow pressure IBC with family name familyName.
    Usage: initOutflow(tc,familyName, P_static)"""
    tc2 = Internal.copyRef(tc)
    _initOutflow(tc2, familyName, P_static)
    return tc2
                 

def _initOutflow(tc, familyName, P_static):
    """Set the value of the pressure P_static for the outflow pressure IBC with family name familyName.
    Usave: _initOutflow(tc,familyName, P_static)"""    
    for zc in Internal.getZones(tc):
        for zsr in Internal.getNodesFromName(zc,'IBCD_4_*'):
            FamNode = Internal.getNodeFromType1(zsr,'FamilyName_t')
            if FamNode is not None:
                FamName = Internal.getValue(FamNode)
                if FamName==familyName:
                    stagPNode =  Internal.getNodeFromName(zsr,'Pressure')    
                    sizeIBC = numpy.shape(stagPNode[1])
                    Internal.setValue(stagPNode,P_static*numpy.ones(sizeIBC))
    return None


# Set the IBC type inj for zones in familyName
def initInj(tc, familyName, P_tot, H_tot, injDir=[1.,0.,0.]):
    """Set the total pressure P_tot, total enthalpy H_tot, and direction of the flow injDir for the injection IBC with family name familyName.
    Usave: initInj(tc, familyName, P_tot, H_tot, injDir=[1.,0.,0.])"""
    tc2 = Internal.copyRef(tc)
    _initInj(tc2, familyName, P_tot, H_tot, injDir)
    return tc2
                 

def _initInj(tc, familyName, P_tot, H_tot, injDir=[1.,0.,0.]):
    """Set the stagnation pressure P_tot, stagnation enthalpy H_tot, and direction of the flow injDir for the injection IBC with family name familyName)
    Usave: initInj(tc, familyName, P_tot, H_tot, injDir=[1.,0.,0.])"""
    for zc in Internal.getZones(tc):
        for zsr in Internal.getNodesFromName(zc,'IBCD_5_*'):
            FamNode = Internal.getNodeFromType1(zsr,'FamilyName_t')
            if FamNode is not None:
                FamName = Internal.getValue(FamNode)
                if FamName==familyName:
                    stagPNode =  Internal.getNodeFromName(zsr,'StagnationPressure')
                    stagHNode =  Internal.getNodeFromName(zsr,'StagnationEnthalpy')
                    dirxNode = Internal.getNodeFromName(zsr,'dirx')
                    diryNode = Internal.getNodeFromName(zsr,'diry')
                    dirzNode = Internal.getNodeFromName(zsr,'dirz')
                    sizeIBC = numpy.shape(stagHNode[1])
                    Internal.setValue(stagHNode,H_tot*numpy.ones(sizeIBC))
                    Internal.setValue(stagPNode,P_tot*numpy.ones(sizeIBC))

                    Internal.setValue(dirxNode, injDir[0]*numpy.ones(sizeIBC))
                    Internal.setValue(diryNode, injDir[1]*numpy.ones(sizeIBC))
                    Internal.setValue(dirzNode, injDir[2]*numpy.ones(sizeIBC))
                    
    return None


# Change IBC Types
def add_variables_tc_ibc(zsr,ibctype,nIBC):
    Nlength = numpy.zeros((nIBC),numpy.float64)
    if ibctype in [2, 3, 6, 10, 11]:
        zsr[2].append(['utau' , Nlength, [], 'DataArray_t'])
        zsr[2].append(['yplus', Nlength, [], 'DataArray_t'])

    if ibctype == 5:
        Internal._createChild(zsr, 'StagnationEnthalpy', 'DataArray_t', value=Nlength)
        Internal._createChild(zsr, 'StagnationPressure', 'DataArray_t', value=Nlength)
        Internal._createChild(zsr, 'dirx'              , 'DataArray_t', value=Nlength)
        Internal._createChild(zsr, 'diry'              , 'DataArray_t', value=Nlength)
        Internal._createChild(zsr, 'dirz'              , 'DataArray_t', value=Nlength)

    if ibctype == 10 or ibctype == 11:
        zsr[2].append(['gradxPressure' , Nlength , [], 'DataArray_t'])
        zsr[2].append(['gradyPressure' , Nlength , [], 'DataArray_t'])
        zsr[2].append(['gradzPressure' , Nlength , [], 'DataArray_t'])

        if ibctype == 11:
            zsr[2].append(['gradxVelocityX' , Nlength , [], 'DataArray_t'])
            zsr[2].append(['gradyVelocityX' , Nlength , [], 'DataArray_t'])
            zsr[2].append(['gradzVelocityX' , Nlength , [], 'DataArray_t'])
            
            zsr[2].append(['gradxVelocityY' , Nlength , [], 'DataArray_t'])
            zsr[2].append(['gradyVelocityY' , Nlength , [], 'DataArray_t'])
            zsr[2].append(['gradzVelocityY' , Nlength , [], 'DataArray_t'])
            
            zsr[2].append(['gradxVelocityZ' , Nlength , [], 'DataArray_t'])
            zsr[2].append(['gradyVelocityZ' , Nlength , [], 'DataArray_t'])
            zsr[2].append(['gradzVelocityZ' , Nlength , [], 'DataArray_t'])
        
    if ibctype == 100:
        zsr[2].append(["KCurv" , Nlength , [], 'DataArray_t'])
        
    return zsr


def changeIBCType(tc, oldIBCType, newIBCType):
    """Change the IBC type in a connectivity tree from oldIBCType to newIBCType.
    Usave: changeIBCType(tc, oldIBCType, newIBCType)"""
    for z in Internal.getZones(tc):
        subRegions = Internal.getNodesFromType1(z, 'ZoneSubRegion_t')
        for zsr in subRegions:
            nameSubRegion = zsr[0]
            if nameSubRegion[:4] == "IBCD":
                ibcType = int(nameSubRegion.split("_")[1])
                if ibcType == oldIBCType:
                    zsr[0] = "IBCD_{}_".format(newIBCType)+"_".join(nameSubRegion.split("_")[2:])

                    pressure = Internal.getNodeFromName(zsr, 'Pressure')[1]
                    nIBC = pressure.shape[0]

                    for var_local in vars_delete_ibm:
                        Internal._rmNodesByName(zsr,var_local)
                    

                    zsr=add_variables_tc_ibc(zsr,newIBCType,nIBC)

    return tc


def transformTc2(tc2):
    for z in Internal.getZones(tc2):
        subRegions = Internal.getNodesFromType1(z, 'ZoneSubRegion_t')
        for zsr in subRegions:
            nameSubRegion = zsr[0]
            if nameSubRegion[:6] == "2_IBCD":
                ibctype = int(nameSubRegion.split("_")[2])
                zsr[0] = "IBCD_{}_".format(ibctype)+"_".join(nameSubRegion.split("_")[3:])

                pressure = Internal.getNodeFromName(zsr, 'Pressure')[1]
                nIBC = pressure.shape[0]
                
                vars_delete = ['Density','VelocityX','VelocityY','VelocityZ']+vars_delete_ibm
                for var_local in vars_delete:
                    Internal._rmNodesByName(zsr,vars_delete)

                Nlength = numpy.zeros((nIBC),numpy.float64)
                zsr[2].append(['Density'    , Nlength , [], 'DataArray_t'])
                zsr[2].append(['VeloicityX' , Nlength , [], 'DataArray_t'])
                zsr[2].append(['VeloicityY' , Nlength , [], 'DataArray_t'])
                zsr[2].append(['VeloicityZ' , Nlength , [], 'DataArray_t'])

                zsr=add_variables_tc_ibc(zsr,ibctype,nIBC)
                
    return tc2


