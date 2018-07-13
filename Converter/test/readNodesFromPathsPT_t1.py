# - readNodesFromPaths (pyTree) -
import Converter.PyTree as C
import Converter.Filter as Filter
import Converter.Internal as Internal
import Generator.PyTree as G
import KCore.test as test

# Cree le fichier test
a = G.cart((0,0,0), (1,1,1), (10,10,10))
b = G.cart((12,0,0), (1,1,1), (10,10,10))
t = C.newPyTree(['Base',a,b])
C.convertPyTree2File(t, 'test.adf')

# Relit les noeuds par leur paths
nodes = Filter.readNodesFromPaths('test.adf', '/Base/cart/GridCoordinates')
test.testT(nodes, 1)
