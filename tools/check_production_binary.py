"""Check that fault-scenario markers are absent from a normal firmware build."""

import argparse
from pathlib import Path


MARKERS = (
    b"FAULT_SCENARIO",
    b"INVALID_FRAME",
    b"RECOVERY_TRIGGER",
    b"WORKER_DELAY",
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("map_file", type=Path)
    parser.add_argument("sdkconfig", type=Path)
    args = parser.parse_args()

    config = args.sdkconfig.read_text(encoding="utf-8")
    config_ok = "# CONFIG_BRINGUP_FAULT_INJECTION is not set" in config
    print(f"fault_injection_disabled={config_ok}")
    clean = config_ok
    for path in (args.elf, args.map_file):
        content = path.read_bytes()
        for marker in MARKERS:
            absent = marker not in content
            print(f"{path.name}: {marker.decode()} absent={absent}")
            clean &= absent
    print("PRODUCTION_BINARY PASS" if clean else "PRODUCTION_BINARY FAIL")
    return 0 if clean else 1


if __name__ == "__main__":
    raise SystemExit(main())
