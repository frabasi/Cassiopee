#!/usr/bin/env python
import os
from distutils.core import setup, Extension

#=============================================================================
# Compressor requires:
# C++ compiler
# Numpy
# KCore
#=============================================================================

# Write setup.cfg
import KCore.Dist as Dist
Dist.writeSetupCfg()

# Test if numpy exists =======================================================
(numpyVersion, numpyIncDir, numpyLibDir) = Dist.checkNumpy()

# Test if kcore exists =======================================================
(kcoreVersion, kcoreIncDir, kcoreLibDir) = Dist.checkKCore()

# Setting libraryDirs and libraries ===========================================
prod = os.getenv("ELSAPROD")
if prod is None: prod = 'xx'
libraryDirs = ['build/'+prod, kcoreLibDir]
libraries = ["compressor", "kcore"]
from KCore.config import *
(ok, libs, paths) = Dist.checkCppLibs([], additionalLibPaths)
libraryDirs += paths; libraries += libs

import srcs

# Extensions =================================================================
extensions = [
    Extension('Compressor.compressor',
              sources=["Compressor/compressor.cpp"],
              include_dirs=["Compressor"]+additionalIncludePaths+[numpyIncDir, kcoreIncDir],
              library_dirs=additionalLibPaths+libraryDirs,
              libraries=libraries+["zstd"]+additionalLibs,
              extra_compile_args=Dist.getCppArgs(),
              extra_link_args=Dist.getLinkArgs())]
if srcs.SZ:
  mySystem = Dist.getSystem()
  if mySystem[0] == 'mingw':
    if Dist.useStatic() == False: additionalLibs += ["zlib1"]
    else: additionalLibs += ["z"]
  else: additionalLibs += ["z"]

  extensions += [
    Extension('Compressor.sz.csz',
               sources=["Compressor/sz/compressor.cpp"],
               include_dirs=["Compressor", "Compressor/sz/include"]+additionalIncludePaths+[numpyIncDir, kcoreIncDir],
               library_dirs=additionalLibPaths+libraryDirs,
               libraries=libraries+["sz", "zstd"]+additionalLibs,
               extra_compile_args=Dist.getCppArgs(),
               extra_link_args=Dist.getLinkArgs())]
if srcs.ZFP:
  extensions += [
    Extension('Compressor.zfp.czfp',
              sources=["Compressor/zfp/compressor.cpp"],
              include_dirs=["Compressor", "Compressor/zfp/include"]+additionalIncludePaths+[numpyIncDir, kcoreIncDir],
              library_dirs=additionalLibPaths+libraryDirs,
              libraries=libraries+["zfp"]+additionalLibs,
              extra_compile_args=Dist.getCppArgs(),
              extra_link_args=Dist.getLinkArgs())]

# Setup ======================================================================
setup(
    name="Compressor",
    version="3.3",
    description="Compress CFD solutions.",
    author="Onera",
    package_dir={"":"."},
    packages=['Compressor', 'Compressor.sz', 'Compressor.zfp'],
    #packages=['Compressor', 'Compressor.zfp'],
    ext_modules=extensions
    )

# Check PYTHONPATH ===========================================================
Dist.checkPythonPath(); Dist.checkLdLibraryPath()
