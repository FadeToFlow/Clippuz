#!/usr/bin/env python3
"""
    python3 run_regression_tests.py \
        --render-exe build/RegressionRender_artefacts/RegressionRender \
        --signals-dir test_signals \
        --reference-dir test_signals/reference \
        [--eps 1e-5] [--update-reference]
"""

import argparse
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
COMPARE_SCRIPT = SCRIPT_DIR / "compare_wav.py"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--render-exe", required=True)
    ap.add_argument("--signals-dir", required=True)
    ap.add_argument("--reference-dir", required=True)
    ap.add_argument("--eps", default="1e-5")
    ap.add_argument("--update-reference", action="store_true")
    args = ap.parse_args()

    signals_dir = Path(args.signals_dir)
    reference_dir = Path(args.reference_dir)
    reference_dir.mkdir(parents=True, exist_ok=True)

    rendered_dir = Path("rendered")
    rendered_dir.mkdir(parents=True, exist_ok=True)

    wav_files = sorted(signals_dir.glob("*.wav"))
    if not wav_files:
        print(f".wav files not excists in {signals_dir} — first run generate_test_signals.py")
        return 1

    failed = []
    for wav in wav_files:
        out_path = rendered_dir / wav.name
        ref_path = reference_dir / wav.name

        render_cmd = [args.render_exe, str(wav), str(out_path)]
        result = subprocess.run(render_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"[RENDER ERROR] {wav.name}\n{result.stdout}\n{result.stderr}")
            failed.append(wav.name)
            continue

        if args.update_reference or not ref_path.exists():
            ref_path.write_bytes(out_path.read_bytes())
            print(f"[REFERENCE UPDATE] {wav.name}")
            continue

        compare_cmd = [sys.executable, str(COMPARE_SCRIPT), str(ref_path), str(out_path),
                        f"--eps={args.eps}"]
        result = subprocess.run(compare_cmd, capture_output=True, text=True)
        print(f"--- {wav.name} ---")
        print(result.stdout.strip())
        if result.returncode != 0:
            failed.append(wav.name)

    if failed:
        print(f"\nFAILED: {', '.join(failed)}")
        return 1

    print("\nAll tests complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
