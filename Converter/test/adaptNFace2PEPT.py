# - adaptNFace2PE (pyTree) -
import Converter.PyTree as C
import Converter.Internal as Internal
import Generator.PyTree as G

# NGon (CGNSv3)
a = G.cartNGon((0,0,0), (1,1,1), (10,10,10))
a = Internal.adaptNFace2PE(a, remove=False)
C.convertPyTree2File(a, 'out.cgns')

# NGon (CGNSv4)
a = G.cartNGon((0,0,0), (1,1,1), (10,10,10), api=3)
a = Internal.adaptNFace2PE(a, remove=False)
C.convertPyTree2File(a, 'out2.cgns')
