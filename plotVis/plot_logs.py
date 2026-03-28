#!/usr/bin/env python3
"""
Script to visualize IMU data from CSV logs.
"""

import argparse
from pathlib import Path

from visualization import plot_logs
from config import config


def main():
    parser = argparse.ArgumentParser(description="Plot IMU data from CSV logs")
    parser.add_argument(
        "--logs-dir",
        type=Path,
        default=config.LOGS_DIR,
        help="Directory containing CSV files",
    )
    parser.add_argument(
        "--fs",
        type=float,
        default=None,
        help="Sampling frequency in Hz (optional)",
    )
    parser.add_argument(
        "--plots",
        nargs="+",
        choices=["ts", "mag", "hist", "psd", "2d", "3d_accel", "3d_gyro"],
        default=["ts", "mag", "hist", "psd", "2d"],
        help="Plot types to generate",
    )
    args = parser.parse_args()

    print("=" * 50)
    print("IMU DATA VISUALIZATION")
    print("=" * 50)

    try:
        plot_logs(
            logs_dir=args.logs_dir,
            fs=args.fs,
            plot_types=args.plots,
        )
        return 0
    except Exception as e:
        print(f"ERROR: {e}")
        return 1


if __name__ == "__main__":
    exit(main())
