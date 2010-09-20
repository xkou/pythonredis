import pyjot
print "run now"
import sys
#print sys.path
import time


class A: pass

class R : pass
pyjot.def_enc(R, A, ["m", "n"] );


pyjot.rule( R )

class B( A ):pass
pyjot.def_enc(R, B );

print A.__dict__
print R.__dict__

o = A()
o.a = 1


o.m = "lllll"
o.n = o



pyjot.set("111", o )
pyjot.save()
buf = pyjot.enc( o, R )
print buf

o = B()
o.m = [False, True, 1, 1.3, -10000, "pppp" ]
o.n = o
buf = pyjot.enc( o, R )
print buf
o = pyjot.dec( buf, R )
print o.__dict__



#o = pyjot.dec( buf, R )
#print pyjot.enc( o, R )
t = time.time()
for i in range(1000):
	pyjot.enc( [ 1, True, None, [1, 987847 ] * 250 ] , R )
t2 = time.time()
print t2 - t

