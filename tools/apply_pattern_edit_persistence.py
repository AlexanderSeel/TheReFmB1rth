#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Keep physical STEP/DRUM edits synchronized with the current A-H pattern.

The active sequencer buffer is what the panel edits and renders. Pattern changes,
project operations, or another path that reloads the current pattern can restore
its bank copy. If that bank copy is stale, a K1 note edit can appear immediately
and then disappear after leaving and revisiting the step.

Patch the final generated FM-1 bridge so every pattern mutation also refreshes
that current pattern slot. This is intentionally applied after the final
hardware rework pass, and does not alter the Virtual FM-1 interaction layer.
"""
from __future__ import annotations

import argparse
from pathlib import Path

STAMP = ".refm-pattern-edit-persistence"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"pattern persistence anchor mismatch in {path}: expected 1, got {count}: {old[:140]!r}"
        )
    path.write_text(text.replace(old, new, 1))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("felucca", type=Path)
    args = parser.parse_args()
    root = args.felucca.resolve()
    if not (root / ".refm-hardware-rework").is_file():
        raise SystemExit("apply hardware rework before pattern edit persistence")

    bridge = root / "firmware" / "refm" / "integration" / "refm_felucca_bridge.c"

    replace_once(
        bridge,
        "    if (r.consumed) return;\n",
        "    if (r.consumed) {\n"
        "        if (on && (g_ui.page == UI240_ACID_STEP || g_ui.page == UI240_DRUM ||\n"
        "                   g_modifier == UI240_MOD_SEQ || g_modifier == UI240_MOD_DRUM ||\n"
        "                   g_modifier == UI240_MOD_EDIT || g_modifier == UI240_MOD_LIVE))\n"
        "            groovebox_pattern_store(&g_refm.groovebox.patterns, &g_refm.groovebox,\n"
        "                                    g_refm.groovebox.current_pattern);\n"
        "        return;\n"
        "    }\n",
    )

    replace_once(
        bridge,
        "        else if (g_ui.page == UI240_ACID_STEP) ui240_edit_acid_step(&g_ui, &g_refm.groovebox, encoder, d);\n"
        "        else if (g_ui.page == UI240_DRUM) ui240_edit_drum(&g_ui, &g_refm.groovebox, encoder, d);\n",
        "        else if (g_ui.page == UI240_ACID_STEP) {\n"
        "            ui240_edit_acid_step(&g_ui, &g_refm.groovebox, encoder, d);\n"
        "            groovebox_pattern_store(&g_refm.groovebox.patterns, &g_refm.groovebox,\n"
        "                                    g_refm.groovebox.current_pattern);\n"
        "        } else if (g_ui.page == UI240_DRUM) {\n"
        "            ui240_edit_drum(&g_ui, &g_refm.groovebox, encoder, d);\n"
        "            groovebox_pattern_store(&g_refm.groovebox.patterns, &g_refm.groovebox,\n"
        "                                    g_refm.groovebox.current_pattern);\n"
        "        }\n",
    )

    replace_once(
        bridge,
        "    default:\n"
        "        break;\n"
        "    }\n"
        "}\n\n"
        "/* Read the exact 16-step pattern state",
        "    default:\n"
        "        break;\n"
        "    }\n"
        "    if ((action == REFM_ACT_CONTEXT || action == REFM_ACT_ALT) &&\n"
        "        (g_ui.page == UI240_ACID_STEP || g_ui.page == UI240_DRUM))\n"
        "        groovebox_pattern_store(&g_refm.groovebox.patterns, &g_refm.groovebox,\n"
        "                                g_refm.groovebox.current_pattern);\n"
        "}\n\n"
        "/* Read the exact 16-step pattern state",
    )

    (root / STAMP).write_text(
        "current-pattern-edits=write-through\n"
        "acid-k1=stored\n"
        "acid-keyboard=stored\n"
        "acid-context=stored\n"
        "drum-editor=stored\n"
        "drum-keyboard=stored\n"
    )
    print("FM-1 current-pattern step edit persistence applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
