#!/bin/bash
set -euo pipefail
PREFIX=src/imp/footprint_generator
GEN="python3 scripts/extract_sub_pos.py"

function get_subs {
    python3 scripts/extract_sub_pos.py src/imp/footprint_generator/${1}.svg \
    $(grep -oP '"#(\w+)"' src/imp/footprint_generator/footprint_generator_${1}.cpp  | sort | uniq | tr -d '"#' | tr '\n' ' ')
}

get_subs quad
get_subs single
get_subs dual
get_subs grid
