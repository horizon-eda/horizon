import sys
import pybis
import json

ibisfile = sys.argv[1]
root = pybis.IBSParser().parse(open(ibisfile, 'r'))

out = {}

for _,comp in root["component"].items() :
	pins = comp['Pin']
	for pin_name, pin in pins.items() :
		sn = pin['signal_name'].split("/")
		out[pin_name] = {"pin": sn[0], "alt":sn[1:]}
print (json.dumps(out, sort_keys=True, indent=4, separators=(',', ': ')))
