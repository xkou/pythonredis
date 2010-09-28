import time
import os
import pyjot
import sys

sys.us = {}
sys.ps = {}

class Rule:pass

sys.rule = Rule

import net

p = net.Proto()

pyjot.server( 3005, p )
print "create server:", 3005







