# - snearFactor (pyTree) -
import Converter.PyTree as C
import Geom.IBM as D_IBM
import Geom.PyTree as D

a = D.circle((0,0,0), 1. , 0., 360.)
a = D_IBM.setSnear(a,0.01)
a = D_IBM.snearFactor(a,2)

C.convertPyTree2File(a, 'out.cgns')
