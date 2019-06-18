import Geom.PyTree as D
import Converter.PyTree as C
import Intersector.PyTree as XOR
import Generator.PyTree as G
import Transform.PyTree as T
import Post.PyTree as P
import Converter.Internal as Internal
import time
import KCore.test as test

a = D.triangle((0,0,0), (0,1,0), (1, 0, 0))
d = G.cart((1,0.,0.), (1,1,1),(2,1,1)) 
a = G.addNormalLayers(a, d)
a = C.convertArray2NGon(a)
a = G.close(a)

b = G.cart((0.2,0.2,-0.5), (0.01,0.01,0.01),(5,5,5)) 
b = C.convertArray2NGon(b)
b = G.close(b)

#C.convertPyTree2File(a,'nonreg_prism.plt')
#C.convertPyTree2File(b,'nonreg_prism_source.plt')

m1 = XOR.adaptCells(a,b, sensor_type=0)
m1 = XOR.closeOctalCells(m1)
C.convertPyTree2File(m1, 'out.cgns')
test.testT(m1,1)

m2 = XOR.adaptCells(a,b, sensor_type=2)
m2 = XOR.closeOctalCells(m2)
C.convertPyTree2File(m2, 'out1.cgns')
test.testT(m2,2)

