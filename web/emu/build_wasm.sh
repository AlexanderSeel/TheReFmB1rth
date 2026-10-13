#!/usr/bin/env sh
set -eu
command -v emcc >/dev/null 2>&1 || { echo 'emcc not found; install/activate Emscripten SDK' >&2; exit 2; }
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/build/wasm"
mkdir -p "$OUT"
python3 "$ROOT/tools/generate_303_wavetables.py"
python3 "$ROOT/tools/generate_303_wavetables.py" --check
python3 "$ROOT/tools/generate_303_hq_wavetables.py" --output "$OUT/acid303_hq_wavetable.h"
python3 "$ROOT/tools/generate_303_hq_wavetables.py" --output "$OUT/acid303_hq_wavetable.h" --check
sed 's#"../../firmware/proto/acid303_wavetable.h"#"acid303_hq_wavetable.h"#' \
  "$ROOT/web/emu/acid303_hq.c" > "$OUT/acid303_hq_build.c"
COMMON="$OUT/acid303_hq_build.c $ROOT/firmware/proto/seq16.c $ROOT/firmware/proto/sample_voice.c $ROOT/firmware/proto/drum_machine.c $ROOT/firmware/proto/mixer_fx.c $ROOT/firmware/proto/midi_transport.c $ROOT/firmware/proto/song.c $ROOT/firmware/proto/project_store.c $ROOT/firmware/proto/pattern_bank.c $ROOT/firmware/proto/groovebox.c $ROOT/firmware/proto/ui_graph_model.c $ROOT/firmware/proto/ui240.c"
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
  -I"$ROOT/firmware/proto" -I"$OUT" \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT=web,worker \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["HEAP8","HEAPU8","HEAP16","HEAP32"]' \
  -s EXPORTED_FUNCTIONS='["_malloc","_free","_refm_wasm_init","_refm_wasm_external_clock","_refm_wasm_midi","_refm_wasm_panic","_refm_wasm_render","_refm_wasm_snapshot","_refm_wasm_snapshot_ptr","_refm_wasm_snapshot_size","_refm_wasm_restore","_refm_wasm_step","_refm_wasm_pattern","_refm_wasm_bpm","_refm_wasm_set_bpm","_refm_wasm_enable_samples","_refm_wasm_sample_mask","_refm_wasm_sample_use_mask","_refm_wasm_sample_active_mask","_refm_wasm_set_sample_lane","_refm_wasm_set_sample_use_mask","_refm_wasm_select_pattern","_refm_wasm_set_acid_step","_refm_wasm_get_acid_step","_refm_wasm_set_drum_step","_refm_wasm_get_drum_step","_refm_wasm_set_acid_wave","_refm_wasm_set_acid_tune","_refm_wasm_set_acid_accent","_refm_wasm_set_acid_drive","_refm_wasm_acid_cutoff_hz","_refm_wasm_set_acid_mod","_refm_wasm_set_mix_track","_refm_wasm_get_mix_track","_refm_wasm_set_fx","_refm_wasm_get_fx","_refm_wasm_graph","_refm_wasm_panel_encoder","_refm_wasm_panel_action","_refm_wasm_panel_key","_refm_wasm_panel_value"]' \
  -o "$OUT/refm.js"

cp "$ROOT/web/emu/index.html" "$OUT/index.html"
cp "$ROOT/web/emu/sample_controls.js" "$OUT/sample_controls.js"
cp "$ROOT/web/emu/fx_board.js" "$OUT/fx_board.js"
cp "$ROOT/web/emu/rebirth_skin.css" "$OUT/rebirth_skin.css"
cp "$ROOT/web/emu/rebirth_skin.js" "$OUT/rebirth_skin.js"
cp "$ROOT/web/emu/worklet.html" "$OUT/worklet.html"
cp "$ROOT/web/emu/worklet_client.js" "$OUT/worklet_client.js"
cp "$ROOT/web/emu/refm_audio_worklet.js" "$OUT/refm_audio_worklet.js"
rm -rf "$OUT/app"
mkdir -p "$OUT/app"
cp "$ROOT/web/app/"*.html "$ROOT/web/app/"*.js "$ROOT/web/app/"*.css "$OUT/app/"

# The combined app is deployed under /app while refm.js/refm.wasm live one level up.
# Inject the current-runtime download helper into the built copy without forcing the
# source app to depend on a generated build directory.
python3 - "$OUT/app/index.html" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text()
needle='</body>'
script='  <script type="module" src="./latest_wasm.js"></script>\n'
if script.strip() not in s:
    if needle not in s: raise SystemExit('combined app body anchor missing')
    s=s.replace(needle,script+needle,1)
p.write_text(s)
PY

WASM_SHA=$(sha256sum "$OUT/refm.wasm" | awk '{print $1}')
COMMIT=${GITHUB_SHA:-local}
printf '{\n  "commit": "%s",\n  "sha256": "%s",\n  "file": "refm.wasm",\n  "app": "app/index.html"\n}\n' "$COMMIT" "$WASM_SHA" > "$OUT/wasm-channel.json"

grep -q 'rebirth_skin.css' "$OUT/index.html"
grep -q 'installRebirthSkin' "$OUT/index.html"
grep -q 'installSampleControls' "$OUT/index.html"
grep -q 'installFxBoard' "$OUT/sample_controls.js"
grep -q 'REVERB' "$OUT/fx_board.js"
grep -q 'REFM_ACID_WT_BANDS 12u' "$OUT/acid303_hq_wavetable.h"
grep -q 'REFM_ACID_WT_SIZE 2048u' "$OUT/acid303_hq_wavetable.h"
grep -q 'Browser-only high-quality TB-303 voice' "$ROOT/web/emu/acid303_hq.c"
grep -q 'data-tab="virtual"' "$OUT/app/index.html"
grep -q 'virtual_fm1.js' "$OUT/app/index.html"
grep -q 'latest_wasm.js' "$OUT/app/index.html"
grep -q '_refm_wasm_panel_value' "$OUT/refm.js"
test -s "$OUT/wasm-channel.json"

echo "WASM groovebox built with full-mipmap Open303-HQ acid core + combined installer/editor/virtual FM-1 app: $OUT/app/index.html"
echo "WASM channel: commit=$COMMIT sha256=$WASM_SHA"
if [ -n "$SAMPLE_SRC" ]; then echo "Bundled audition samples: enabled"; else echo "Bundled audition samples: not present (synth fallback)"; fi
