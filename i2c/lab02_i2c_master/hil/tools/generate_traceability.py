
"""
BMP280 HIL Requirements Traceability Generator.

Pass 9.9:
- Load requirements from CSV
- Collect pytest requirement markers
- Generate a requirements-to-tests matrix
- Identify uncovered requirements
- Detect invalid requirement references

This measures test mapping coverage, not test pass coverage.
"""

import csv
import json
from collections import defaultdict
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[1]

REQUIREMENTS_FILE = (
    ROOT / "requirements" / "bmp280_requirements.csv"
)

REPORTS_DIR = ROOT / "reports"


class RequirementCollector:
    def __init__(self):
        self.mappings = defaultdict(list)
        self.errors = []

    def pytest_collection_modifyitems(self, items):
        for item in items:
            markers = list(
                item.iter_markers(name="requirement")
            )

            for marker in markers:
                if len(marker.args) != 1:
                    self.errors.append(
                        f"Invalid requirement marker: {item.nodeid}"
                    )
                    continue

                requirement_id = marker.args[0]

                if not isinstance(requirement_id, str):
                    self.errors.append(
                        f"Non-string requirement ID: {item.nodeid}"
                    )
                    continue

                self.mappings[requirement_id].append(
                    item.nodeid
                )


def load_requirements():
    requirements = {}

    with REQUIREMENTS_FILE.open(
        "r", encoding="utf-8-sig", newline=""
    ) as file:
        reader = csv.DictReader(file)

        for row in reader:
            requirement_id = row["requirement_id"].strip()

            if not requirement_id:
                raise ValueError(
                    "Requirement ID must not be empty"
                )

            if requirement_id in requirements:
                raise ValueError(
                    f"Duplicate requirement: {requirement_id}"
                )

            requirements[requirement_id] = {
                "description": row["description"].strip(),
                "category": row["category"].strip(),
            }

    return requirements


def generate_reports(requirements, mappings):
    REPORTS_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    matrix_path = (
        REPORTS_DIR / "requirements-traceability.csv"
    )

    json_path = (
        REPORTS_DIR / "requirements-coverage.json"
    )

    covered = 0
    matrix = []

    for requirement_id, details in requirements.items():
        tests = sorted(set(mappings.get(requirement_id, [])))

        if tests:
            covered += 1

        matrix.append({
            "requirement_id": requirement_id,
            "description": details["description"],
            "category": details["category"],
            "test_count": len(tests),
            "tests": "; ".join(tests),
            "status": "MAPPED" if tests else "UNMAPPED",
        })

    with matrix_path.open(
        "w", encoding="utf-8", newline=""
    ) as file:
        writer = csv.DictWriter(
            file,
            fieldnames=[
                "requirement_id",
                "description",
                "category",
                "test_count",
                "tests",
                "status",
            ],
        )

        writer.writeheader()
        writer.writerows(matrix)

    total = len(requirements)

    coverage = (
        100.0 * covered / total
        if total else 0.0
    )

    summary = {
        "total_requirements": total,
        "mapped_requirements": covered,
        "unmapped_requirements": total - covered,
        "mapping_coverage_percent": round(coverage, 2),
        "requirements": matrix,
    }

    with json_path.open(
        "w", encoding="utf-8"
    ) as file:
        json.dump(
            summary,
            file,
            indent=2
        )

    return summary


def main():
    requirements = load_requirements()

    collector = RequirementCollector()

    result = pytest.main(
        [
            "-c",
            str(ROOT / "pytest.ini"),
            "--collect-only",
            "-q",
            str(ROOT / "tests"),
        ],
        plugins=[collector],
    )

    if result != pytest.ExitCode.OK:
        raise SystemExit(
            f"Pytest collection failed: {result}"
        )

    invalid_ids = (
        set(collector.mappings) - set(requirements)
    )

    if collector.errors or invalid_ids:
        for error in collector.errors:
            print(f"ERROR: {error}")

        for requirement_id in sorted(invalid_ids):
            print(
                f"ERROR: Unknown requirement ID: "
                f"{requirement_id}"
            )

        raise SystemExit(1)

    summary = generate_reports(
        requirements,
        collector.mappings,
    )

    print("\nBMP280 Requirements Traceability")
    print("--------------------------------")

    print(
        f"Total requirements: "
        f"{summary['total_requirements']}"
    )

    print(
        f"Mapped requirements: "
        f"{summary['mapped_requirements']}"
    )

    print(
        f"Unmapped requirements: "
        f"{summary['unmapped_requirements']}"
    )

    print(
        f"Mapping coverage: "
        f"{summary['mapping_coverage_percent']}%"
    )

    print("\nReports generated in:", REPORTS_DIR)


if __name__ == "__main__":
    main()
