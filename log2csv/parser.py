#!/usr/bin/env python3
import glob
import os
import sys
from pathlib import Path


def convert_log_to_csv(input_file: str, output_file: str) -> None:
    with open(input_file, "r") as f_in, open(output_file, "w") as f_out:
        header_found = False
        index = 0

        for line in f_in:
            line = line.strip()
            if not line:
                continue

            if line.startswith("gx_dps"):
                f_out.write(f"Index,{line}\n")
                header_found = True
                continue

            if not header_found:
                continue

            f_out.write(f"{index},{line}\n")
            index += 1


def main():
    logs_dir = Path(__file__).parent.parent / "logs"

    log_files = list(logs_dir.glob("*.log"))

    if not log_files:
        print(f"No .log files in the folder: {logs_dir}")
        return

    for input_file in log_files:
        output_file = input_file.with_suffix(".csv")

        try:
            convert_log_to_csv(str(input_file), str(output_file))
            print(f"Ok {input_file.name} → {output_file.name}")
        except Exception as e:
            print(f"Error during processing {input_file}: {e}")

    print("Conversion completed")


if __name__ == "__main__":
    main()
