"""Check the 125-second normal-firmware serial acceptance capture."""

import argparse
from pathlib import Path
import re


REPORT = re.compile(
    r"FRAME_REPORT width=(\d+) height=(\d+) valid=(\d+) invalid=(\d+)"
    r" elapsed_ms=(\d+) fps=([\d.]+)"
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    text = args.log.read_text(encoding="utf-8")
    reports = [tuple(match.groups()) for match in REPORT.finditer(text)]
    boot_count = len(re.findall(r"rst:0x", text))
    warnings_errors = len(re.findall(r"\| [WE] \(\d+\)", text))
    scenario_count = text.count("FAULT_SCENARIO")
    recovery_count = text.count("RECOVERY stage=")
    fault_count = text.count("FAULT code=")
    healthy_reports = all(
        int(width) == 320 and int(height) == 240 and int(valid) > 0
        and int(invalid) == 0 and float(fps) > 0
        for width, height, valid, invalid, elapsed, fps in reports
    )
    print(f"boot_sequences={boot_count}")
    print(f"frame_reports={len(reports)} healthy_reports={healthy_reports}")
    if reports:
        print(
            f"valid_total={sum(int(row[2]) for row in reports)} "
            f"invalid_total={sum(int(row[3]) for row in reports)} "
            f"fps_min={min(float(row[5]) for row in reports):.2f} "
            f"fps_max={max(float(row[5]) for row in reports):.2f}"
        )
    print(f"warnings_errors={warnings_errors} fault_scenarios={scenario_count}")
    print(f"recovery_events={recovery_count} faults={fault_count}")
    passed = (
        boot_count == 1 and len(reports) >= 24 and healthy_reports
        and warnings_errors == 0 and scenario_count == 0
        and recovery_count == 0 and fault_count == 0
    )
    print("FINAL_LOG PASS" if passed else "FINAL_LOG FAIL")
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
