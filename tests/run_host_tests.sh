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
COMMON='firmware/proto/acid303.c firmware/proto/seq16.c firmware/proto/drum_machine.c firmware/proto/mixer_fx.c firmware/proto/midi_transport.c firmware/proto/song.c firmware/proto/project_store.c firmware/proto/pattern_bank.c firmware/proto/ui_graph_model.c firmware/proto/groovebox.c'
FEATURES='firmware/proto/groovebox_pattern.c firmware/proto/midi_router.c firmware/proto/groovebox_midi.c'
TARGET="$COMMON $FEATURES firmware/integration/refm_target.c"
cc $CFLAGS $COMMON tests/test_groovebox.c -o build/host/test_groovebox
cc $CFLAGS firmware/proto/pattern_bank.c tests/test_pattern_bank.c -o build/host/test_pattern_bank
cc $CFLAGS firmware/proto/drum_machine.c firmware/proto/midi_router.c tests/test_midi_router.c -o build/host/test_midi_router
cc $CFLAGS $COMMON $FEATURES tests/test_groovebox_features.c -o build/host/test_groovebox_features
cc $CFLAGS $TARGET tests/test_refm_target.c -o build/host/test_refm_target
cc $CFLAGS $TARGET web/emu/refm_wasm.c tests/test_wasm_bridge.c -o build/host/test_wasm_bridge
cc $CFLAGS $TARGET firmware/integration/refm_felucca_bridge.c tests/test_felucca_bridge.c -o build/host/test_felucca_bridge
for t in test_acid303 test_seq16 test_ui_graph_model test_song test_midi_transport test_project_store test_mixer_fx test_drum_machine test_groovebox test_pattern_bank test_midi_router test_groovebox_features test_refm_target test_wasm_bridge test_felucca_bridge; do build/host/$t; done
python3 -m unittest discover -s tests -p 'test_*.py'
