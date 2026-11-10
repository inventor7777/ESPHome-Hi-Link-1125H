#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tests/schema_validation_test.py
python3 tests/test_save.py
binary=$(mktemp "${TMPDIR:-/tmp}/ld1125h-test.XXXXXX")
trap 'rm -f "$binary"' EXIT
clang++ -std=c++17 -fsanitize=address,undefined -I tests/stubs -I components \
  tests/protocol_test.cpp components/hi_link_ld1125h/hi_link_ld1125h.cpp -o "$binary"
"$binary"
