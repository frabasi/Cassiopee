# Installation de libxcore pour etre accessible par les autres modules
# Si libxcore.a existe, on la recopie
# Sinon, on cherche xcore.so ou xcore.pyd, on le recopie en 
# libxcore.so ou dll
import os, shutil
import platform
system = platform.uname()[0]

if system == 'Windows':
    __EXTMODULE__ = '.pyd'
    __EXTSHARED__ = '.dll'
else:
    __EXTMODULE__ = '.so'
    __EXTSHARED__ = '.so'

import KCore.installPath as K
libPath = K.libPath
installPathLocal = K.installPath

# La librarie statique existe?
a = os.access(installPathLocal+"/XCore/libxcore.a", os.F_OK)
if a:
    shutil.copy(installPathLocal+"/XCore/libxcore.a", libPath+"/libxcore.a")
else: # Essai en dynamique
    a = os.access(installPathLocal+"/XCore/xcore"+__EXTMODULE__, os.F_OK)
    if a:
        shutil.copy(installPathLocal+"/XCore/xcore"+__EXTMODULE__,
                    libPath+"/libxcore"+__EXTSHARED__) 
    else:
        print("Error: xcore"+__EXTMODULE__+" can not be found.")
