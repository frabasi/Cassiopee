import KCore.Dist as Dist
from KCore.config import *

SCOTCH=True; PARADIGMA=True; ZOLTAN=False

#==============================================================================
# Fichiers c++
#==============================================================================
cpp_srcs = ['XCore/CMP/src/recv_buffer.cpp', 
            'XCore/CMP/src/send_buffer.cpp',
            'XCore/xmpi/context_mpi_impl.cpp',
            'XCore/xmpi/context_stub_impl.cpp',
            'XCore/xmpi/communicator.cpp',
            'XCore/SplitElement/splitter.cpp',
            'XCore/test/xmpi_t1.cpp']
