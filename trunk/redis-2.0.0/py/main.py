import pyjot
print "run now"
import sys
#print sys.path
import time
import os

class A: pass

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
		#conn.send(data)
		#while 1: pyjot.enc( data, R )
		pass

p = proto()

pyjot.server( 3005, p  )
print "create server:", 3005


