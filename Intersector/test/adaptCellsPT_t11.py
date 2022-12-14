# - adapts a cells with respect to b points (array) -
import Intersector.PyTree as XOR
import Converter.PyTree as C
import Generator.PyTree as G
import Converter.Internal as I
import Transform.PyTree as T
import Connector.PyTree as X
import numpy
import KCore.test as test


N = 5
t = G.cartHexa((0.,0.,0.), (0.1,0.1,0.1), (N,2,2))

t = C.convertArray2NGon(t); t = G.close(t)

t = C.fillEmptyBCWith(t, 'wall', 'BCWall')
t = C.initVars(t, '{centers:Density} = {centers:CoordinateX} + {centers:CoordinateY}')

zs = I.getZones(t)
n = C.getNCells(zs[0])

cv0 = numpy.empty((n,), dtype=I.E_NpyInt)
cv0[:] = 0
cv0[0]=4

cv = []
cv.append(cv0)

#C.convertPyTree2File(t, 't.cgns')
ta = XOR.adaptCells(t,cv, sensor_type=3, subdiv_type=3)## DIR
ta = XOR.closeCells(ta)

test.testT(ta,1)
#C.convertPyTree2File(ta, 'PT_t11.cgns')








