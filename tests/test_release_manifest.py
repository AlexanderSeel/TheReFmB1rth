import importlib.util
import tempfile
import unittest
from pathlib import Path

P=Path(__file__).resolve().parents[1]/"tools"/"release_manifest.py"
s=importlib.util.spec_from_file_location("release_manifest",P); m=importlib.util.module_from_spec(s); assert s.loader; s.loader.exec_module(m)

def make_package(path: Path, product="FM-1_910"):
    raw=bytearray([0x7D]*(m.BLOCKS*m.BLK+128))
    for i,ch in enumerate(product): raw[i*m.BLK+m.KEEP]=(ord(ch)+i+1)&0xFF
    path.write_bytes(raw)

class ReleaseManifestTests(unittest.TestCase):
    def test_manifest_roundtrip_and_tamper_detection(self):
        with tempfile.TemporaryDirectory() as td:
            pkg=Path(td)/"test.fwsc"; make_package(pkg)
            manifest=m.make_manifest(pkg,"abc123")
            self.assertEqual(manifest["product"],"FM-1_910")
            self.assertEqual(manifest["felucca_upstream"],m.UPSTREAM_FELUCCA)
            self.assertEqual(m.verify(pkg,manifest),[])
            pkg.write_bytes(pkg.read_bytes()+b"x")
            self.assertTrue(m.verify(pkg,manifest))

if __name__=="__main__": unittest.main()
