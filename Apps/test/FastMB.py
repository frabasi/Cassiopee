# - Fast.MB -
import Apps.Fast.MB as App

myApp = App.MB(NP=0, format='single')
myApp.set(numb={"temporal_scheme": "implicit",
                "ss_iteration":3})
myApp.set(numz={"time_step": 0.0007,
                "scheme":"roe_min",
                "time_step_nature":"local",
                "cfl":4.})

# Prepare
myApp.prepare('naca.cgns', t_out='t.cgns', tc_out='tc.cgns')

# closed compute
myApp.compute('t.cgns', 'tc.cgns', t_out='restart.cgns', nit=300)

# Post
myApp.post('restart.cgns', 'out.cgns', 'wall.cgns')
