from typing import Optional, Dict
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.figure import Figure

from config import config
from utils.io import read_imu_csv
from calibration import (
    AccelCalibration,
    GyroCalibration,
    load_accel_calibration,
    apply_accel_calibration,
    apply_gyro_bias,
    compute_gyro_bias,
)
from .psd import simple_psd


def plot_time_series(
    data: Dict[str, np.ndarray],
    title: str = "IMU Data",
    fs: Optional[float] = None,
    window_sec: float = 2.0,
) -> Figure:
    """Plot time series for gyro and accel data."""
    idx = data["index"]

    if fs and fs > 0:
        t = (idx - idx[0]) / fs
        x_label = "Time [s]"
        win_n = max(1, int(window_sec * fs))
    else:
        t = idx
        x_label = "Index"
        win_n = 101

    def smooth(x, n):
        return np.convolve(x, np.ones(n) / n, mode="same")

    fig, axes = plt.subplots(3, 2, figsize=(14, 10))
    fig.suptitle(title)

    labels = [("gx", "gy", "gz"), ("ax", "ay", "az")]
    units = ["dps", "g"]

    for col, (axis_names, unit) in enumerate(zip(labels, units)):
        for row, name in enumerate(axis_names):
            ax = axes[row, col]
            y = data[name]
            ax.plot(t, y, "-", alpha=0.5, label="raw")
            ax.plot(t, smooth(y, win_n), "-", linewidth=1.5, label="smoothed")
            ax.set_ylabel(f"{name} [{unit}]")
            ax.grid(True, alpha=0.3)
            ax.legend(loc="upper right")

    for ax in axes[-1, :]:
        ax.set_xlabel(x_label)

    plt.tight_layout()
    return fig


def plot_magnitudes(
    data: Dict[str, np.ndarray],
    title: str = "Magnitudes",
    fs: Optional[float] = None,
    window_sec: float = 2.0,
) -> Figure:
    """Plot magnitude of gyro and accel vectors."""
    idx = data["index"]

    if fs and fs > 0:
        t = (idx - idx[0]) / fs
        x_label = "Time [s]"
        win_n = max(1, int(window_sec * fs))
    else:
        t = idx
        x_label = "Index"
        win_n = 101

    def smooth(x, n):
        return np.convolve(x, np.ones(n) / n, mode="same")

    gm = np.sqrt(data["gx"] ** 2 + data["gy"] ** 2 + data["gz"] ** 2)
    am = np.sqrt(data["ax"] ** 2 + data["ay"] ** 2 + data["az"] ** 2)

    fig, axes = plt.subplots(2, 1, figsize=(12, 6))
    fig.suptitle(title)

    axes[0].plot(t, gm, "-", alpha=0.5, label="raw")
    axes[0].plot(t, smooth(gm, win_n), "-", linewidth=1.5, label="smoothed")
    axes[0].set_ylabel("|g| [dps]")
    axes[0].grid(True, alpha=0.3)
    axes[0].legend()

    axes[1].plot(t, am, "-", alpha=0.5, label="raw")
    axes[1].plot(t, smooth(am, win_n), "-", linewidth=1.5, label="smoothed")
    axes[1].set_ylabel("|a| [g]")
    axes[1].set_xlabel(x_label)
    axes[1].grid(True, alpha=0.3)
    axes[1].legend()

    plt.tight_layout()
    return fig


def plot_histograms(
    data: Dict[str, np.ndarray], title: str = "Histograms", bins: int = 100
) -> Figure:
    """Plot histograms for each axis."""
    fig, axes = plt.subplots(3, 2, figsize=(12, 10))
    fig.suptitle(title)

    names = [("gx", "ax"), ("gy", "ay"), ("gz", "az")]
    units = ["dps", "g"]

    for row, (gyro_name, accel_name) in enumerate(names):
        axes[row, 0].hist(data[gyro_name], bins=bins, edgecolor="black", alpha=0.7)
        axes[row, 0].set_ylabel(gyro_name)
        axes[row, 0].grid(True, alpha=0.3)

        axes[row, 1].hist(data[accel_name], bins=bins, edgecolor="black", alpha=0.7)
        axes[row, 1].set_ylabel(accel_name)
        axes[row, 1].grid(True, alpha=0.3)

    axes[-1, 0].set_xlabel("Value [dps]")
    axes[-1, 1].set_xlabel("Value [g]")

    plt.tight_layout()
    return fig


