import Converter.PyTree as C
import Post.PyTree as P
import Generator.PyTree as G
import KCore.test as test

#-----
# NGON 2D
#-----
ni = 10
nj = 20
nk = 2
m = G.cartNGon((0,0,0), (1./(ni-1),1./(nj-1),1./(nk-1)), (ni,nj,nk))
def F(x, y) : return x*x + 2.*y*y
C._initVars(m,'centers:F', F, ['centers:CoordinateX', 'centers:CoordinateY'])
dim = 2
P._computeGradLSQ(m, 'centers:F', dim)
test.testT(m,0)

#-----
# NGON 3D
#-----
ni = 30
nj = 40
nk = 10
m = G.cartNGon((0,0,0), (1./(ni-1),1./(nj-1),1./(nk-1)), (ni,nj,nk))
def F(x, y, z) : return x*x + 2.*y*y + 3.*z*z
C._initVars(m,'centers:F', F, ['centers:CoordinateX', 'centers:CoordinateY', 'centers:CoordinateZ'])
dim = 3
P._computeGradLSQ(m, 'centers:F', dim)
test.testT(m,1)
