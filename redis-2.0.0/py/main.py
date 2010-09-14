import pyjot
print "run now"
import sys
#print sys.path
import time


class A: pass

class R : pass
pyjot.def_enc(R, A, ["m", "n"] );


print A.__dict__
print R.__dict__

o = A()
o.a = 1

o.m = "lllll"
o.n = o


a = [None, None, True, False]
b = [ True, False, [None, False] ]
a[1] = b
a[2] = b
a[3] =a 

a = dict( m = True, n= False )
a['ooo'] = a
a[1] = a

buf = pyjot.enc( a, R )
print buf
o = pyjot.dec( buf, R )
print pyjot.enc( o, R )
t = time.time()
#for i in range(100000):pyjot.enc( o, R )
t2 = time.time()
print t2 - t

