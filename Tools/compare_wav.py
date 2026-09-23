#!/usr/bin/env python3
"""
    python3 compare_wav.py reference.wav candidate.wav [--eps=1e-6]
"""

import sys
import numpy as np
from scipy.io import wavfile


def load_float(path: str):
    sr, data = wavfile.read(path)
    if data.dtype != np.float32:
        raise ValueError(f"{path}: needs 32-bit float WAV, get {data.dtype}")
    return sr, data.astype(np.float64)


def main() -> None:
    if len(sys.argv) < 3:
        print("Usage: compare_wav.py reference.wav candidate.wav [--eps=1e-6]")
        sys.exit(2)

    ref_path, cand_path = sys.argv[1], sys.argv[2]
    eps = 1e-6
    for arg in sys.argv[3:]:
        if arg.startswith("--eps="):
            eps = float(arg.split("=", 1)[1])

    sr_ref, ref = load_float(ref_path)
    sr_cand, cand = load_float(cand_path)

    if sr_ref != sr_cand:
        print(f"ERROR: differetn sample rate ({sr_ref} vs {sr_cand})")
        sys.exit(1)

    if ref.shape != cand.shape:
        n = min(len(ref), len(cand))
        print(f"Different files length ({len(ref)} vs {len(cand)} samples), "
              f"compare first {n}")
        ref, cand = ref[:n], cand[:n]

    diff = np.abs(ref - cand)
    max_diff = float(diff.max())
    rms_ref = float(np.sqrt(np.mean(ref ** 2)) + 1e-20)
    rms_diff = float(np.sqrt(np.mean(diff ** 2)))
    rms_diff_db = 20 * np.log10(rms_diff / rms_ref + 1e-20)

    print(f"Max. diff : {max_diff:.3e}")
    print(f"RMS diff              : {rms_diff:.3e}  ({rms_diff_db:.1f} dB )")

    if max_diff > eps:
        flat_idx = int(np.argmax(diff.max(axis=1) if diff.ndim > 1 else diff))
        print(f"Different results (threshold {eps:.1e} over, peak on sample ~{flat_idx})")
        sys.exit(1)

    print(f"OK  {eps:.1e}")
    sys.exit(0)


if __name__ == "__main__":
    main()
