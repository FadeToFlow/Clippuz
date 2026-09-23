#!/usr/bin/env python3
"""
    pip install numpy scipy
    python3 generate_test_signals.py
"""

import os
import numpy as np
from scipy.io import wavfile
from scipy.signal import chirp

SAMPLE_RATE = 48000
OUT_DIR = "test_signals"


def save(name: str, mono: np.ndarray) -> None:
    stereo = np.stack([mono, mono], axis=1).astype(np.float32)
    path = os.path.join(OUT_DIR, name)
    wavfile.write(path, SAMPLE_RATE, stereo)
    print(f"  -> {name} ({len(mono) / SAMPLE_RATE:.2f} с)")


def main() -> None:
    os.makedirs(OUT_DIR, exist_ok=True)
    print(f"Генерирую тестовые сигналы в {OUT_DIR}/ ...")

    t = np.linspace(0, 5.0, int(5.0 * SAMPLE_RATE), endpoint=False)
    sweep = 0.5 * chirp(t, f0=20, f1=20000, t1=5.0, method="logarithmic")
    save("01_sine_sweep.wav", sweep)

    rng = np.random.default_rng(42)
    noise = 0.3 * rng.standard_normal(int(3.0 * SAMPLE_RATE))
    save("02_white_noise.wav", noise)

    impulse = np.zeros(SAMPLE_RATE, dtype=np.float64)
    impulse[0] = 1.0
    save("03_impulse.wav", impulse)
 
    t_seg = np.linspace(0, 0.5, int(0.5 * SAMPLE_RATE), endpoint=False)
    levels_db = np.linspace(-40, 6, 8)
    step = np.concatenate(
        [10 ** (db / 20.0) * np.sin(2 * np.pi * 440 * t_seg) for db in levels_db]
    )
    save("04_level_steps.wav", step)

    silence = np.zeros(int(1.0 * SAMPLE_RATE), dtype=np.float64)
    save("05_silence.wav", silence)

    print("Generated.")


if __name__ == "__main__":
    main()
