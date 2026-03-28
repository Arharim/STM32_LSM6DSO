#!/usr/bin/env python3
"""
Script to test accelerometer calibration on monitor data.
"""

import argparse
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

from calibration import load_accel_calibration, apply_accel_calibration
from utils import read_imu_csv
from config import config


def main():
    parser = argparse.ArgumentParser(description="Test accelerometer calibration")
    parser.add_argument(
        "--logs-dir",
        type=Path,
        default=config.LOGS_DIR,
        help="Directory containing CSV files",
    )
    parser.add_argument(
        "--monitor-file",
        type=str,
        default="monitor.csv",
        help="CSV file to test calibration on",
    )
    parser.add_argument(
        "--save-plot",
        type=Path,
        default=None,
        help="Save plot to file",
    )
    args = parser.parse_args()

    print("=" * 50)
    print("CALIBRATION TEST")
    print("=" * 50)
    print()

    try:
        calibration = load_accel_calibration(args.logs_dir)
    except FileNotFoundError:
        print("Calibration file not found. Run run_calibration.py first.")
        return 1

    monitor_path = args.logs_dir / args.monitor_file
    if not monitor_path.exists():
        print(f"Monitor file not found: {monitor_path}")
        return 1

    print(f"Reading data from {monitor_path}...")
    data = read_imu_csv(monitor_path)
    print(f"Loaded {len(data['index'])} measurements")

    ax_raw, ay_raw, az_raw = data["ax"], data["ay"], data["az"]
    ax_cal, ay_cal, az_cal = apply_accel_calibration(
        ax_raw, ay_raw, az_raw, calibration
    )

    print()
    print("=" * 50)
    print("STATISTICS (BEFORE CALIBRATION)")
    print("=" * 50)
    for name, arr in [("ax", ax_raw), ("ay", ay_raw), ("az", az_raw)]:
        print(
            f"{name}: mean={np.mean(arr):.4f}, std={np.std(arr):.4f}, "
            f"min={np.min(arr):.4f}, max={np.max(arr):.4f}"
        )

    print()
    print("=" * 50)
    print("STATISTICS (AFTER CALIBRATION)")
    print("=" * 50)
    for name, arr in [("ax", ax_cal), ("ay", ay_cal), ("az", az_cal)]:
        print(
            f"{name}: mean={np.mean(arr):.4f}, std={np.std(arr):.4f}, "
            f"min={np.min(arr):.4f}, max={np.max(arr):.4f}"
        )

    mag_raw = np.sqrt(ax_raw**2 + ay_raw**2 + az_raw**2)
    mag_cal = np.sqrt(ax_cal**2 + ay_cal**2 + az_cal**2)

    print()
    print("=" * 50)
    print("ACCELERATION MAGNITUDE")
    print("=" * 50)
    print(
        f"Before:  mean={np.mean(mag_raw):.4f}, std={np.std(mag_raw):.4f} (ideal = 1.0)"
    )
    print(
        f"After:   mean={np.mean(mag_cal):.4f}, std={np.std(mag_cal):.4f} (ideal = 1.0)"
    )

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle("Accelerometer Calibration Test")

    axes[0, 0].plot(ax_raw, "r-", linewidth=0.8, label="ax")
    axes[0, 0].plot(ay_raw, "g-", linewidth=0.8, label="ay")
    axes[0, 0].plot(az_raw, "b-", linewidth=0.8, label="az")
    axes[0, 0].set_title("Raw Data")
    axes[0, 0].set_xlabel("Sample")
    axes[0, 0].set_ylabel("Acceleration (g)")
    axes[0, 0].legend()
    axes[0, 0].grid(True, alpha=0.3)

    axes[0, 1].plot(ax_cal, "r-", linewidth=0.8, label="ax")
    axes[0, 1].plot(ay_cal, "g-", linewidth=0.8, label="ay")
    axes[0, 1].plot(az_cal, "b-", linewidth=0.8, label="az")
    axes[0, 1].set_title("Calibrated Data")
    axes[0, 1].set_xlabel("Sample")
    axes[0, 1].set_ylabel("Acceleration (g)")
    axes[0, 1].legend()
    axes[0, 1].grid(True, alpha=0.3)

    axes[1, 0].plot(mag_raw, "k-", linewidth=0.8, label="Before")
    axes[1, 0].plot(mag_cal, "r-", linewidth=0.8, label="After")
    axes[1, 0].axhline(
        y=1.0, color="g", linestyle="--", linewidth=2, label="Ideal (1g)"
    )
    axes[1, 0].set_title("Acceleration Magnitude")
    axes[1, 0].set_xlabel("Sample")
    axes[1, 0].set_ylabel("|a| (g)")
    axes[1, 0].legend()
    axes[1, 0].grid(True, alpha=0.3)

    axes[1, 1].hist(
        mag_raw, bins=50, alpha=0.7, label="Before", color="gray", edgecolor="black"
    )
    axes[1, 1].hist(
        mag_cal, bins=50, alpha=0.7, label="After", color="red", edgecolor="black"
    )
    axes[1, 1].axvline(
        x=1.0, color="g", linestyle="--", linewidth=2, label="Ideal (1g)"
    )
    axes[1, 1].set_title("Magnitude Distribution")
    axes[1, 1].set_xlabel("|a| (g)")
    axes[1, 1].set_ylabel("Count")
    axes[1, 1].legend()
    axes[1, 1].grid(True, alpha=0.3)

    plt.tight_layout()

    if args.save_plot:
        plt.savefig(args.save_plot, dpi=300)
        print(f"\nPlot saved to: {args.save_plot}")

    plt.show()
    return 0


if __name__ == "__main__":
    exit(main())
