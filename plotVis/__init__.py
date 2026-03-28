"""
Python tools for IMU data visualization and calibration.

Usage:
    # Run accelerometer calibration
    python -m plotVis.run_calibration

    # Plot all logs
    python -m plotVis.plot_logs

    # Test calibration
    python -m plotVis.test_calibration
"""

from .calibration import (
    AccelCalibration,
    GyroCalibration,
    compute_accel_calibration,
    compute_gyro_bias,
    load_accel_calibration,
    apply_accel_calibration,
    apply_gyro_bias,
    save_calibration,
    evaluate_calibration_quality,
)

from .visualization import (
    plot_logs,
    plot_time_series,
    plot_magnitudes,
    plot_histograms,
    plot_psd,
    simple_psd,
)

from .utils import (
    read_imu_csv,
    save_calibration_coefficients,
    load_calibration_coefficients,
)

from .config import config

__all__ = [
    "config",
    "AccelCalibration",
    "GyroCalibration",
    "compute_accel_calibration",
    "compute_gyro_bias",
    "load_accel_calibration",
    "apply_accel_calibration",
    "apply_gyro_bias",
    "save_calibration",
    "evaluate_calibration_quality",
    "plot_logs",
    "plot_time_series",
    "plot_magnitudes",
    "plot_histograms",
    "plot_psd",
    "simple_psd",
    "read_imu_csv",
    "save_calibration_coefficients",
    "load_calibration_coefficients",
]
