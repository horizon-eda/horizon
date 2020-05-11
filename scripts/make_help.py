import sys
mode = sys.argv[1]
infile = sys.argv[2]

texts = {}
text_current = None
for line in open(infile, "r") :
	line = line.strip()
	if line.startswith("#") :
		text_current = line[1:]
		texts[text_current] = ""
	elif text_current is not None :
		texts[text_current] += line+"\n"

texts = {k:v.strip() for k,v in texts.items()}

if mode == 'h' :
	print("#pragma once")
	print("namespace horizon {")
	print("class HelpTexts {")
	print("public:")
	for k in texts.keys() :
		print("    static const char* %s;"%k)
	print("};")
	print("}")

if mode == 'c' :
	print('#include "help_texts.hpp"')
	print("namespace horizon {")
	
	for k,v in texts.items() :
		print('const char* HelpTexts::%s = R"EOF(%s)EOF";'%(k,v))
	print("}")
