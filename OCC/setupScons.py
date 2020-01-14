#!/usr/bin/env python
from distutils.core import setup, Extension
from KCore.config import *
import os, sys

#=============================================================================
# OCC requires:
# ELSAPROD variable defined in environment
# C++ compiler
# KCore library
#=============================================================================

# Write setup.cfg
import KCore.Dist as Dist
Dist.writeSetupCfg()

# Test if kcore exists =======================================================
(kcoreVersion, kcoreIncDir, kcoreLibDir) = Dist.checkKCore()

# Test if generator exists ===================================================
(generatorVersion, generatorIncDir, generatorLibDir) = Dist.checkGenerator()
    
# Test if open-cascade is installed ===========================================
(OCEPresent, OCEIncDir, OCELibDir) = Dist.checkOCE(additionalLibPaths, 
                                                   additionalIncludePaths)

# Compilation des fortrans ===================================================
prod = os.getenv("ELSAPROD")
if prod is None: prod = 'xx'

# Setting libraryDirs and libraries ===========================================
libraryDirs = ["build/"+prod, kcoreLibDir, generatorLibDir]
includeDirs = [kcoreIncDir, generatorIncDir]
libraries = ["occ_cassiopee", "generator", "kcore"]
if OCEPresent:
    libOCE = ["TKBin", "TKBinL", "TKBinTObj", "TKBinXCAF", "TKBO",
              "TKBool", "TKBRep", "TKCAF", "TKCDF", "TKernel",
              "TKFeat", "TKFillet", "TKG2d", "TKG3d", "TKGeomAlgo",
              "TKGeomBase", "TKHLR", "TKIGES", "TKLCAF", "TKMath",
              "TKMesh", "TKMeshVS", "TKNIS", "TKOffset", "TKOpenGl", 
              "TKPCAF", "TKPLCAF", "TKPrim", "TKPShape", "TKService",
              "TKShapeSchema", "TKShHealing", "TKStdLSchema",
              "TKStdSchema", "TKSTEP", "TKSTEP209", "TKSTEPAttr",
              "TKSTEPBase", "TKSTL", "TKTObj", "TKTopAlgo",
              "TKV3d", "TKVoxel", "TKVRML", "TKXCAF", "TKXCAFSchema",
              "TKXDEIGES", "TKXDESTEP", "TKXMesh", "TKXml",
              "TKXmlL", "TKXmlTObj", "TKXmlXCAF", "TKXSBase"]
    libOCE = [i+".dll" for i in libOCE]
else:
    libOCE = ["TKIGES", "TKXSBase", "TKShHealing", "TKTopAlgo", "TKPrim", "TKBool", "TKBool2", "TKBool3", "TKBool4", "TKGeomAlgo", "TKBRep", "TKBRep2", "TKGeomBase", "TKG3d", "TKMath", "TKernel", "TKG2d"]
libraries += libOCE + libOCE

(ok, libs, paths) = Dist.checkFortranLibs([], additionalLibPaths)
libraryDirs += paths; libraries += libs
(ok, libs, paths) = Dist.checkCppLibs([], additionalLibPaths)
libraryDirs += paths; libraries += libs

# setup ======================================================================
setup(
    name="OCC",
    version="3.1",
    description="OpenCascade python module.",
    author="Onera",
    package_dir={"":"."},
    packages=['OCC'],
    ext_modules=[Extension('OCC.occ',
                           sources=["OCC/occ.cpp"],
                           include_dirs=["OCC"]+additionalIncludePaths+includeDirs,
                           library_dirs=additionalLibPaths+libraryDirs,
                           libraries=libraries+additionalLibs,
                           extra_compile_args=Dist.getCppArgs(),
                           extra_link_args=Dist.getLinkArgs()
                           )]
    )

# Check PYTHONPATH ===========================================================
Dist.checkPythonPath(); Dist.checkLdLibraryPath()
