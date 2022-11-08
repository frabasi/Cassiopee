# Interface pour MPI

import Converter.Mpi as Cmpi
from . import PyTree as P
import Converter.Internal as Internal
import Converter.PyTree as C
import KCore.Vector as Vector
import numpy

#==============================================================================
# extractMesh
# IN: t: maillage source distribue
# IN: extractionMesh: maillage de destination distribue
# IN: graph: graph d'intersection si deja calcule
#==============================================================================
def extractMesh(t, extractionMesh, order=2, extrapOrder=1,
                constraint=40., tol=1.e-6, mode='robust', hook=None, graph=None):
    if graph is None:
        tb = Cmpi.createBBoxTree(t)
        tb2 = Cmpi.createBBoxTree(extractionMesh)
        graph = Cmpi.computeGraph(tb, type='bbox3', t2=tb2)
    tl = Cmpi.addXZones(t, graph)
    tl = Cmpi.convert2PartialTree(tl)
    ext = Cmpi.convert2PartialTree(extractionMesh)
    # print info
    nztl = len(Internal.getZones(tl))
    nzext = len(Internal.getZones(ext))
    print('Rank %d has %d source zones and %d destination zones.'%(Cmpi.rank, nztl, nzext))
    ext = P.extractMesh(tl, ext, order=order, extrapOrder=extrapOrder, constraint=constraint, tol=tol, mode=mode,
                        hook=hook)
    return ext

#==============================================================================
def integ(t, var=''):
    """Integral of fields defined in t."""
    if t is not None:
        ret = P.integ(t, var)
    else:
        ret = 0.
    ret  = numpy.array(ret, dtype=numpy.float64)
    ret1 = numpy.empty(ret.shape, dtype=numpy.float64)
    Cmpi.Allreduce(ret, ret1, Cmpi.SUM)
    return ret1.tolist()

#==============================================================================
def integNorm(t, var=''):
    """Integral of fields times normal."""
    if t is not None:
        ret = P.integNorm(t, var)
    else:
        ret = 0.
    ret  = numpy.array(ret, dtype=numpy.float64)
    ret1 = numpy.empty(ret.shape, dtype=numpy.float64)
    Cmpi.Allreduce(ret, ret1, Cmpi.SUM)
    return [ret1.tolist()]

#==============================================================================
def integNormProduct(t, vector=[]):
    if t is not None:
        ret = P.integNormProduct(t, vector)
    else:
        ret = 0.    
    ret = numpy.array(ret, dtype=numpy.float64)
    ret1 = numpy.empty(ret.shape, dtype=numpy.float64)
    Cmpi.Allreduce(ret, ret1, Cmpi.SUM)
    return ret1.tolist()

#==============================================================================
def integMoment(t, center=(0.,0.,0.), vector=[]):
    if t is not None:
        ret = P.integMoment(t, center, vector)
    else:
        ret = 0.    
    ret = numpy.array(ret, dtype=numpy.float64)
    ret1 = numpy.empty(ret.shape, dtype=numpy.float64)
    Cmpi.Allreduce(ret, ret1, Cmpi.SUM)
    return ret1.tolist()

#==============================================================================
def integNormMoment(t, center=(0.,0.,0.), vector=[]):
    if t is not None:
        ret = P.integNormMoment(t, center, vector)
    else:
        ret = 0.    
    ret = numpy.array(ret, dtype=numpy.float64)
    ret1 = numpy.empty(ret.shape, dtype=numpy.float64)
    Cmpi.Allreduce(ret, ret1, Cmpi.SUM)
    return ret1.tolist()

#=============================================================================
# Parallel streamline2 : dans la direction de l'ecoulement uniquement
def streamLine2(t, X0, vector, N=2000, eps=1.e-2, maxCompt=20):
    """Compute a streamline starting from (x0,y0,z0) given
    a list of arrays containing 'vector' information."""
    
    out = []; compt = 0

    while len(X0) > 0 and compt < maxCompt:
        ret = P.streamLine2(t, X0, vector, N=N, dir=1, eps=eps)
        for z in ret: z[0] = z[0]+'_%d'%Cmpi.rank
        
        # Get new pool (supprime les streamlines degenerees)
        X0 = []; ret2 = []
        for z in ret:
            P0 = C.getValue(z, 'GridCoordinates', -1)
            P1 = C.getValue(z, 'GridCoordinates', 1)
            dP = Vector.sub(P0, P1)
            l = Vector.norm2(dP)
            if l >= 1.e-10:
                Pts = P0 # last point
                X0.append(tuple(Pts))
                ret2.append(z)
        #print('>> New pool', X0)
        out += ret2
    
        # Communicate and merge pool
        b = Cmpi.allgather(X0)
        X0 = []
        for i in b: X0 += i
        #print('>> New pool after com', X0)
        print('it=%d pool length=%d'%(compt,len(X0)))
        
        compt += 1

    return out
