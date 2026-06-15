#!/usr/bin/env python3
import argparse
import subprocess
from pathlib import Path


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def write_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def normalize(text: str) -> str:
    return text.replace("\r\n", "\n")


def main() -> int:
    parser = argparse.ArgumentParser(description="MiniLangVM golden test runner")
    parser.add_argument("--minilang", default="./build/minilang", help="path to minilang executable")
    parser.add_argument("--tests", default="tests/golden", help="directory with .ml golden tests")
    parser.add_argument("--update", action="store_true", help="update golden stdout/stderr/code files")
    args = parser.parse_args()

    exe = Path(args.minilang)
    tests_dir = Path(args.tests)

    if not exe.exists():
        print(f"error: minilang executable not found: {exe}")
        return 2

    test_files = sorted(tests_dir.glob("*.ml"))

    if not test_files:
        print(f"error: no .ml tests found in {tests_dir}")
        return 2

    failed = 0

    for test in test_files:
        print(f"[ RUN      ] {test}")

        proc = subprocess.run(
            [str(exe), str(test)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        actual_stdout = normalize(proc.stdout)
        actual_stderr = normalize(proc.stderr)
        actual_code = str(proc.returncode) + "\n"

        out_file = test.with_suffix(".stdout")
        err_file = test.with_suffix(".stderr")
        code_file = test.with_suffix(".code")

        if args.update:
            write_text(out_file, actual_stdout)
            write_text(err_file, actual_stderr)
            write_text(code_file, actual_code)
            print(f"[ UPDATED  ] {test.name}")
            continue

        expected_stdout = normalize(read_text(out_file))
        expected_stderr = normalize(read_text(err_file))
        expected_code = normalize(read_text(code_file))

        ok = True

        if actual_stdout != expected_stdout:
            ok = False
            print(f"[ STDOUT MISMATCH ] {test.name}")
            print("expected:")
            print(expected_stdout)
            print("actual:")
            print(actual_stdout)

        if actual_stderr != expected_stderr:
            ok = False
            print(f"[ STDERR MISMATCH ] {test.name}")
            print("expected:")
            print(expected_stderr)
            print("actual:")
            print(actual_stderr)

        if actual_code != expected_code:
            ok = False
            print(f"[ EXIT CODE MISMATCH ] {test.name}")
            print(f"expected: {expected_code.strip()}")
            print(f"actual:   {actual_code.strip()}")

        if ok:
            print(f"[       OK ] {test.name}")
        else:
            failed += 1
            print(f"[  FAILED  ] {test.name}")

    if args.update:
        print("golden files updated")
        return 0

    if failed:
        print(f"{failed} golden test(s) failed")
        return 1

    print("all golden tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
