import sys
import yaml
sys.path.append("build")
import horizon

types = (
	"unit",
	"symbol",
	"entity",
	"part",
	"padstack",
	"package",
	"frame",
	"decal",
	"project",
	"board",
	"schematic",
	"blocks",
)

version_yml = yaml.load(open("scripts/app_versions.yml", "r"), Loader=yaml.SafeLoader)["versions"]

exitcode = 0

for object_type in types:
	app_version = horizon.get_app_version(object_type)
	if app_version != 0 :
		if object_type not in version_yml :
			print(object_type, "not found")
			exitcode = 1
			continue
		for v in range(1, app_version+1) :
			if v not in version_yml[object_type] :
				print(object_type, "version", v, "not found")
				exitcode = 1

if exitcode == 0 :
	print("All app versions okay")

exit(exitcode)
