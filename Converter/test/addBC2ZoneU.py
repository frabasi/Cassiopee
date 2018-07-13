# - addBC2Zone (pyTree) -
import Converter.PyTree as C
import Generator.PyTree as G

# - NGons -
a = G.cartNGon((2,0,0), (0.1,0.1,1), (10,10,2))
a = C.addBC2Zone(a, 'wall', 'BCWall', faceList=[1,2])

t = C.newPyTree(['Base', a])
C.convertPyTree2File(t, 'out.cgns')
