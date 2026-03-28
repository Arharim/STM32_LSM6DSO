# Python IMU Tools

Python tools for IMU (LSM6DSO) data visualization and calibration.

## Installation

```bash
pip install -r requirements.txt
```

Or manually:
```bash
pip install numpy matplotlib
```

## Usage

All scripts must be run from the `plotVis` directory:

```bash
cd plotVis
```

### Run Accelerometer Calibration

```bash
python run_calibration.py --evaluate
```

Options:
- `--logs-dir PATH` - Directory containing calibration CSV files (default: `../logs/`)
- `--evaluate` - Evaluate calibration quality after computation

### Plot IMU Data

```bash
python plot_logs.py --fs 100 --plots ts mag hist psd 2d
```

Options:
- `--logs-dir PATH` - Directory containing CSV files (default: `../logs/`)
- `--fs FREQ` - Sampling frequency in Hz (optional)
- `--plots TYPES` - Plot types: `ts`, `mag`, `hist`, `psd`, `2d`, `3d_accel`, `3d_gyro`

### Test Calibration

```bash
python test_calibration.py --monitor-file monitor.csv
```

Options:
- `--logs-dir PATH` - Directory containing CSV files
- `--monitor-file FILE` - CSV file to test on (default: `monitor.csv`)
- `--save-plot PATH` - Save plot to file

### As a Module

```python
from plotVis import (
    compute_accel_calibration,
    load_accel_calibration,
    apply_accel_calibration,
    read_imu_csv,
)

# Compute calibration
calibration = compute_accel_calibration()
print(f"Bias: {calibration.bias}")
print(f"Scale: {calibration.scale}")

# Apply to data
data = read_imu_csv("monitor.csv")
ax_cal, ay_cal, az_cal = apply_accel_calibration(
    data["ax"], data["ay"], data["az"], calibration
)
```

## Structure

```
plotVis/
├── __init__.py           # Package exports
├── config.py             # Configuration (paths, etc.)
├── run_calibration.py    # CLI: run accel calibration
├── plot_logs.py          # CLI: visualize IMU data
├── test_calibration.py   # CLI: test calibration quality
├── requirements.txt      # Dependencies
├── calibration/
│   ├── __init__.py
│   └── calibration.py    # AccelCalibration, GyroCalibration classes
├── visualization/
│   ├── __init__.py
│   ├── plots.py          # Plotting functions
│   └── psd.py            # PSD calculation
└── utils/
    ├── __init__.py
    └── io.py             # CSV reading, calibration file I/O
```

## Migration from MATLAB/Octave

| MATLAB Script | Python Equivalent |
|---------------|-------------------|
| `accel_6axis_calibration.m` | `calibration.compute_accel_calibration()` |
| `compute_accel_calibration.m` | *(removed - duplicate)* |
| `accel_calibration_simple.m` | *(removed - duplicate)* |
| `load_accel_calibration.m` | `calibration.load_accel_calibration()` |
| `apply_accel_calibration.m` | `calibration.apply_accel_calibration()` |
| `test_accel_calibration.m` | `test_calibration.py` |
| `compute_gyro_bias.m` | `calibration.compute_gyro_bias()` |
| `plot_logs.m` | `plot_logs.py` |
| `simple_psd.m` | `visualization.simple_psd()` |

## Output Files

Calibration generates:
- `logs/accel_calibration_coefficients.txt` - Coefficients in KEY=VALUE format (for Python)
- C code constants printed to console (copy to embedded project)
