import pyjot
print "run now"
import sys
print sys.path
import time
print pyjot.enc( [None] )
print pyjot.enc( [None] )
i = 0
o = [None,True,None, [ float("nan") ] * 50, [2222]* 60 ] * 14 

print len( pyjot.enc( o )  )
print pyjot.enc( o )
t = time.time()
for i in range(1000):pyjot.enc( o )
t2 = time.time()
print t2 - t

