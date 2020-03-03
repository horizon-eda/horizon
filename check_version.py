import version as v
import xml.etree.ElementTree as ET
tree = ET.parse('org.horizon_eda.HorizonEDA.metainfo.xml')
root = tree.getroot()
version_from_xml = root.find("releases").find("release").attrib["version"]

if version_from_xml == v.string :
	print("Version okay")
else:
	print("Version mismatch %s != %s"%(v.string, version_from_xml))
	exit(1)
