#!/usr/bin/env python3
import pathlib
import unittest
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[1]


def pack7(data: bytes) -> bytes:
    out = bytearray()
    for off in range(0, len(data), 7):
        chunk = data[off:off + 7]
        mask = 0
        for i, b in enumerate(chunk):
            mask |= (b >> 7) << i
        out.append(mask)
        out.extend(b & 0x7F for b in chunk)
    return bytes(out)


def unpack7(data: bytes, expected: int) -> bytes:
    out = bytearray()
    pos = 0
    while pos < len(data) and len(out) < expected:
        mask = data[pos]
        pos += 1
        n = min(7, expected - len(out))
        if mask >> n or pos + n > len(data):
            raise ValueError
        for i in range(n):
            out.append(data[pos] | (((mask >> i) & 1) << 7))
            pos += 1
    if len(out) != expected:
        raise ValueError
    return bytes(out)


class ProjectTransferContractTests(unittest.TestCase):
    def test_pack7_roundtrip_and_crc_reference(self):
        raw = bytes(range(256))
        encoded = pack7(raw)
        self.assertEqual(unpack7(encoded, len(raw)), raw)
        self.assertEqual(zlib.crc32(b"123456789") & 0xFFFFFFFF, 0xCBF43926)

    def test_overlay_wires_shared_felucca_editor_transport(self):
        overlay = (ROOT / "tools" / "apply_felucca_overlay.py").read_text()
        handler = (ROOT / "firmware" / "integration" / "refm_editor_backup.h").read_text()
        self.assertIn('refm_editor_backup.h', overlay)
        self.assertIn('refm_backup_handle(cmd, a, na)', overlay)
        self.assertIn('#define ED_REFM_PROJECT 90u', handler)
        self.assertNotIn('#define ED_REFM_PROJECT 68u', handler)
        self.assertIn('refm_felucca_save(ED_BK_RAW', handler)
        self.assertIn('refm_felucca_load(ED_BK_RAW', handler)
        self.assertIn('st_crc32(ED_BK_RAW, refm_bk_len)', handler)
        self.assertIn('refm_felucca_ui_value(3u)', handler)

    def test_browser_uses_same_command_and_chunk_bound(self):
        browser = (ROOT / "web" / "app" / "project_transfer.js").read_text()
        html = (ROOT / "web" / "app" / "index.html").read_text()
        self.assertIn('const COMMAND=90;', browser)
        self.assertIn('const CHUNK=256;', browser)
        self.assertIn(".refmproj", browser)
        self.assertIn('PROJECT RESTORE OK', browser)
        self.assertIn('./project_transfer.js', html)


if __name__ == '__main__':
    unittest.main()
