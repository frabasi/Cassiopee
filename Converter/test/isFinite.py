# - isFinite (array) -
import Generator as G
import Converter as C

a = G.cart((0,0,0), (1,1,1), (10,10,10))
a = C.initVars(a, 'F', 1.)
print(C.isFinite(a))
print(C.isFinite(a, var='x'))
print(C.isFinite(a, var='F'))
