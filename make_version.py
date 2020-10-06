import version as v
import os
import sys
import subprocess
from string import Template

cpp_template = """#include "util/version.hpp"
namespace horizon {
const unsigned int Version::major = $major;
const unsigned int Version::minor = $minor;
const unsigned int Version::micro = $micro;
const char *Version::name = "$name";
const char *Version::commit = "$commit";
const char *Version::commit_hash = "$commit_hash";
}
"""

gitversion = ""
commit_hash = ""
if os.path.isdir(".git"):
	gitversion = subprocess.check_output(['git', 'log', '-1', '--pretty=format:%h %ci %s']).decode()
	gitversion = gitversion.replace("\"", "\\\"")
	commit_hash = subprocess.check_output(['git', 'log', '-1', '--pretty=format:%h']).decode()

outfile = sys.argv[1]
with open(outfile, "w", encoding="utf-8") as fi:
	tmpl = Template(cpp_template)
	fi.write(tmpl.substitute(major=v.major, minor=v.minor, micro=v.micro, string=v.string, name=v.name, commit=gitversion, commit_hash=commit_hash))
