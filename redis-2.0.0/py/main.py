import pyjot
print "run now"
import sys
print sys.path
import time
print pyjot.enc( [None] )
print pyjot.enc( [None] )
i = 0
a = [9]
o = [None,True,None, 123, a, 8  ]

o = [False,True,True]
o[1] = o
print o

#print len( pyjot.enc( o )  )
print pyjot.enc( o )
t = time.time()
#for i in range(1000):pyjot.enc( o )
t2 = time.time()
print t2 - t

