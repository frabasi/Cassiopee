# - Fast.IBM -
import Apps.Fast.IBM as App
import Converter.PyTree as C
import KCore.test as test

LOCAL = test.getLocal()

myApp = App.IBM(NP=0, format='single')
myApp.set(numb={"temporal_scheme": "implicit",
                "ss_iteration":3,
                "omp_mode":0})
myApp.set(numz={"time_step": 0.0007,
                "scheme":"roe_min",
                "time_step_nature":"local",
                "cfl":4.})

# Prepare
t, tc = App.prepare0('naca1D.cgns', t_out=LOCAL+'/t.cgns', tc_out=LOCAL+'/tc.cgns', frontType=2)
test.testT(tc, 1)

# Compute
t,tc = myApp.compute(LOCAL+'/t.cgns', LOCAL+'/tc.cgns', t_out=LOCAL+'/restart.cgns', tc_out=LOCAL+'/tc_restart.cgns', nit=300)
test.testT(t, 2)

# Post
t, zw = myApp.post('naca1D.cgns', LOCAL+'/restart.cgns', LOCAL+'/tc_restart.cgns', t_out=LOCAL+'/out.cgns', wall_out=LOCAL+'/wall.cgns')
test.testT(t, 3)
