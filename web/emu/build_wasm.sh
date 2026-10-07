#!/usr/bin/env sh
set -eu
command -v emcc >/dev/null 2>&1 || { echo 'emcc not found; install/activate Emscripten SDK' >&2; exit 2; }
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/wasm"
mkdir -p "$OUT"
COMMON="$ROOT/firmware/proto/acid303.c $ROOT/firmware/proto/seq16.c $ROOT/firmware/proto/sample_voice.c $ROOT/firmware/proto/drum_machine.c $ROOT/firmware/proto/mixer_fx.c $ROOT/firmware/proto/midi_transport.c $ROOT/firmware/proto/song.c $ROOT/firmware/proto/project_store.c $ROOT/firmware/proto/pattern_bank.c $ROOT/firmware/proto/groovebox.c $ROOT/firmware/proto/ui_graph_model.c"
FEATURES="$ROOT/firmware/proto/groovebox_pattern.c $ROOT/firmware/proto/midi_router.c $ROOT/firmware/proto/groovebox_midi.c"
SAMPLE_SRC=""
SAMPLE_FLAGS=""
if [ -s "$ROOT/build/samples/processed/refm_sample_assets.c" ]; then
  SAMPLE_SRC="$ROOT/build/samples/processed/refm_sample_assets.c"
  SAMPLE_FLAGS="-DREFM_BUNDLED_SAMPLES=1 -I$ROOT/build/samples/processed"
  cp "$ROOT/build/samples/processed/manifest.json" "$OUT/sample-manifest.json"
  [ ! -s "$ROOT/build/samples.lock.json" ] || cp "$ROOT/build/samples.lock.json" "$OUT/sample-lock.json"
fi
emcc -O3 -std=c11 $SAMPLE_FLAGS \
  $COMMON $FEATURES $SAMPLE_SRC \
  "$ROOT/firmware/integration/refm_target.c" \
  "$ROOT/web/emu/refm_wasm.c" \
  -I"$ROOT/firmware/proto" \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT=web,worker \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["HEAP8","HEAPU8","HEAP16","HEAP32"]' \
  -s EXPORTED_FUNCTIONS='["_malloc","_free","_refm_wasm_init","_refm_wasm_external_clock","_refm_wasm_midi","_refm_wasm_panic","_refm_wasm_render","_refm_wasm_snapshot","_refm_wasm_snapshot_ptr","_refm_wasm_snapshot_size","_refm_wasm_restore","_refm_wasm_step","_refm_wasm_pattern","_refm_wasm_bpm","_refm_wasm_enable_samples","_refm_wasm_sample_mask","_refm_wasm_select_pattern","_refm_wasm_set_acid_step","_refm_wasm_get_acid_step","_refm_wasm_set_drum_step","_refm_wasm_get_drum_step","_refm_wasm_set_acid_wave","_refm_wasm_set_acid_accent","_refm_wasm_set_acid_drive","_refm_wasm_acid_cutoff_hz","_refm_wasm_set_acid_mod","_refm_wasm_graph"]' \
  -o "$OUT/refm.js"
cp "$ROOT/web/emu/index.html" "$OUT/index.html"
cp "$ROOT/web/emu/sample_controls.js" "$OUT/sample_controls.js"
cp "$ROOT/web/emu/worklet.html" "$OUT/worklet.html"
cp "$ROOT/web/emu/worklet_client.js" "$OUT/worklet_client.js"
cp "$ROOT/web/emu/refm_audio_worklet.js" "$OUT/refm_audio_worklet.js"
python3 - "$OUT/index.html" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text(encoding='utf-8')
s=s.replace("mod=await createRefm(", "window.__refm=mod=await createRefm(", 1)
insert='''\n<script type="module">\nimport {installSampleControls} from './sample_controls.js';\nconst wait=()=>window.__refm?installSampleControls(window.__refm):setTimeout(wait,25); wait();\n</script>\n'''
s=s.replace('</body>', insert+'</body>', 1)
p.write_text(s, encoding='utf-8')
PY
echo "WASM groovebox built: $OUT/index.html + worklet.html + refm.js + refm.wasm"
if [ -n "$SAMPLE_SRC" ]; then echo "Bundled audition samples: enabled"; else echo "Bundled audition samples: not present (synth fallback)"; fi
