# - distribute (pyTree) -
# - match -
import Generator.PyTree as G
import Distributor2.PyTree as D2
import Converter.PyTree as C
import Connector.PyTree as X
import KCore.test as test
import Converter.Mpi as Cmpi

LOCAL = test.getLocal()
N = 11

# Cas test
t = C.newPyTree(['Base'])
off = 0
for i in range(N):
    a = G.cart( (off,0,0), (1,1,1), (10+i, 10, 10) )
    off += 9+i
    t[2][1][2].append(a)
t = X.connectMatch(t)
if Cmpi.rank == 0: C.convertPyTree2File(t, LOCAL+'/in.cgns')
Cmpi.barrier()

# arbre complet (reference)
if Cmpi.rank == 0:
    t, stats = D2.distribute(t, NProc=Cmpi.size, algorithm='gradient', useCom='match')
    print ('full:', stats)
    test.testT(t, 1)
Cmpi.barrier()

# distribution sur arbre squelette
t = Cmpi.convertFile2SkeletonTree(LOCAL+'/in.cgns')
t, stats = D2.distribute(t, NProc=Cmpi.size, algorithm='gradient', useCom='match')
if Cmpi.rank == 0:
    print('skel:', stats)
    test.testT(t, 2)

# distribution sur arbre squelette charge
t = Cmpi.convertFile2SkeletonTree(LOCAL+'/in.cgns')
t, stats = D2.distribute(t, NProc=Cmpi.size, algorithm='fast', useCom=0)
t = Cmpi.readZones(t, 'in.cgns', rank=Cmpi.rank)
t, stats = D2.distribute(t, NProc=Cmpi.size, algorithm='gradient', useCom='match')
if Cmpi.rank == 0:
    print('loaded skel:', stats)
    test.testT(t, 3)
