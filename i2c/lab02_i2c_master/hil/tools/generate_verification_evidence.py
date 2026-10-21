
"""
BMP280 HIL — Pass 9.10

Combine requirements traceability with pytest JUnit XML
execution results to produce verification evidence.

Statuses:
    UNMAPPED
    NOT_EXECUTED
    PASSED
    FAILED
    INCOMPLETE
"""

import csv
import json
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / "reports"

TRACEABILITY = REPORTS / "requirements-traceability.csv"

JUNIT_FILES = [
    REPORTS / "simulation-results.xml",
    REPORTS / "hardware-results.xml",
]

OUTPUT_CSV = REPORTS / "requirements-verification.csv"
OUTPUT_JSON = REPORTS / "verification-evidence.json"


def read_traceability():
    if not TRACEABILITY.exists():
        raise FileNotFoundError(
            "Traceability report missing. Run "
            "generate_traceability.py first."
        )

    with TRACEABILITY.open(
        encoding="utf-8-sig", newline=""
    ) as file:
        return list(csv.DictReader(file))


def normalize_test_id(classname, name):
    """
    Convert pytest JUnit identifiers into pytest node IDs.

    Example:
        classname = tests.test_connectivity
        name = test_ping

    Result:
        tests/test_connectivity.py::test_ping
    """
    module_path = classname.replace(".", "/")
    return f"{module_path}.py::{name}"


def read_junit_results():
    results = defaultdict(list)

    for junit_file in JUNIT_FILES:
        if not junit_file.exists():
            print(
                f"WARNING: Missing execution report: "
                f"{junit_file.name}"
            )
            continue

        tree = ET.parse(junit_file)

        for testcase in tree.iter("testcase"):
            classname = testcase.get("classname", "")
            name = testcase.get("name", "")

            if not classname or not name:
                continue

            test_id = normalize_test_id(classname, name)

            if testcase.find("failure") is not None:
                status = "FAILED"
            elif testcase.find("error") is not None:
                status = "FAILED"
            elif testcase.find("skipped") is not None:
                status = "SKIPPED"
            else:
                status = "PASSED"

            results[test_id].append({
                "status": status,
                "source": junit_file.name,
            })

    return results


def requirement_status(tests, results):
    if not tests:
        return "UNMAPPED"

    statuses = []

    for test in tests:
        executions = results.get(test, [])

        if not executions:
            statuses.append("NOT_EXECUTED")
            continue

        execution_statuses = [
            item["status"] for item in executions
        ]

        if "FAILED" in execution_statuses:
            statuses.append("FAILED")
        elif all(
            status == "PASSED"
            for status in execution_statuses
        ):
            statuses.append("PASSED")
        else:
            statuses.append("INCOMPLETE")

    if "FAILED" in statuses:
        return "FAILED"

    if all(status == "PASSED" for status in statuses):
        return "PASSED"

    if all(
        status == "NOT_EXECUTED"
        for status in statuses
    ):
        return "NOT_EXECUTED"

    return "INCOMPLETE"


def generate_evidence():
    traceability = read_traceability()
    results = read_junit_results()

    rows = []

    for requirement in traceability:
        tests = [
            test.strip()
            for test in requirement["tests"].split(";")
            if test.strip()
        ]

        status = requirement_status(tests, results)

        rows.append({
            "requirement_id": requirement["requirement_id"],
            "description": requirement["description"],
            "category": requirement["category"],
            "mapped_tests": len(tests),
            "verification_status": status,
            "tests": "; ".join(tests),
        })

    fieldnames = [
        "requirement_id",
        "description",
        "category",
        "mapped_tests",
        "verification_status",
        "tests",
    ]

    with OUTPUT_CSV.open(
        "w", encoding="utf-8", newline=""
    ) as file:
        writer = csv.DictWriter(
            file, fieldnames=fieldnames
        )
        writer.writeheader()
        writer.writerows(rows)

    counts = {
        status: sum(
            row["verification_status"] == status
            for row in rows
        )
        for status in [
            "PASSED",
            "FAILED",
            "INCOMPLETE",
            "NOT_EXECUTED",
            "UNMAPPED",
        ]
    }

    total = len(rows)

    summary = {
        "total_requirements": total,
        "verified_requirements": counts["PASSED"],
        "verification_percent": round(
            100 * counts["PASSED"] / total, 2
        ) if total else 0.0,
        "status_counts": counts,
        "requirements": rows,
    }

    with OUTPUT_JSON.open(
        "w", encoding="utf-8"
    ) as file:
        json.dump(summary, file, indent=2)

    return summary


def main():
    summary = generate_evidence()

    print("\nBMP280 Verification Evidence")
    print("---------------------------")

    print(
        f"Total requirements: "
        f"{summary['total_requirements']}"
    )

    print(
        f"Verified requirements: "
        f"{summary['verified_requirements']}"
    )

    print(
        f"Verification coverage: "
        f"{summary['verification_percent']}%"
    )

    print("\nStatus counts:")

    for status, count in summary["status_counts"].items():
        print(f"  {status}: {count}")

    print("\nReports generated:")
    print(f"  {OUTPUT_CSV}")
    print(f"  {OUTPUT_JSON}")

    if summary["verified_requirements"] != summary["total_requirements"]:
        sys.exit(1)


if __name__ == "__main__":
    main()
