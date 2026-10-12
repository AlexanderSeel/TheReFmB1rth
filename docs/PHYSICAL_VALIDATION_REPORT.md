# Physical validation report gate

The browser **VALIDATION** tab exports a JSON report containing:

- the firmware-channel tag, exact commit and FWSC SHA-256;
- every mandatory physical FM-1 check;
- the assigned factory-pattern runtime capture;
- the assigned worst-case stress runtime capture;
- notes and timestamps.

The report can be checked independently of the browser with `tools/physical_validation_report.py`.

## beta.2 verification

For `v0.1.0-beta.2` use:

```sh
python3 tools/physical_validation_report.py refm-physical-validation-*.json \
  --expect-tag v0.1.0-beta.2 \
  --expect-commit 2b4c537cc1a1f8c42c43ee953e14e27d7740837b \
  --expect-sha256 f2d2e13089a52cf279cee0bc43d9d4791ae913f0b3b889ec5b3de02b70c667b2 \
  --output build/physical-validation-summary.json
```

Exit codes:

- `0` — every mandatory physical check is PASS and both runtime gates pass;
- `1` — the report is complete but one or more physical/runtime gates are still open or failed;
- `2` — malformed/incomplete report, unsupported schema or expected tag/commit/SHA mismatch.

## Integrity rules

The offline gate does not trust the browser's displayed summary. It recalculates acceptance from the exported evidence.

- Every required check ID must be present.
- Every required physical check must be exactly `pass`; `pending`, `fail` and `na` do not qualify.
- Both `factory` and `stress` runtime captures must exist and their evaluated runtime gate must pass.
- Optional `--expect-*` arguments bind the report to the intended binary.
- Browser and Python required-check lists are contract-tested in CI so they cannot silently drift.

Keep the exported browser JSON, its Markdown companion, both raw runtime JSON captures and `build/physical-validation-summary.json` together as the hardware-validation evidence bundle.
