import pyjot
print "run now"
import sys
#print sys.path
import time
import os

class A(object): pass


print A

class R : pass
pyjot.def_enc(R, A, ["m", "n"] );

pyjot.rule( R )

import pyjot as X
class Re:pass

class proto:
	def connectionMade( self, conn ):
		print self, "made", conn, type( conn ), type(conn).__bases__

	def connectionLost( self, conn, rea ):
		print self, "lost",conn, rea
	def dataReceive( self, conn, data ):
		#print self, "data", conn, data
#		conn.send( data )
#		while 1: conn.send( data )	
		#while 1: pyjot.enc( data, R )
		pass

p = proto()

pyjot.server( 3005, p  )
print "create server:", 3005

print X.SkipList

sl = X.SkipList()
print sl
for i in range( 1,1000 ):
	sl.insert( i, i )

print "len:", len(sl)
t1 = time.time()
for v in sl:
	if v.rank < 10:
		print v
t2 = time.time()
print t2 - t1, (t2-t1)/1000

print 


print len(sl), sl.rank( 1 )









