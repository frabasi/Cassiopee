# - Probe (pyTree) -
import Post.Probe as Probe
import Converter.PyTree as C
import Transform.PyTree as T
import Generator.PyTree as G
import Converter.Internal as Internal
import KCore.test as test

# test case
a = G.cartRx((0,0,0), (1,1,1), (30,30,30), (5,5,5), depth=0, addCellN=False)
C._initVars(a, '{centers:F} = {centers:CoordinateX}')
t = C.newPyTree(['Base',a])

# create a probe
p1 = Probe.Probe('probe1.cgns', t, X=(10.,10.,10.), fields=['centers:F'], append=False)
for i in range(20):
    time = 0.1*i
    C._initVars(t, f'{{centers:F}} = {{centers:CoordinateX}}+10.*sin({time})')
    p1.extract(time=time)
p1.flush()

# test append
p1 = Probe.Probe('probe1.cgns', t, X=(10.,10.,10.), fields=['centers:F'], append=True)
for i in range(20):
    time = 2.+0.1*i
    C._initVars(t, f'{{centers:F}} = {{centers:CoordinateX}}+10.*sin({time})')
    p1.extract(time=time)
p1.flush()
test.testT(p1._pZone, 1)

# reread probe
p1 = Probe.Probe('probe1.cgns')
out = p1.read()
test.testT(out, 2)
