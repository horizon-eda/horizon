#!/bin/bash

# Find clang-format v5
CLANG_FORMAT=$(command -v clang-format)
CLANG_FORMAT5=$(command -v clang-format-5.0)

if [ ! -z $CLANG_FORMAT5 ]; then
	# clang-format-5.0 exists
	CLANG_BIN=$CLANG_FORMAT5
  echo "Using clang-format-5.0 command, version 5 detected"
elif [ ! -z $CLANG_FORMAT ]; then
  CLANG_FORMAT_VERSION="$(clang-format -version | cut -d " " -f 3 | cut -d "." -f 1)"

  # Check version
  if [ "$CLANG_FORMAT_VERSION" = "5" ]; then
	  # clang-format is 5.0
	  CLANG_BIN=$CLANG_FORMAT
    echo "Using clang-format command, version 5 detected"
  fi
else
  # error messages
	if [ -z $CLANG_FORMAT ]; then
		echo "clang-format was not found"
	else
		echo "clang-format-5.0 was not found, and clang-format was not the correct version"
		echo "Version was $CLANG_FORMAT_VERSION"
	fi
	exit -1
fi

# Search using the binary found
find ./src -iname *.h -o -iname *.cpp -o -iname *.hpp -o -iname *.cc -o -iname *.c | xargs $CLANG_BIN -style=file -i
