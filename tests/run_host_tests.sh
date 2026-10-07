#!/usr/bin/env sh
set -eu
mkdir -p build/host
cc -std=c11 -Wall -Wextra -Werror -O2 firmware/proto/acid303.c tests/test_acid303.c -o build/host/test_acid303
build/host/test_acid303
python3 -m unittest discover -s tests -p 'test_*.py'
