#!/bin/bash
set -e

echo "const char *gitversion = \"`git log -1 --pretty="format:%h %ci %s"`\";" > gitversion.cpp

