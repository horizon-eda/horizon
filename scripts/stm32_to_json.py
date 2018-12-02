import xml.etree.ElementTree as ET
import sys
import json
#use this script with the XML files from the db/mcu directory in an STM32CubeMX installation

ns = {'st': 'http://mcd.rou.st.com/modules.php?name=mcu'}
type_dir_map = {
	"Power": "power_input",
	"Reset": "input"
}

tree = ET.parse(sys.argv[1])
root = tree.getroot()
out = {}

for pin in root.findall("st:Pin", ns) :
	pin_name = pin.get("Name")
	pin_pos = pin.get("Position")
	pin_type = pin.get("Type")
	pin_dir = type_dir_map.get(pin_type, "bidirectional")

	alt_names = []
	for alt in pin.findall("st:Signal", ns) :
		n = alt.get("Name")
		if n != "GPIO":
			alt_names.append(n)

	out[pin_pos] = {"pin": pin_name, "direction": pin_dir, "alt":alt_names}

print (json.dumps(out, sort_keys=True, indent=4, separators=(',', ': ')))
