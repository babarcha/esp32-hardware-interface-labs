
"""
Pass 9.11.3 — Requirements Quality Gate Unit Tests.

Tests G1, G2, and G3 without ESP32 hardware.
"""

import pytest

from tools.check_quality_gates import (
    evaluate_mapping,
    evaluate_test_references,
    evaluate_verification,
    evaluate_quality_gates,
)


pytestmark = pytest.mark.simulation


TEST_ID = "tests/test_connectivity.py::test_ping"


def make_requirement(
    requirement_id="REQ-HIL-001",
    tests=TEST_ID,
):
    return {
        "requirement_id": requirement_id,
        "description": "Device responds to PING",
        "category": "Connectivity",
        "tests": tests,
    }


def make_verification(status="PASSED"):
    return {
        "requirement_id": "REQ-HIL-001",
        "verification_status": status,
    }


def test_g1_complete_mapping():
    rows = [make_requirement()]

    assert evaluate_mapping(rows) == []


def test_g1_unmapped_requirement():
    rows = [make_requirement(tests="")]

    failures = evaluate_mapping(rows)

    assert len(failures) == 1
    assert "Unmapped" in failures[0]


def test_g1_duplicate_requirement():
    rows = [
        make_requirement(),
        make_requirement(),
    ]

    failures = evaluate_mapping(rows)

    assert any("Duplicate" in item for item in failures)


def test_g1_empty_report():
    failures = evaluate_mapping([])

    assert failures


def test_g2_valid_test_reference():
    rows = [make_requirement()]

    failures = evaluate_test_references(
        rows,
        {TEST_ID},
    )

    assert failures == []


def test_g2_invalid_test_reference():
    rows = [
        make_requirement(
            tests="tests/test_missing.py::test_unknown"
        )
    ]

    failures = evaluate_test_references(
        rows,
        {TEST_ID},
    )

    assert len(failures) == 1
    assert "unknown test" in failures[0]


def test_g3_passed_verification():
    rows = [make_verification()]

    assert evaluate_verification(rows) == []


def test_g3_failed_verification():
    rows = [make_verification("FAILED")]

    failures = evaluate_verification(rows)

    assert len(failures) == 1


def test_g3_incomplete_verification():
    rows = [make_verification("INCOMPLETE")]

    failures = evaluate_verification(rows)

    assert len(failures) == 1


def test_g3_empty_report():
    assert evaluate_verification([])


def test_all_mapping_gates_pass():
    summary = evaluate_quality_gates(
        [make_requirement()],
        {TEST_ID},
    )

    assert summary["passed"] is True
    assert len(summary["gates"]) == 2


def test_mapping_gate_failure():
    summary = evaluate_quality_gates(
        [make_requirement(tests="")],
        {TEST_ID},
    )

    assert summary["passed"] is False


def test_optional_verification_gate_passes():
    summary = evaluate_quality_gates(
        [make_requirement()],
        {TEST_ID},
        [make_verification("PASSED")],
    )

    assert summary["passed"] is True
    assert len(summary["gates"]) == 3


def test_optional_verification_gate_fails():
    summary = evaluate_quality_gates(
        [make_requirement()],
        {TEST_ID},
        [make_verification("FAILED")],
    )

    assert summary["passed"] is False
