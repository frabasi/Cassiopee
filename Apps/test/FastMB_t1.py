# - Fast.MB -
import Apps.Fast.MB as App
import KCore.test as test
import Converter.PyTree as C

myApp = App.MB()
myApp.set(NP=0, format='single')
myApp.set(numb={"temporal_scheme": "implicit",
                "ss_iteration":3})
myApp.set(numz={"time_step": 0.0007,
                "scheme":"roe_min",
                "time_step_nature":"local",
                "cfl":4.})

t, tc = myApp.prepare('naca.cgns', t_out='t.cgns', tc_out='tc.cgns')
test.testT(tc, 2)

t = myApp.compute('t.cgns', 'tc.cgns', t_out='restart.cgns', nit=300)
t = C.convertFile2PyTree('restart.cgns')
test.testT(t, 1)

