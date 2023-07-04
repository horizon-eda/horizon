#!/usr/bin/python
from __future__ import print_function

import json
import sys
import os

out = []
for filename in sys.argv[2:] :
	with open(filename, "r") as fi :
		j = json.load(fi)
	j["name"] = os.path.basename(filename).split(".")[0].replace("_", " ")
	out.append(j)

with open(sys.argv[1], "w") as ofi:
	json.dump(out, ofi, sort_keys=True, indent=4)
