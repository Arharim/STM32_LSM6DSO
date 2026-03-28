# Python IMU Tools

This package provides Python tools for IMU (LSM6DSO) data visualization and calibration.

## Installation

```bash
pip install numpy matplotlib
```

## Usage

### Run Accelerometer Calibration

```bash
cd plotVis
python run_calibration.py --evaluate
```

### Plot IMU Data

```bash
python plot_logs.py [--fs SAMPLING_FREQ] [--plots ts mag hist psd 2d]
```

### Test Calibration

```bash
python test_calibration.py [--monitor-file monitor.csv]
```

### As a Module

```python
from plotVis import (
    compute_accel_calibration,
    load_accel_calibration,
    apply_accel_calibration,
    plot_logs,
)

# Compute calibration
calibration = compute_accel_calibration()
print(f"Bias: {calibration.bias}")
print(f"Scale: {calibration.scale}")

# Apply to data
ax_cal, ay_cal, az_cal = apply_accel_calibration(ax, ay, az, calibration)

# Visualize data
plot_logs(fs=100.0, plot_types=["ts", "mag", "psd"])
```

## Structure

```
plotVis/
├── __init__.py           # Package exports
├── config.py             # Configuration (paths, etc.)
├── run_calibration.py    # CLI for calibration
├── plot_logs.py          # CLI for visualization
├── test_calibration.py   # CLI for testing
├── calibration/
│   ├── __init__.py
│   └── calibration.py    # Calibration algorithms
├── visualization/
│   ├── __init__.py
│   ├── plots.py          # Plotting functions
│   └── psd.py            # PSD calculation
└── utils/
    ├── __init__.py
    └── io.py             # File I/O
```

## Comparison with MATLAB/Octave

| MATLAB Script | Python Equivalent |
|---------------|-------------------|
| `accel_6axis_calibration.m` | `calibration.compute_accel_calibration()` |
| `compute_accel_calibration.m` | (removed - duplicate) |
| `accel_calibration_simple.m` | (removed - duplicate) |
| `load_accel_calibration.m` | `calibration.load_accel_calibration()` |
| `apply_accel_calibration.m` | `calibration.apply_accel_calibration()` |
| `test_accel_calibration.m` | `test_calibration.py` |
| `compute_gyro_bias.m` | `calibration.compute_gyro_bias()` |
| `plot_logs.m` | `visualization.plot_logs()` |
| `simple_psd.m` | `visualization.simple_psd()` |
