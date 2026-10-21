
"""
BMP280 HIL — Pass 9.11.3

Requirements Coverage Quality Gates.

G1: All requirements must have mapped tests.
G2: Mapped test IDs must exist in the pytest inventory.
G3: Optional verification-evidence gate.

Exit codes:
    0 = all selected gates passed
    1 = one or more gates failed
"""

import argparse
import csv
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / "reports"

TRACEABILITY = REPORTS / "requirements-traceability.csv"
VERIFICATION = REPORTS / "requirements-verification.csv"
OUTPUT = REPORTS / "quality-gates.json"


def read_csv(path):
    """Read a CSV file and return its records."""

    if not path.is_file():
        raise FileNotFoundError(
            f"Required report not found: {path}"
        )

    with path.open(
        encoding="utf-8-sig",
        newline="",
    ) as file:
        return list(csv.DictReader(file))


def split_tests(value):
    """Convert semicolon-separated test IDs into a list."""

    return [
        item.strip()
        for item in (value or "").split(";")
        if item.strip()
    ]


def evaluate_mapping(rows):
    """
    G1 — Every requirement must map to a test.
    Also reject duplicate requirement IDs.
    """

    failures = []
    seen = set()

    if not rows:
        failures.append("Traceability report is empty")

    for row in rows:
        requirement_id = (
            row.get("requirement_id") or ""
        ).strip()

        tests = split_tests(row.get("tests", ""))

        if not requirement_id:
            failures.append("Empty requirement ID")
            continue

        if requirement_id in seen:
            failures.append(
                f"Duplicate requirement: {requirement_id}"
            )

        seen.add(requirement_id)

        if not tests:
            failures.append(
                f"Unmapped requirement: {requirement_id}"
            )

    return failures


def evaluate_test_references(rows, collected_tests):
    """
    G2 — Every mapped test must exist in the
    collected pytest test inventory.
    """

    failures = []

    for row in rows:
        requirement_id = row.get("requirement_id", "")
        tests = split_tests(row.get("tests", ""))

        for test_id in tests:
            if test_id not in collected_tests:
                failures.append(
                    f"{requirement_id}: unknown test {test_id}"
                )

    return failures


def evaluate_verification(rows):
    """
    G3 — All requirements must have PASSED status.

    This gate requires fresh execution evidence.
    It is intended for local/HIL release verification,
    not ordinary GitHub-hosted CI.
    """

    failures = []

    if not rows:
        failures.append("Verification report is empty")

    for row in rows:
        requirement_id = row.get("requirement_id", "")
        status = row.get("verification_status", "")

        if status != "PASSED":
            failures.append(
                f"{requirement_id}: status {status}"
            )

    return failures


def evaluate_quality_gates(
    traceability,
    collected_tests,
    verification=None,
):
    """Evaluate all applicable gates."""

    mapping_failures = evaluate_mapping(traceability)

    reference_failures = evaluate_test_references(
        traceability,
        collected_tests,
    )

    gates = {
        "G1_requirements_mapping": {
            "passed": not mapping_failures,
            "failures": mapping_failures,
        },
        "G2_test_reference_validity": {
            "passed": not reference_failures,
            "failures": reference_failures,
        },
    }

    if verification is not None:
        verification_failures = evaluate_verification(
            verification
        )

        gates["G3_verification_evidence"] = {
            "passed": not verification_failures,
            "failures": verification_failures,
        }

    return {
        "passed": all(
            gate["passed"]
            for gate in gates.values()
        ),
        "gates": gates,
    }


def collect_test_ids():
    """
    Collect real pytest node IDs without running tests.

    A separate pytest process avoids interfering with
    the quality-gate script's own execution.
    """

    import subprocess

    command = [
        sys.executable,
        "-m",
        "pytest",
        "-c",
        str(ROOT / "pytest.ini"),
        "--collect-only",
        "-q",
        str(ROOT / "tests"),
    ]

    result = subprocess.run(
        command,
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        raise RuntimeError(
            "Pytest collection failed:\n"
            + result.stdout
            + result.stderr
        )

    test_ids = set()

    for line in result.stdout.splitlines():
        line = line.strip()

        if "::" in line and ".py::" in line:
            test_ids.add(line)

    return test_ids


def main():
    parser = argparse.ArgumentParser(
        description="BMP280 HIL requirements quality gates"
    )

    parser.add_argument(
        "--require-verification",
        action="store_true",
        help="Enable G3 verification-evidence gate",
    )

    args = parser.parse_args()

    REPORTS.mkdir(parents=True, exist_ok=True)

    try:
        traceability = read_csv(TRACEABILITY)
        collected_tests = collect_test_ids()

        verification = None

        if args.require_verification:
            verification = read_csv(VERIFICATION)

        summary = evaluate_quality_gates(
            traceability,
            collected_tests,
            verification,
        )

    except (FileNotFoundError, RuntimeError, ValueError) as exc:
        summary = {
            "passed": False,
            "error": str(exc),
            "gates": {},
        }

    with OUTPUT.open(
        "w",
        encoding="utf-8",
    ) as file:
        json.dump(summary, file, indent=2)

    print("\nBMP280 Requirements Quality Gates")
    print("--------------------------------")

    for name, gate in summary.get("gates", {}).items():
        status = "PASS" if gate["passed"] else "FAIL"

        print(f"{name}: {status}")

        for failure in gate["failures"]:
            print(f"  - {failure}")

    if "error" in summary:
        print("ERROR:", summary["error"])

    print("\nReport:", OUTPUT)

    return 0 if summary["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
