import KCore.Dist as Dist
from KCore.config import *
mpi, mpiIndDic, mpiLibDir = Dist.checkMpi(additionalLibPaths, additionalIncludePaths)
#==============================================================================
# Fichiers C++
#==============================================================================
cpp_srcs = ['Connector/KInterp/BlkInterp.cpp',
            'Connector/KInterp/KMesh.cpp',
            'Connector/KInterp/BlkIntTreeNode.cpp',
            'Connector/KInterp/BlkInterpData.cpp',
            'Connector/KInterp/BlkInterpDataStruct.cpp',        
            'Connector/KInterp/BlkInterpWithKMesh.cpp',
            'Connector/KInterp/BlkInterpAdt.cpp',
            'Connector/KInterp/BlkInterpAdt_getCell.cpp',
            'Connector/optimizeOverlap.cpp',
            'Connector/maximizeBlankedCells.cpp',
            'Connector/maskXRay.cpp',
            'Connector/maskGen.cpp',
            'Connector/blankCells.cpp',
            'Connector/blankCellsTetra.cpp',
            'Connector/getIntersectingDomainsAABB.cpp',
            'Connector/changeWall.cpp',
            'Connector/changeWallEX.cpp',
            'Connector/setDoublyDefinedBC.cpp',
            'Connector/getInterpolatedPoints.cpp',
            'Connector/getInterpolatedEXPoints.cpp',
            'Connector/setInterpolations.cpp',
            'Connector/setInterpData.cpp',
            'Connector/setInterpDataCons.cpp',
            'Connector/setInterpDataForGhostCells.cpp',
            'Connector/initNuma.cpp',
            'Connector/setInterpTransfers.cpp',
            'Connector/setInterpTransfersD.cpp',
            'Connector/IBC/setIBCTransfersD.cpp',
            'Connector/IBC/blankClosestTargetCells.cpp',
            'Connector/writeCoefs.cpp',
            'Connector/chimeraTransfer.cpp',
            'Connector/transferVariables.cpp',
            'Connector/blankIntersectingCells.cpp',
            'Connector/cellN2OversetHoles.cpp',
            'Connector/identifyMatching.cpp',
            'Connector/identifyDegenerated.cpp',
            'Connector/gatherMatching.cpp',
            'Connector/gatherMatchingNM.cpp',
            'Connector/gatherMatchingNGon.cpp',
            'Connector/gatherDegenerated.cpp',
            'Connector/IBC/setIBCTransfers.cpp',
            'Connector/setInterpDataLS.cpp',
            'Connector/modifyBorders.cpp',
            'Connector/applyBCOverlaps.cpp',
            "Connector/getExtrapAbsCoefs.cpp",
            "Connector/getEmptyBCInfoNGON.cpp",
            "Connector/updateNatureForIBM.cpp",
            "Connector/getIBMPtsWithFront.cpp",
            "Connector/getIBMPtsWithoutFront.cpp",
            "Connector/getIBMPtsBasic.cpp"
            ]
if mpi is True:
    cpp_srcs  += [
            "Connector/CMP/src/recv_buffer.cpp", 
            "Connector/CMP/src/send_buffer.cpp"
            ]

#==============================================================================
# Fichiers fortran
#==============================================================================
for_srcs = ['Connector/Fortran/spalart_1d.for',
            'Connector/Fortran/CompMotionCentersF.for',
            'Connector/Fortran/CompMotionCentersEXF.for',
            'Connector/Fortran/BlkAdjustCellNatureFieldF.for',
            'Connector/Fortran/MaskSearchBlankedNodesXF.for',
            'Connector/Fortran/MaskSearchBlankedNodesX2DF.for',
            'Connector/Fortran/MaskSearchBlankedNodesXDF.for',
            'Connector/Fortran/MaskSearchBlankedNodesXD2DF.for',
            'Connector/Fortran/MaskSearchBlankedCellsX2DF.for',
            'Connector/Fortran/MaskSearchBlankedCellsXF.for',
            'Connector/Fortran/MaskSearchBlankedCellsX12DF.for',
            'Connector/Fortran/MaskSearchBlankedCellsX1F.for',
            'Connector/Fortran/MaskSearchBlankedCellsX22DF.for',
            'Connector/Fortran/MaskSearchBlankedCellsX2F.for',
            'Connector/Fortran/MaskSearchBlankedCellsXDF.for',
            'Connector/Fortran/MaskSearchBlankedCellsXD2DF.for',
            'Connector/Fortran/MaskSearchBlankedCellsXD2F.for',
            'Connector/Fortran/MaskSearchBlankedCellsXD22DF.for',
            'Connector/Fortran/MaskSearchBlankedCellsXD1F.for',
            'Connector/Fortran/MaskSearchBlankedCellsXD12DF.for',
            'Connector/Fortran/MaskSearchBlankedCellsUnstrXF.for',
            'Connector/Fortran/MaskSearchBlankedCellsUnstrXDF.for',
            'Connector/Fortran/MaskSearchBlankedCellsUnstrX2F.for']
