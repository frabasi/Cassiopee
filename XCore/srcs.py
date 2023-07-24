import KCore.Dist as Dist
from KCore.config import *

SCOTCH=True; ZOLTAN=True
# 0: None, 1: paradigma, 2: paradigma23
PARADIGMA=1

#==============================================================================
# Fichiers c++
#==============================================================================
cpp_srcs = ['XCore/CMP/src/recv_buffer.cpp', 
            'XCore/CMP/src/send_buffer.cpp',
            'XCore/xmpi/context_mpi_impl.cpp',
            'XCore/xmpi/context_stub_impl.cpp',
            'XCore/xmpi/communicator.cpp',
            'XCore/zoltan1.cpp',
            'XCore/chunk2part.cpp',
            'XCore/SplitElement/splitter.cpp',
            'XCore/test/xmpi_t1.cpp']
