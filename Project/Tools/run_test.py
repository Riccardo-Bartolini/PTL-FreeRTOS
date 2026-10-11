"""Run one regression test in QEMU; preserve logs and reject any failure.

SPDX-License-Identifier: MIT
"""

import argparse
import math
from pathlib import Path
import re
import subprocess
import sys


def result_passed(output):
    """A PASS cannot mask a FAIL, fatal error, assertion, or hard fault."""
    return bool(re.search(r"\[PASS\]", output)) and not re.search(
        r"\[FAIL(?:\]|\s|-)|\[FATAL\]|\[ERROR\]|HardFault|hardfault|assert(?:ion)? failed",
        output, re.IGNORECASE,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qemu", default="qemu-system-arm")
    parser.add_argument("--elf", required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--duration", type=float, default=5)
    parser.add_argument("--show", choices=("0", "1"), default="0")
    parser.add_argument("--show-on-screen", choices=("0", "1"), default="0")
    args = parser.parse_args()
    if not math.isfinite(args.duration) or args.duration <= 0:
        parser.error("duration must be a finite positive number")

    log_dir = Path("logs")
    log_dir.mkdir(exist_ok=True)
    log_path = log_dir / (Path(args.name).stem + ".log")
    command = [args.qemu, "-machine", "mps2-an385", "-cpu", "cortex-m3",
               "-kernel", args.elf, "-monitor", "none", "-nographic",
               "-serial", "stdio", "-icount", "1"]
    interrupted = False
    with log_path.open("w", encoding="utf-8") as log:
        try:
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
        except OSError as error:
            print(f"Could not launch QEMU: {error}", file=sys.stderr)
            return 1
        timed_out = False
        try:
            process.wait(timeout=args.duration)
        except subprocess.TimeoutExpired:
            # Bare-metal tests halt in a loop after printing their verdict.
            timed_out = True
        except KeyboardInterrupt:
            interrupted = True
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()

    output = log_path.read_text(encoding="utf-8", errors="replace")
    passed = (not interrupted and (timed_out or process.returncode == 0)
              and result_passed(output))
    verdict = "PASSED" if passed else "FAILED"
    if args.show == "1":
        summary = output
    else:
        statistics = re.findall(r"====== PTL STATISTICS ======.*?============================", output, re.S)
        checks = [line for line in output.splitlines()
                  if re.search(r"\[(?:PASS|FAIL|FATAL|ERROR|CHECK)\]", line)]
        summary = "\n".join(statistics + checks) + "\n"

    with Path("qemu_test.log").open("a", encoding="utf-8") as master:
        master.write(f"=== {args.name}: {verdict} ===\n{summary}\n")
    print(f" -> {verdict}: {args.name} (full log: {log_path})")
    if not passed:
        print(output)
    elif args.show_on_screen == "1":
        print(summary)
    return 130 if interrupted else (0 if passed else 1)


if __name__ == "__main__":
    sys.exit(main())
