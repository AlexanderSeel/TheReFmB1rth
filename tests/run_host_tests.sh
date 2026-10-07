#!/usr/bin/env sh
set -eu
mkdir -p build/host
CFLAGS='-std=c11 -Wall -Wextra -Werror -O2'
cc $CFLAGS firmware/proto/acid303.c tests/test_acid303.c -o build/host/test_acid303
cc $CFLAGS firmware/proto/seq16.c tests/test_seq16.c -o build/host/test_seq16
cc $CFLAGS firmware/proto/ui_graph_model.c tests/test_ui_graph_model.c -o build/host/test_ui_graph_model
build/host/test_acid303
build/host/test_seq16
build/host/test_ui_graph_model
python3 -m unittest discover -s tests -p 'test_*.py'
