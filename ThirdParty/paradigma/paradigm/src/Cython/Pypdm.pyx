#!python
##cython: language_level=3
##cython: boundscheck=False
#cython: boundscheck=True
#cython: cdivision=True
#cython: wraparound=False
#cython: profile=True
###cython: embedsignature=False
# See : https://cython.readthedocs.io/en/latest/src/reference/compilation.html#compiler-directives

# Import Section (Python) :
# ------------------------
import logging   as LOG
import numpy     as NPY

# Import Section (Cython) :
# ------------------------
from    libc.stdlib cimport malloc, free
from    cpython.ref cimport PyObject, Py_INCREF, Py_DECREF, Py_XDECREF
cimport cython
cimport numpy       as      NPY

#from    mpi4py      cimport MPI
cimport mpi4py.MPI as MPI

# -----------------------------------------------------------------
cdef extern from "numpy/arrayobject.h":
  void PyArray_ENABLEFLAGS(NPY.ndarray arr, int flags)

# MANDATORY :
# ---------
NPY.import_array()

# PDM Specific type :
# -----------------

ctypedef NPY.int32_t npy_pdm_gnum_t
ctypedef NPY.int32_t npy_pdm_lnum_t
npy_pdm_gnum_dtype = NPY.int32

ctypedef int PDM_g_num_t
ctypedef int PDM_l_num_t

ctypedef int PDM_MPI_Comm

ctypedef enum PDM_g_num_npy_t:
  PDM_G_NUM_NPY_INT = NPY.NPY_INT32

# -----------------------------------------------------------------

cdef extern from "pdm_config.h":
    cdef int PDM_IN_PDMA_BOOL

PDM_IN_PDMA = PDM_IN_PDMA_BOOL

cdef extern from "pdm.h":
    ctypedef enum PDM_data_t:
        PDM_INT
        PDM_DOUBLE

    ctypedef enum PDM_stride_t:
        PDM_STRIDE_CST
        PDM_STRIDE_VAR

    ctypedef enum PDM_mesh_nature_t:
        PDM_MESH_NATURE_NODAL
        PDM_MESH_NATURE_SURFACE_MESH

    ctypedef enum PDM_mesh_entities_t:
        PDM_MESH_ENTITY_CELL
        PDM_MESH_ENTITY_FACE
        PDM_MESH_ENTITY_EDGE
        PDM_MESH_ENTITY_VERTEX

    ctypedef enum PDM_bool_t:
        PDM_FALSE = 0
        PDM_TRUE  = 1

    void PDM_Finalize ()

def Finalize ():
    PDM_Finalize

cdef extern from "pdm_mpi.h":
    ctypedef int PDM_MPI_Comm
    PDM_MPI_Comm PDM_MPI_mpi_2_pdm_mpi_comm(void *pt_comm)
# -----------------------------------------------------------------


#cdef extern from "pdm_plugin.h":
#    void PDM_plugin_load ()
#PDM_plugin_load()


# PDM_part :
# ----------
include "pdm_part.pxi"

# PDM_part_coarse :
# ----------
include "pdm_part_coarse.pxi"

# PDM_part_to_block :
# ------------------
include "pdm_part_to_block.pxi"

# PDM_part_to_block :
# ------------------
include "pdm_block_to_part.pxi"

# PDM_block_to_block :
# ------------------
include "pdm_block_to_block.pxi"

# PDM_cellface_orient :
# --------------------
include "pdm_cellface_orient.pxi"

# PDM_dmesh_nodal :
# ----------------
include "pdm_dmesh_nodal.pxi"

# PDM_dist_cloud_surf :
# --------------------
include "pdm_dist_cloud_surf.pxi"

# PDM_dist_cloud_surf :
# --------------------
#include "pdm_dist_cellcenter_surf.pxi"







# TODO: PDM_writer, ...
