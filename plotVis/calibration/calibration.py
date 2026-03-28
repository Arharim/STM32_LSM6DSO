from dataclasses import dataclass
from typing import Tuple, Optional
import numpy as np
from pathlib import Path

from config import config
from utils.io import (
    read_imu_csv,
    save_calibration_coefficients,
    load_calibration_coefficients,
)


@dataclass
class AccelCalibration:
    bias: np.ndarray
    scale: np.ndarray

    @property
    def bias_x(self) -> float:
        return self.bias[0]

    @property
    def bias_y(self) -> float:
        return self.bias[1]

    @property
    def bias_z(self) -> float:
        return self.bias[2]

    @property
    def scale_x(self) -> float:
        return self.scale[0]

    @property
    def scale_y(self) -> float:
        return self.scale[1]

    @property
    def scale_z(self) -> float:
        return self.scale[2]

    @classmethod
    def from_values(
        cls,
        bias_x: float,
        bias_y: float,
        bias_z: float,
        scale_x: float,
        scale_y: float,
        scale_z: float,
    ) -> "AccelCalibration":
        return cls(
            bias=np.array([bias_x, bias_y, bias_z]),
            scale=np.array([scale_x, scale_y, scale_z]),
        )

    def to_c_defines(self) -> str:
        return f"""// Accelerometer calibration coefficients
#define ACCEL_BIAS_X  {self.bias_x:.6f}f
#define ACCEL_BIAS_Y  {self.bias_y:.6f}f
#define ACCEL_BIAS_Z  {self.bias_z:.6f}f
#define ACCEL_SCALE_X {self.scale_x:.6f}f
#define ACCEL_SCALE_Y {self.scale_y:.6f}f
#define ACCEL_SCALE_Z {self.scale_z:.6f}f
"""


@dataclass
class GyroCalibration:
    bias: np.ndarray

    @property
    def bias_x(self) -> float:
        return self.bias[0]

    @property
    def bias_y(self) -> float:
        return self.bias[1]

    @property
    def bias_z(self) -> float:
        return self.bias[2]


def compute_accel_calibration(logs_dir: Optional[Path] = None) -> AccelCalibration:
    """
    Compute accelerometer calibration coefficients using 6-axis method.

    Returns:
        AccelCalibration: Calibration coefficients (bias and scale for each axis)
    """
    logs_dir = Path(logs_dir) if logs_dir else config.LOGS_DIR
    files = {
        "X_down": logs_dir / "accel_X_down_calibration_data.csv",
        "X_up": logs_dir / "accel_X_up_calibration_data.csv",
        "Y_down": logs_dir / "accel_Y_down_calibration_data.csv",
        "Y_up": logs_dir / "accel_Y_up_calibration_data.csv",
        "Z_down": logs_dir / "accel_Z_down_calibration_data.csv",
        "Z_up": logs_dir / "accel_Z_up_calibration_data.csv",
    }

    for name, path in files.items():
        if not path.exists():
            raise FileNotFoundError(f"Calibration file not found: {path}")

    avg_values = {}
    for name, path in files.items():
        data = read_imu_csv(path)
        ax, ay, az = data["ax"], data["ay"], data["az"]
        avg_values[name] = (np.mean(ax), np.mean(ay), np.mean(az))
        print(
            f"{name}: {len(ax)} samples, avg=[{avg_values[name][0]:.4f}, {avg_values[name][1]:.4f}, {avg_values[name][2]:.4f}] g"
        )

    X_down = avg_values["X_down"][0]
    X_up = avg_values["X_up"][0]
    Y_down = avg_values["Y_down"][1]
    Y_up = avg_values["Y_up"][1]
    Z_down = avg_values["Z_down"][2]
    Z_up = avg_values["Z_up"][2]

    bias_x = (X_down + X_up) / 2
    bias_y = (Y_down + Y_up) / 2
    bias_z = (Z_down + Z_up) / 2

    measured_scale_x = (X_down - X_up) / 2
    measured_scale_y = (Y_down - Y_up) / 2
    measured_scale_z = (Z_down - Z_up) / 2

    scale_x = 1.0 / measured_scale_x
    scale_y = 1.0 / measured_scale_y
    scale_z = 1.0 / measured_scale_z

    return AccelCalibration.from_values(
        bias_x, bias_y, bias_z, scale_x, scale_y, scale_z
    )


def compute_gyro_bias(csv_path: Optional[Path] = None) -> GyroCalibration:
    """
    Compute gyroscope bias from calibration data.

    Args:
        csv_path: Path to gyro calibration CSV file

    Returns:
        GyroCalibration: Gyroscope bias values
    """
    csv_path = (
        Path(csv_path) if csv_path else config.LOGS_DIR / config.GYRO_CALIBRATION_FILE
    )

    if not csv_path.exists():
        raise FileNotFoundError(f"Gyro calibration file not found: {csv_path}")

    data = read_imu_csv(csv_path)
    gx, gy, gz = data["gx"], data["gy"], data["gz"]

    gx = gx[~np.isnan(gx)]
    gy = gy[~np.isnan(gy)]
    gz = gz[~np.isnan(gz)]

    bias_x = np.mean(gx)
    bias_y = np.mean(gy)
    bias_z = np.mean(gz)

    print(f"Gyro bias: x={bias_x:.6f}, y={bias_y:.6f}, z={bias_z:.6f} dps")

    return GyroCalibration(bias=np.array([bias_x, bias_y, bias_z]))


