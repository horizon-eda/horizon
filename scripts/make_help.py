import sys
infile = sys.argv[1]
h_file = sys.argv[2]
c_file = sys.argv[3]

texts = {}
text_current = None
for line in open(infile, "r", encoding="UTF-8") :
	line = line.strip()
	if line.startswith("#") :
		text_current = line[1:]
		texts[text_current] = ""
	elif text_current is not None :
		texts[text_current] += line+"\n"

texts = {k:v.strip() for k,v in texts.items()}

with open(h_file, "w") as fi:
	print("#pragma once", file=fi)
	print("namespace horizon {", file=fi)
	print("class HelpTexts {", file=fi)
	print("public:", file=fi)
	for k in texts.keys() :
		print("    static const char* %s;"%k, file=fi)
	print("};", file=fi)
	print("}", file=fi)

with open(c_file, "w") as fi:
	print('#include "help_texts.hpp"', file=fi)
	print("namespace horizon {", file=fi)
	
	for k,v in texts.items() :
		print('const char* HelpTexts::%s = R"EOF(%s)EOF";'%(k,v), file=fi)
	print("}", file=fi)
