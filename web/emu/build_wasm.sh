#!/usr/bin/env sh
set -eu
command -v emcc >/dev/null 2>&1 || { echo 'emcc not found; install/activate Emscripten SDK' >&2; exit 2; }
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/wasm"
mkdir -p "$OUT"
COMMON="$ROOT/firmware/proto/acid303.c $ROOT/firmware/proto/seq16.c $ROOT/firmware/proto/drum_machine.c $ROOT/firmware/proto/mixer_fx.c $ROOT/firmware/proto/midi_transport.c $ROOT/firmware/proto/song.c $ROOT/firmware/proto/project_store.c $ROOT/firmware/proto/pattern_bank.c $ROOT/firmware/proto/groovebox.c $ROOT/firmware/proto/ui_graph_model.c"
FEATURES="$ROOT/firmware/proto/groovebox_pattern.c $ROOT/firmware/proto/midi_router.c $ROOT/firmware/proto/groovebox_midi.c"
emcc -O3 -std=c11 \
  $COMMON $FEATURES \
  "$ROOT/firmware/integration/refm_target.c" \
  "$ROOT/web/emu/refm_wasm.c" \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT=web,worker \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["HEAP8","HEAPU8","HEAP16","HEAP32"]' \
  -s EXPORTED_FUNCTIONS='["_malloc","_free","_refm_wasm_init","_refm_wasm_external_clock","_refm_wasm_midi","_refm_wasm_panic","_refm_wasm_render","_refm_wasm_snapshot","_refm_wasm_snapshot_ptr","_refm_wasm_snapshot_size","_refm_wasm_restore","_refm_wasm_step","_refm_wasm_pattern","_refm_wasm_bpm","_refm_wasm_select_pattern","_refm_wasm_set_acid_step","_refm_wasm_get_acid_step","_refm_wasm_set_drum_step","_refm_wasm_get_drum_step","_refm_wasm_set_acid_mod","_refm_wasm_graph"]' \
  -o "$OUT/refm.js"
cp "$ROOT/web/emu/index.html" "$OUT/index.html"
echo "WASM groovebox built: $OUT/index.html + refm.js + refm.wasm"
