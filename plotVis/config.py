from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict


@dataclass
class Config:
    BASE_DIR: Path = field(default_factory=lambda: Path(__file__).parent)
    LOGS_DIR: Path = field(default=None)
    CALIBRATION_FILE: str = "accel_calibration_coefficients.txt"
    ACCEL_CALIBRATION_FILES: Dict[str, Path] = field(default_factory=dict)
    GYRO_CALIBRATION_FILE: str = "gyro_calibration_data.csv"

    def __post_init__(self):
        if self.LOGS_DIR is None:
            self.LOGS_DIR = self.BASE_DIR.parent / "logs"
        self.LOGS_DIR = Path(self.LOGS_DIR)
        self.ACCEL_CALIBRATION_FILES = {
            "X_down": self.LOGS_DIR / "accel_X_down_calibration_data.csv",
            "X_up": self.LOGS_DIR / "accel_X_up_calibration_data.csv",
            "Y_down": self.LOGS_DIR / "accel_Y_down_calibration_data.csv",
            "Y_up": self.LOGS_DIR / "accel_Y_up_calibration_data.csv",
            "Z_down": self.LOGS_DIR / "accel_Z_down_calibration_data.csv",
            "Z_up": self.LOGS_DIR / "accel_Z_up_calibration_data.csv",
        }

    def get_calibration_path(self) -> Path:
        return self.LOGS_DIR / self.CALIBRATION_FILE


config = Config()
