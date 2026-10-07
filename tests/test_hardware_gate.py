import hashlib
import importlib.util
import unittest
from pathlib import Path

P=Path(__file__).resolve().parents[1]/"tools"/"hardware_gate.py"
s=importlib.util.spec_from_file_location("hardware_gate",P); m=importlib.util.module_from_spec(s); assert s.loader; s.loader.exec_module(m)

def package(product="FM-1_910"):
    raw=bytearray([0x7D]*(m.BLOCKS*m.BLK+128))
    for i,ch in enumerate(product): raw[i*m.BLK+m.KEEP]=(ord(ch)+i+1)&0xFF
    return bytes(raw)

class HardwareGateTests(unittest.TestCase):
    def test_gate_passes_only_with_all_acknowledgements(self):
        raw=package(); sha=hashlib.sha256(raw).hexdigest()
        self.assertEqual(m.validate_gate(raw,sha,"FM-1_15",True,m.CONFIRM_TEXT),[])

    def test_gate_rejects_hash_and_confirmation(self):
        raw=package(); errors=m.validate_gate(raw,"0"*64,"FM-1_15",False,"")
        self.assertIn("SHA-256 mismatch",errors)
        self.assertIn("--experimental is required",errors)
        self.assertTrue(any("confirmation" in e for e in errors))

    def test_gate_rejects_wrong_device(self):
        raw=package(); sha=hashlib.sha256(raw).hexdigest()
        self.assertTrue(m.validate_gate(raw,sha,"OTHER_1",True,m.CONFIRM_TEXT))

if __name__=="__main__": unittest.main()
