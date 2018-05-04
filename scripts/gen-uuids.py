#!/usr/bin/python3
import uuid
import sys

print("std::vector<horizon::UUID> %s= {"%sys.argv[1])
for i in range(int(sys.argv[2])) :
	print('"%s",'%str(uuid.uuid4()))
print("};")
