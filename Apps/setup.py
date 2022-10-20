#!/usr/bin/env python
from distutils.core import setup
import os
import KCore.Dist as Dist

#=============================================================================
# Apps requires:
# ELSAPROD variable defined in environment
# CASSIOPEE
#=============================================================================

prod = os.getenv("ELSAPROD")
if prod is None: prod = 'xx'
    
# setup ======================================================================
setup(
    name="Apps",
    version="3.5",
    description="Application modules",
    author="ONERA",
    packages=['Apps', 'Apps.Chimera', 'Apps.Fast', 'Apps.Mesh', 'Apps.Coda'],
    package_dir={"":"."}
    )

# Check PYTHONPATH ===========================================================
Dist.checkPythonPath(); Dist.checkLdLibraryPath()
