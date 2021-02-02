import Generator.PyTree as G
import Transform.PyTree as T
import Converter.PyTree as C
import Converter.Internal as I
import Intersector.PyTree as XOR
import KCore.test as test
import Post.PyTree as P

t1 = G.cart((0,0,0), (1,1,1), (10,10,10))
t1 = C.convertArray2NGon(t1); t1 = G.close(t1)
t2 = G.cart((1.,1.5,3.), (1,1,1), (10,10,10))
t2 = C.convertArray2NGon(t2); t2 = G.close(t2)

res = XOR.getCollidingCells(t1, t2, RTOL = 0.05)

m = XOR.getCells(t1, res[0], are_face_ids=False)

test.testT(m,1)
