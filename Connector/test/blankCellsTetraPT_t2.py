# - blankCellsTetra (pyTree)
import Converter.PyTree as C
import Connector.PyTree as X
import Generator.PyTree as G
import Geom.PyTree as D
import KCore.test as test
import sys

# Test 1
# Tet mask
mT4 = G.cart((0.,0.,0.), (0.1,0.1,0.2), (10,10,10))
mT4 = C.convertArray2Tetra(mT4)
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
# celln init
t = C.initVars(t, 'nodes:cellN', 1.)
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="node_in", tol=1.e-12, cellnval=2, overwrite=1)
#C.convertPyTree2File(t, 'out1.cgns')
test.testT(t,1)

# Test 2
# Tet mask
mT4 = G.cart((0.,0.,0.), (0.1,0.1,0.2), (10,10,10))
mT4 = C.convertArray2Tetra(mT4)
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
# celln init
t = C.initVars(t, 'centers:cellN', 1.)
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="center_in", tol=1.e-12, cellnval=2, overwrite=1)
#C.convertPyTree2File(t, 'out2.cgns')
test.testT(t,2)

# Test 3
# Tet mask
mT4 = G.cart((0.,0.,0.), (0.1,0.1,0.2), (10,10,10))
mT4 = C.convertArray2Tetra(mT4)
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
# celln init
t = C.initVars(t, 'centers:cellN', 1.)
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="cell_intersect", tol=1.e-12, cellnval=2, overwrite=0)
#C.convertPyTree2File(t, 'out3.cgns')
test.testT(t,3)

# Test 4
# Tet mask
mT4 = D.sphere((0,0,0), 20., 50)
mT4 = C.convertArray2Tetra(mT4)
mT4 = G.close(mT4)
mT4 = G.tetraMesher(mT4, algo=1)
#C.convertPyTree2File(mT4, 'sph.cgns')
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
# celln init
t = C.initVars(t, 'nodes:cellN', 1.)
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="node_in", tol=1.e-12, cellnval=2, overwrite=1)
#C.convertPyTree2File(t, 'out4.cgns')
test.testT(t,4)

# Test 5
# Tet mask
#C.convertPyTree2File(mT4, 'sph.cgns')
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
# celln init
t = C.initVars(t, 'centers:cellN', 1.)
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="center_in", tol=1.e-12, cellnval=2, overwrite=1)
#C.convertPyTree2File(t, 'out5.cgns')
test.testT(t,5)

# Test 6
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
# celln init
t = C.initVars(t, 'centers:cellN', 1.)
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="cell_intersect", tol=1.e-12, cellnval=2, overwrite=1)
#C.convertPyTree2File(t, 'out6.cgns')
test.testT(t,6)

# Test 7
# Tet mask
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
t = C.convertArray2Tetra(t)
# celln init
t = C.initVars(t, 'nodes:cellN', 1.)
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="node_in", tol=1.e-12, cellnval=2, overwrite=1)
#C.convertPyTree2File(t, 'out7.cgns')
test.testT(t,7)
# Test 8
# Tet mask
# Mesh to blank
a = G.cart((-5.,-5.,-5.), (0.5,0.5,0.5), (100,100,100))
t = C.newPyTree(['Cart', a])
t = C.initVars(t, 'nodes:cellN', 1.)
# celln init
t = C.convertArray2NGon(t);
# Blanking
t = X.blankCellsTetra(t, [[mT4]], [], blankingType="node_in", tol=1.e-12, cellnval=2, overwrite=1)
#C.convertPyTree2File(t, 'out8.cgns')
test.testT(t,8)

def function():
    pass
