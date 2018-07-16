# - adapts a cells with respect to b points (array) -
import Intersector as XOR
import Converter as C
import Generator as G

a = G.cartHexa((0.,0.,0.), (0.1,0.1,0.1), (5,5,5))
a = C.convertArray2NGon(a); a = G.close(a)
#C.convertArrays2File([a], 'a.plt')
b = G.cartHexa((0.,0.,0.), (0.005,0.005,0.005), (5,5,5))
#C.convertArrays2File([b], 'b.plt')

m = XOR.adaptCells(a,b)
m = C.conformizeNGon(m)
m = XOR.closeOctalCells(m)
C.convertArrays2File([m], 'out.plt')

