#!/usr/bin/env python3
"""
Script to visualize IMU data from CSV logs.
"""

import argparse
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt

from visualization.plots import (
    plot_time_series,
    plot_magnitudes,
    plot_histograms,
    plot_psd,
    plot_2d_accel,
    plot_3d_scatter,
)
from utils.io import read_imu_csv
from calibration import (
    compute_gyro_bias,
    load_accel_calibration,
    apply_accel_calibration,
    apply_gyro_bias,
    AccelCalibration,
    GyroCalibration,
)
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

    logs_dir = Path(args.logs_dir) if args.logs_dir else config.LOGS_DIR

    if not logs_dir.exists():
        print(f"ERROR: Logs directory not found: {logs_dir}")
        return 1

    csv_files = list(logs_dir.glob("*.csv"))
    if not csv_files:
        print(f"No CSV files found in {logs_dir}")
        return 1

    print(f"Found {len(csv_files)} CSV files")

    try:
        gyro_cal = compute_gyro_bias()
        print(
            f"Using gyro calibration: bias=[{gyro_cal.bias_x:.6f}, {gyro_cal.bias_y:.6f}, {gyro_cal.bias_z:.6f}] dps"
        )
    except FileNotFoundError:
        print("Gyro calibration not found, using zero bias")
        gyro_cal = GyroCalibration(bias=np.zeros(3))

    try:
        accel_cal = load_accel_calibration()
        print(
            f"Using accel calibration: bias={accel_cal.bias}, scale={accel_cal.scale}"
        )
    except FileNotFoundError:
        print("Accel calibration not found, using defaults")
        accel_cal = AccelCalibration(bias=np.zeros(3), scale=np.ones(3))

    for csv_file in csv_files:
        print(f"\nProcessing: {csv_file.name}")

        data = read_imu_csv(csv_file)

        data["gx"], data["gy"], data["gz"] = apply_gyro_bias(
            data["gx"], data["gy"], data["gz"], gyro_cal
        )
        data["ax"], data["ay"], data["az"] = apply_accel_calibration(
            data["ax"], data["ay"], data["az"], accel_cal
        )

        base_title = csv_file.stem

        if "ts" in args.plots:
            plot_time_series(data, f"Time Series: {base_title}", args.fs)

        if "mag" in args.plots:
            plot_magnitudes(data, f"Magnitudes: {base_title}", args.fs)

        if "hist" in args.plots:
            plot_histograms(data, f"Histograms: {base_title}")

        if "psd" in args.plots:
            plot_psd(data, f"PSD: {base_title}", args.fs)

        if "2d" in args.plots:
            plot_2d_accel(data, f"Accel 2D: {base_title}")

        if "3d_accel" in args.plots:
            plot_3d_scatter(data, f"Accel 3D: {base_title}", sensor="accel")

        if "3d_gyro" in args.plots:
            plot_3d_scatter(data, f"Gyro 3D: {base_title}", sensor="gyro")

    plt.show()


if __name__ == "__main__":
    exit(main())
