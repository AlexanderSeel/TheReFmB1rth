#!/usr/bin/env sh
set -eu
mkdir -p build/host
CFLAGS='-std=c11 -Wall -Wextra -Werror -O2'
cc $CFLAGS firmware/proto/acid303.c tests/test_acid303.c -o build/host/test_acid303
cc $CFLAGS firmware/proto/seq16.c tests/test_seq16.c -o build/host/test_seq16
cc $CFLAGS firmware/proto/ui_graph_model.c tests/test_ui_graph_model.c -o build/host/test_ui_graph_model
cc $CFLAGS firmware/proto/song.c tests/test_song.c -o build/host/test_song
cc $CFLAGS firmware/proto/midi_transport.c tests/test_midi_transport.c -o build/host/test_midi_transport
cc $CFLAGS firmware/proto/project_store.c tests/test_project_store.c -o build/host/test_project_store
cc $CFLAGS firmware/proto/mixer_fx.c tests/test_mixer_fx.c -o build/host/test_mixer_fx
cc $CFLAGS firmware/proto/drum_machine.c tests/test_drum_machine.c -o build/host/test_drum_machine
cc $CFLAGS firmware/proto/acid303.c firmware/proto/seq16.c firmware/proto/drum_machine.c firmware/proto/mixer_fx.c firmware/proto/midi_transport.c firmware/proto/song.c firmware/proto/project_store.c firmware/proto/groovebox.c tests/test_groovebox.c -o build/host/test_groovebox
build/host/test_acid303
build/host/test_seq16
build/host/test_ui_graph_model
build/host/test_song
build/host/test_midi_transport
build/host/test_project_store
build/host/test_mixer_fx
build/host/test_drum_machine
build/host/test_groovebox
python3 -m unittest discover -s tests -p 'test_*.py'
