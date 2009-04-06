#!/usr/bin/env python

import sys

if len(sys.argv[1:]) == 0:
	sys.exit(1)

for arg in sys.argv[1:]:
	f = open(arg, 'r')
	f.seek(-1, 2)
	c = f.read(1)
	if c != '\n' and c != '\r':
		f.close()
		print(arg)
		f = open(arg, 'a')
		f.write('\n')
	
	f.close()
