# - cylinder (array) -
import Geom as D
import Converter as C
import KCore.test as test

a = D.cylinder((0,0,0), 1., 10.)
test.testA(a, 1)
