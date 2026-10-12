// SPDX-License-Identifier: GPL-3.0-only
// Browser-side FM-1 package helpers. Protocol/layout derived from the pinned
// Felucca 1.0.5 browser updater (GPL-3.0-only).
const BLOCKS = 20, BLK = 0x30, KEEP = 0x2f;

export function productOf(fwsc) {
  if (!(fwsc instanceof Uint8Array) || fwsc.length < BLOCKS * BLK)
    throw new Error('Not an FM-1 .fwsc package (file is too short).');
  let s = '';
  for (let i = 0; i < BLOCKS; i++) {
    const m = fwsc[i * BLK + KEEP];
    if (m !== 0x7d) s += String.fromCharCode((m - i - 1) & 0xff);
  }
  if (!/^FM-1_\d+$/.test(s)) throw new Error(`Unsupported firmware identity: ${s || '(missing)'}`);
  return s;
}

export function logicalImage(fwsc) {
  productOf(fwsc);
  const out = new Uint8Array(fwsc.length - BLOCKS);
  for (let i = 0; i < BLOCKS; i++) out.set(fwsc.subarray(i * BLK, i * BLK + KEEP), i * KEEP);
  out.set(fwsc.subarray(BLOCKS * BLK), BLOCKS * KEEP);
  return out;
}

export async function sha256hex(bytes) {
  const digest = await crypto.subtle.digest('SHA-256', bytes);
  return [...new Uint8Array(digest)].map(v => v.toString(16).padStart(2, '0')).join('');
}

export async function inspectPackage(bytes) {
  const product = productOf(bytes);
  const sha256 = await sha256hex(bytes);
  if (bytes.length < 128 * 1024) throw new Error('Package is unexpectedly small for an FM-1 image.');
  return { product, sha256, bytes: bytes.length, image: logicalImage(bytes) };
}