def plot_psd(
    data: Dict[str, np.ndarray], title: str = "PSD", fs: Optional[float] = None
) -> Figure:
    """Plot power spectral density for each axis."""
    fig, axes = plt.subplots(3, 2, figsize=(14, 10))
    fig.suptitle(title)

    names = [("gx", "ax"), ("gy", "ay"), ("gz", "az")]

    for row, (gyro_name, accel_name) in enumerate(names):
        f, pxx, label = simple_psd(data[gyro_name], fs)
        axes[row, 0].plot(f, 10 * np.log10(pxx + np.finfo(float).eps))
        axes[row, 0].set_ylabel(f"{gyro_name} [dB]")
        axes[row, 0].grid(True, alpha=0.3)

        f, pxx, label = simple_psd(data[accel_name], fs)
        axes[row, 1].plot(f, 10 * np.log10(pxx + np.finfo(float).eps))
        axes[row, 1].set_ylabel(f"{accel_name} [dB]")
        axes[row, 1].grid(True, alpha=0.3)

    axes[-1, 0].set_xlabel(label)
    axes[-1, 1].set_xlabel(label)

    plt.tight_layout()
    return fig


def plot_2d_accel(data: Dict[str, np.ndarray], title: str = "Accel 2D") -> Figure:
    """Plot 2D projection of accelerometer data (ax-ay)."""
    fig, ax = plt.subplots(figsize=(8, 8))
    ax.plot(data["ax"], data["ay"], "-", alpha=0.5)
    ax.set_xlabel("ax [g]")
    ax.set_ylabel("ay [g]")
    ax.set_title(title)
    ax.grid(True, alpha=0.3)
    ax.axis("equal")
    plt.tight_layout()
    return fig


def plot_3d_scatter(
    data: Dict[str, np.ndarray], title: str = "3D Trajectory", sensor: str = "accel"
) -> Figure:
    """Plot 3D scatter plot colored by time/index."""
    from mpl_toolkits.mplot3d import Axes3D

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection="3d")

    if sensor == "accel":
        x, y, z = data["ax"], data["ay"], data["az"]
        xlabel, ylabel, zlabel = "ax [g]", "ay [g]", "az [g]"
    else:
        x, y, z = data["gx"], data["gy"], data["gz"]
        xlabel, ylabel, zlabel = "gx [dps]", "gy [dps]", "gz [dps]"

    idx = data["index"]
    scatter = ax.scatter(x, y, z, c=idx, cmap="jet", s=6)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_zlabel(zlabel)
    ax.set_title(title)
    fig.colorbar(scatter, ax=ax, label="Index")

    plt.tight_layout()
    return fig


def plot_logs(
    logs_dir: Optional[Path] = None,
    fs: Optional[float] = None,
    plot_types: Optional[list] = None,
) -> None:
    """
    Main function to plot all CSV files in logs directory.

    Args:
        logs_dir: Directory containing CSV files
        fs: Sampling frequency in Hz (optional)
        plot_types: List of plot types to generate. Options:
            ["ts", "mag", "hist", "psd", "2d", "3d_accel", "3d_gyro"]
    """
    import matplotlib

    matplotlib.use("Agg")

    logs_dir = Path(logs_dir) if logs_dir else config.LOGS_DIR

    if not logs_dir.exists():
        raise FileNotFoundError(f"Logs directory not found: {logs_dir}")

    csv_files = list(logs_dir.glob("*.csv"))
    if not csv_files:
        print(f"No CSV files found in {logs_dir}")
        return

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

    if plot_types is None:
        plot_types = ["ts", "mag", "hist", "psd", "2d"]

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

        if "ts" in plot_types:
            plot_time_series(data, f"Time Series: {base_title}", fs)

        if "mag" in plot_types:
            plot_magnitudes(data, f"Magnitudes: {base_title}", fs)

        if "hist" in plot_types:
            plot_histograms(data, f"Histograms: {base_title}")

        if "psd" in plot_types:
            plot_psd(data, f"PSD: {base_title}", fs)

        if "2d" in plot_types:
            plot_2d_accel(data, f"Accel 2D: {base_title}")

        if "3d_accel" in plot_types:
            plot_3d_scatter(data, f"Accel 3D: {base_title}", sensor="accel")

        if "3d_gyro" in plot_types:
            plot_3d_scatter(data, f"Gyro 3D: {base_title}", sensor="gyro")

    plt.show()
