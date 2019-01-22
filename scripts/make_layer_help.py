#!/usr/bin/python
from __future__ import print_function

import json
import sys
import os

out = {}
for filename in sys.argv[1:] :
	txt = open(filename, "r").read().strip()
	layer = os.path.basename(filename)
	out[layer] = txt

print(json.dumps(out, sort_keys=True, indent=4))