def load_accel_calibration(logs_dir: Optional[Path] = None) -> AccelCalibration:
    """
    Load accelerometer calibration coefficients from file.

    Returns:
        AccelCalibration: Loaded calibration coefficients
    """
    logs_dir = Path(logs_dir) if logs_dir else config.LOGS_DIR
    filepath = logs_dir / config.CALIBRATION_FILE

    if not filepath.exists():
        print(f"Calibration file not found: {filepath}")
        print("Using default calibration (bias=0, scale=1)")
        return AccelCalibration(bias=np.zeros(3), scale=np.ones(3))

    coeffs = load_calibration_coefficients(filepath)

    bias = np.array(
        [
            coeffs.get("ACCEL_BIAS_X", 0.0),
            coeffs.get("ACCEL_BIAS_Y", 0.0),
            coeffs.get("ACCEL_BIAS_Z", 0.0),
        ]
    )

    scale = np.array(
        [
            coeffs.get("ACCEL_SCALE_X", 1.0),
            coeffs.get("ACCEL_SCALE_Y", 1.0),
            coeffs.get("ACCEL_SCALE_Z", 1.0),
        ]
    )

    print(f"Loaded calibration: bias={bias}, scale={scale}")

    return AccelCalibration(bias=bias, scale=scale)


def apply_accel_calibration(
    ax: np.ndarray, ay: np.ndarray, az: np.ndarray, calibration: AccelCalibration
) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Apply calibration to raw accelerometer data.

    Args:
        ax, ay, az: Raw accelerometer data (in g)
        calibration: Calibration coefficients

    Returns:
        Tuple of calibrated (ax, ay, az)
    """
    ax_cal = (ax - calibration.bias_x) * calibration.scale_x
    ay_cal = (ay - calibration.bias_y) * calibration.scale_y
    az_cal = (az - calibration.bias_z) * calibration.scale_z
    return ax_cal, ay_cal, az_cal


def apply_gyro_bias(
    gx: np.ndarray, gy: np.ndarray, gz: np.ndarray, calibration: GyroCalibration
) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Apply bias correction to raw gyroscope data.

    Args:
        gx, gy, gz: Raw gyroscope data (in dps)
        calibration: Gyroscope calibration

    Returns:
        Tuple of corrected (gx, gy, gz)
    """
    gx_corr = gx - calibration.bias_x
    gy_corr = gy - calibration.bias_y
    gz_corr = gz - calibration.bias_z
    return gx_corr, gy_corr, gz_corr


def evaluate_calibration_quality(
    calibration: AccelCalibration, logs_dir: Optional[Path] = None
) -> float:
    """
    Evaluate calibration quality by applying it to calibration data.

    Returns:
        Average RMS error in g
    """
    logs_dir = Path(logs_dir) if logs_dir else config.LOGS_DIR

    files = {
        "X_down": (logs_dir / "accel_X_down_calibration_data.csv", np.array([1, 0, 0])),
        "X_up": (logs_dir / "accel_X_up_calibration_data.csv", np.array([-1, 0, 0])),
        "Y_down": (logs_dir / "accel_Y_down_calibration_data.csv", np.array([0, 1, 0])),
        "Y_up": (logs_dir / "accel_Y_up_calibration_data.csv", np.array([0, -1, 0])),
        "Z_down": (logs_dir / "accel_Z_down_calibration_data.csv", np.array([0, 0, 1])),
        "Z_up": (logs_dir / "accel_Z_up_calibration_data.csv", np.array([0, 0, -1])),
    }

    total_error = 0.0

    for name, (path, expected) in files.items():
        if not path.exists():
            continue

        data = read_imu_csv(path)
        ax_cal, ay_cal, az_cal = apply_accel_calibration(
            data["ax"], data["ay"], data["az"], calibration
        )

        measured = np.array([np.mean(ax_cal), np.mean(ay_cal), np.mean(az_cal)])
        error = np.sqrt(np.sum((measured - expected) ** 2))
        total_error += error
        print(f"{name}: measured={measured}, expected={expected}, error={error:.4f} g")

    avg_error = total_error / len(files)

    if avg_error < 0.01:
        quality = "EXCELLENT"
    elif avg_error < 0.02:
        quality = "GOOD"
    elif avg_error < 0.05:
        quality = "ACCEPTABLE"
    else:
        quality = "POOR"

    print(f"Average RMS error: {avg_error:.4f} g")
    print(f"Calibration quality: {quality}")

    return avg_error


def save_calibration(
    calibration: AccelCalibration, logs_dir: Optional[Path] = None
) -> None:
    """Save calibration coefficients to file."""
    logs_dir = Path(logs_dir) if logs_dir else config.LOGS_DIR
    filepath = logs_dir / config.CALIBRATION_FILE

    coeffs = {
        "ACCEL_BIAS_X": calibration.bias_x,
        "ACCEL_BIAS_Y": calibration.bias_y,
        "ACCEL_BIAS_Z": calibration.bias_z,
        "ACCEL_SCALE_X": calibration.scale_x,
        "ACCEL_SCALE_Y": calibration.scale_y,
        "ACCEL_SCALE_Z": calibration.scale_z,
    }

    save_calibration_coefficients(filepath, coeffs)
    print(f"Calibration saved to: {filepath}")
