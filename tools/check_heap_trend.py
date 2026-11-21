"""Summarize free heaps at recovery starts in a serial capture."""

import argparse
from pathlib import Path
import re


HEAP = re.compile(
    r"HEAP stage=(\S+).*psram_heap_free_bytes=(\d+) "
    r"internal_heap_free_bytes=(\d+)"
)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--min-cycles", type=int, default=30)
    parser.add_argument("--max-spread-bytes", type=int, default=1024)
    args = parser.parse_args()

    samples = []
    text = args.log.read_text(encoding="utf-8")
    for line in text.splitlines():
        match = HEAP.search(line)
        if match and match.group(1) == "recovery_start":
            samples.append((int(match.group(2)), int(match.group(3))))

    print(f"recovery_start_samples={len(samples)}")
    print(f"boot_sequences={len(re.findall(r'rst:0x', text))}")
    print(f"successful_recoveries={text.count('RECOVERY stage=success')}")
    print(f"budget_resets={text.count('RECOVERY stage=budget_reset')}")
    if not samples:
        return 1
    for name, column in (("psram", 0), ("internal", 1)):
        values = [sample[column] for sample in samples]
        spread = max(values) - min(values)
        print(
            f"{name}_free_bytes first={values[0]} last={values[-1]} "
            f"min={min(values)} max={max(values)} spread={spread}"
        )
        if spread > args.max_spread_bytes:
            print(f"FAIL: {name} spread exceeds {args.max_spread_bytes} bytes")
            return 1
    if len(samples) < args.min_cycles:
        print(f"FAIL: fewer than {args.min_cycles} recovery starts")
        return 1
    print("HEAP_TREND PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
