"""Capture ESP32 UART output to a timestamped UTF-8 file without a terminal."""

import argparse
from datetime import datetime, timezone
from pathlib import Path
import sys
import time


def positive_seconds(value: str) -> float:
    seconds = float(value)
    if not 0 < seconds <= 3600:
        raise argparse.ArgumentTypeError("seconds must be in (0, 3600]")
    return seconds


def arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Windows COM port, for example COM5")
    parser.add_argument("--seconds", required=True, type=positive_seconds)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--baud", type=int, default=115200)
    return parser.parse_args()


def write_line(output, raw: bytes) -> None:
    timestamp = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
    output.write(f"{timestamp} | {raw.decode('utf-8', errors='replace').rstrip()}\n")
    output.flush()


def main() -> int:
    args = arguments()
    try:
        import serial
    except ImportError:
        print("pyserial is missing from this Python environment", file=sys.stderr)
        return 2

    args.output.parent.mkdir(parents=True, exist_ok=True)
    try:
        port_handle = serial.Serial(port=None, baudrate=args.baud, timeout=0.2)
        # Keep GPIO0 (DTR) released before opening CH343. Opening with pyserial's
        # default asserted DTR/RTS can leave ESP32-S3 in DOWNLOAD mode.
        port_handle.dtr = False
        port_handle.rts = False
        port_handle.port = args.port
        with port_handle as port:
            port.dtr = False
            port.rts = True  # EN low
            time.sleep(0.1)
            port.rts = False  # EN high, boot app with GPIO0 high
            with args.output.open("w", encoding="utf-8", newline="\n") as output:
                output.write(
                    f"# capture port={args.port} baud={args.baud} "
                    f"seconds={args.seconds} started_utc="
                    f"{datetime.now(timezone.utc).isoformat(timespec='seconds')}\n"
                )
                output.flush()
                deadline = time.monotonic() + args.seconds
                pending = bytearray()
                while time.monotonic() < deadline:
                    pending.extend(port.read(max(1, port.in_waiting)))
                    while b"\n" in pending:
                        line, _, remainder = pending.partition(b"\n")
                        write_line(output, line)
                        pending = bytearray(remainder)
                if pending:
                    write_line(output, pending)
    except (OSError, serial.SerialException) as error:
        print(f"Serial capture failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
