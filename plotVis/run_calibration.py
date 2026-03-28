#!/usr/bin/env python3
"""
Script to run accelerometer calibration using 6-axis method.
"""

import argparse
from pathlib import Path

from calibration import (
    compute_accel_calibration,
    save_calibration,
    evaluate_calibration_quality,
)
from config import config


def main():
    parser = argparse.ArgumentParser(description="Accelerometer 6-axis calibration")
    parser.add_argument(
        "--logs-dir",
        type=Path,
        default=config.LOGS_DIR,
        help="Directory containing calibration CSV files",
    )
    parser.add_argument(
        "--evaluate",
        action="store_true",
        help="Evaluate calibration quality after computation",
    )
    args = parser.parse_args()

    print("=" * 50)
    print("ACCELEROMETER 6-AXIS CALIBRATION")
    print("=" * 50)
    print()

    required_files = [
        "accel_X_down_calibration_data.csv",
        "accel_X_up_calibration_data.csv",
        "accel_Y_down_calibration_data.csv",
        "accel_Y_up_calibration_data.csv",
        "accel_Z_down_calibration_data.csv",
        "accel_Z_up_calibration_data.csv",
    ]

    missing = [f for f in required_files if not (args.logs_dir / f).exists()]
    if missing:
        print("ERROR: Missing calibration files:")
        for f in missing:
            print(f"  - {args.logs_dir / f}")
        return 1

    print("All calibration files found.")
    print()

    try:
        calibration = compute_accel_calibration(args.logs_dir)

        print()
        print("=" * 50)
        print("CALIBRATION RESULTS")
        print("=" * 50)
        print(
            f"Bias:  X={calibration.bias_x:.6f}, Y={calibration.bias_y:.6f}, Z={calibration.bias_z:.6f} g"
        )
        print(
            f"Scale: X={calibration.scale_x:.6f}, Y={calibration.scale_y:.6f}, Z={calibration.scale_z:.6f}"
        )
        print()

        save_calibration(calibration, args.logs_dir)

        print()
        print("C CODE CONSTANTS:")
        print(calibration.to_c_defines())

        if args.evaluate:
            print()
            print("=" * 50)
            print("CALIBRATION QUALITY EVALUATION")
            print("=" * 50)
            evaluate_calibration_quality(calibration, args.logs_dir)

        print()
        print("Calibration completed successfully!")
        return 0

    except Exception as e:
        print(f"ERROR: {e}")
        return 1


if __name__ == "__main__":
    exit(main())
