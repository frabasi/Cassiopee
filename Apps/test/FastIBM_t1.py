# - Fast.IBM -
import Apps.Fast.IBM as App
import KCore.test as test

myApp = App.IBM(NP=0, format='single')
myApp.set(numb={"temporal_scheme": "implicit",
                "ss_iteration":3,
                "omp_mode":0})
myApp.set(numz={"time_step": 0.0007,
                "scheme":"roe_min",
                "time_step_nature":"local",
                "cfl":4.})

# Prepare
myApp.prepare('naca1D.cgns', t_out='t.cgns', tc_out='tc.cgns')

# Compute
myApp.compute('t.cgns', 'tc.cgns', t_out='restart.cgns', tc_out='tc_restart.cgns', nit=300)

# Post
t, zw = myApp.post('naca1D.cgns', 'restart.cgns', 'tc_restart.cgns', t_out='out.cgns', wall_out='wall.cgns')

test.testT(t, 1)
