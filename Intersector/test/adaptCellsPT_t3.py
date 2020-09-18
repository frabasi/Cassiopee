# - adapts a cells with respect to b points (PyTree) -
# 
import Generator.PyTree as G
import Transform.PyTree as T
import Converter.PyTree as C
import Converter.Internal as I
import Intersector.PyTree as XOR
import KCore.test as test

mesh = G.cart((0,0,0), (1,1,1), (20,20,20))
source = G.cartHexa((8,8,8), (0.2,0.2,0.2), (20,20,20))
#C.convertPyTree2File(mesh, 'm.cgns')
#C.convertPyTree2File(source, 's.cgns')

t = C.newPyTree(['Base',mesh])
t = T.splitNParts(t, 2, multigrid=0, dirs=[1,2,3])

zones = I.getZones(t)
p1 = zones[0]
p1 = C.convertArray2Tetra(p1, split='withBarycenters')
p1 = C.convertArray2NGon(p1)

p2 = C.convertArray2NGon(zones[1])

mesh = XOR.booleanUnion(p1,p2) #conformize the join
#C.convertPyTree2File(mesh, 'u.cgns')

mesh = C.initVars(mesh, '{centers:Density} = {centers:CoordinateX} + {centers:CoordinateY}')

m0 = XOR.adaptCells(mesh,source, sensor_type=0)
m0 = XOR.closeCells(m0)
test.testT(m0,1)
#C.convertPyTree2File(m0, 'out0.cgns')

m1 = XOR.adaptCells(mesh,source, sensor_type=0, smoothing_type=1)
m1 = XOR.closeCells(m1)
test.testT(m1,2)
#C.convertPyTree2File(m1, 'out1.cgns')

m2 = XOR.adaptCells(mesh,source, sensor_type=0)
m2 = XOR.closeCells(m2)
test.testT(m2,3)
#C.convertPyTree2File(m2, 'out2.cgns')

## dynamic adaptation
hmsh = XOR.createHMesh(mesh)
m3 = XOR.adaptCells(mesh, source, hmesh = hmsh, sensor_type=0)
m3 = XOR.conformizeHMesh(m3, hmsh)
m3 = XOR.closeCells(m3)
XOR.deleteHMesh(hmsh);
test.testT(m3,4)
#C.convertPyTree2File(m3, 'out3.cgns')

hmsh = XOR.createHMesh(mesh)
m4 = XOR.adaptCells(mesh, source, hmesh = hmsh, sensor_type=0, smoothing_type=1)

m4 = XOR.conformizeHMesh(m4, hmsh)
m4 = XOR.closeCells(m4)
test.testT(m4,5)
#C.convertPyTree2File(m4, 'out4.cgns')

m5 = XOR.adaptCells(m4, source, hmesh = hmsh, sensor_type=0) # applied to existing hmesh with the basic sensor

m5 = XOR.conformizeHMesh(m4, hmsh)
m5 = XOR.closeCells(m5)
XOR.deleteHMesh(hmsh);
test.testT(m5,6)
#C.convertPyTree2File(m5, 'out5.cgns')
