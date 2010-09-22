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
#pyjot.set("111", o )
#pyjot.save()


def foo(  a, name = None ):
	print "111111111",a, name
	print pyjot.get("111")


print "get", pyjot.get("111")


class Re:pass

pyjot.def_enc( R, Re, [], "!" )

buf = pyjot.enc( o, R )
print buf

o = B()
o.m = [False, True, 1, 1.3, -10000, "pppp" ]
o.n = o
buf = pyjot.enc( o, R )
print buf
o = pyjot.dec( buf, R )
print o.__dict__

e = 122221.11112

d = {'a':e, 'b':e, 'c':e}


class proto:
	def connectionMade( self, conn ):
		print self, "made", conn
	def connectionLost( self, conn ):
		print self, "lost",conn
	def dataReceive( self, conn, data ):
		print self, "data", conn, data

p = proto()

pyjot.server( 3005, p  )
print "create server:", 3005

#o = pyjot.dec( buf, R )
#print pyjot.enc( o, R )
t = time.time()
for i in range(10000):
	pyjot.enc( d , R )
t2 = time.time()
print t2 - t, "///", (t2-t)/10000

import marshal as M

t = time.time()
for i in range( 10000 ):
	M.dumps( d )
	pass
t2 = time.time()
print t2 - t


