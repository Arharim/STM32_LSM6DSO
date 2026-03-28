from typing import Dict
import numpy as np
from pathlib import Path


def read_imu_csv(filepath: Path) -> Dict[str, np.ndarray]:
    """
    Read IMU data from CSV file.

    Expected format:
        Index,gx_dps,gy_dps,gz_dps,ax_g,ay_g,az_g
        0,-30.52,+8.54,+8.12,+0.051,-0.018,+1.013

    Returns:
        Dictionary with keys: index, gx, gy, gz, ax, ay, az
    """
    data = np.genfromtxt(filepath, delimiter=",", skip_header=1)

    return {
        "index": data[:, 0],
        "gx": data[:, 1],
        "gy": data[:, 2],
        "gz": data[:, 3],
        "ax": data[:, 4],
        "ay": data[:, 5],
        "az": data[:, 6],
    }


def save_calibration_coefficients(filepath: Path, coeffs: Dict[str, float]) -> None:
    """
    Save calibration coefficients to file in format compatible with C code.

    Args:
        filepath: Output file path
        coeffs: Dictionary of coefficient names and values
    """
    from datetime import datetime

    with open(filepath, "w") as f:
        f.write("# Accelerometer calibration coefficients (6-axis method)\n")
        f.write(f"# Created: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("\n")
        f.write("# Bias values (in g)\n")
        f.write(f"ACCEL_BIAS_X={coeffs.get('ACCEL_BIAS_X', 0.0):.6f}\n")
        f.write(f"ACCEL_BIAS_Y={coeffs.get('ACCEL_BIAS_Y', 0.0):.6f}\n")
        f.write(f"ACCEL_BIAS_Z={coeffs.get('ACCEL_BIAS_Z', 0.0):.6f}\n")
        f.write("\n")
        f.write("# Scale factors\n")
        f.write(f"ACCEL_SCALE_X={coeffs.get('ACCEL_SCALE_X', 1.0):.6f}\n")
        f.write(f"ACCEL_SCALE_Y={coeffs.get('ACCEL_SCALE_Y', 1.0):.6f}\n")
        f.write(f"ACCEL_SCALE_Z={coeffs.get('ACCEL_SCALE_Z', 1.0):.6f}\n")
        f.write("\n")
        f.write("# Application formula:\n")
        f.write("# ax_calibrated = (ax_raw - ACCEL_BIAS_X) * ACCEL_SCALE_X\n")
        f.write("# ay_calibrated = (ay_raw - ACCEL_BIAS_Y) * ACCEL_SCALE_Y\n")
        f.write("# az_calibrated = (az_raw - ACCEL_BIAS_Z) * ACCEL_SCALE_Z\n")


def load_calibration_coefficients(filepath: Path) -> Dict[str, float]:
    """
    Load calibration coefficients from file.

    Returns:
        Dictionary of coefficient names and values
    """
    coeffs = {}

    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            if "=" in line:
                name, value = line.split("=", 1)
                coeffs[name.strip()] = float(value.strip())

    return coeffs
