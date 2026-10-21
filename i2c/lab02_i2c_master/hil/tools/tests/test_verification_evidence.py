
"""
Pass 9.11.2 — Verification Evidence Unit Tests.

Validate production requirement-status logic
without requiring physical ESP32 hardware.
"""

import pytest

from tools.generate_verification_evidence import requirement_status


pytestmark = pytest.mark.simulation


def make_result(status):
    """Create a synthetic execution result."""
    return {
        "status": status,
        "source": "synthetic-results.xml",
    }


def test_unmapped_requirement():
    assert requirement_status([], {}) == "UNMAPPED"


def test_requirement_not_executed():
    tests = ["test_a"]

    assert requirement_status(tests, {}) == "NOT_EXECUTED"


def test_requirement_passed():
    tests = ["test_a"]

    results = {
        "test_a": [make_result("PASSED")]
    }

    assert requirement_status(tests, results) == "PASSED"


def test_requirement_failed():
    tests = ["test_a"]

    results = {
        "test_a": [make_result("FAILED")]
    }

    assert requirement_status(tests, results) == "FAILED"


def test_requirement_skipped():
    tests = ["test_a"]

    results = {
        "test_a": [make_result("SKIPPED")]
    }

    assert requirement_status(tests, results) == "INCOMPLETE"


def test_multiple_tests_all_passed():
    tests = ["test_a", "test_b"]

    results = {
        "test_a": [make_result("PASSED")],
        "test_b": [make_result("PASSED")],
    }

    assert requirement_status(tests, results) == "PASSED"


def test_multiple_tests_one_failed():
    tests = ["test_a", "test_b"]

    results = {
        "test_a": [make_result("PASSED")],
        "test_b": [make_result("FAILED")],
    }

    assert requirement_status(tests, results) == "FAILED"


def test_multiple_tests_one_missing():
    tests = ["test_a", "test_b"]

    results = {
        "test_a": [make_result("PASSED")]
    }

    assert requirement_status(tests, results) == "INCOMPLETE"


def test_duplicate_passing_executions():
    """
    Duplicate passing executions are ambiguous
    and must not verify a requirement.
    """

    tests = ["test_a"]

    results = {
        "test_a": [
            make_result("PASSED"),
            make_result("PASSED"),
        ]
    }

    assert requirement_status(tests, results) == "INCOMPLETE"


def test_conflicting_execution_results():
    """
    A failed execution takes precedence over a pass.
    """

    tests = ["test_a"]

    results = {
        "test_a": [
            make_result("PASSED"),
            make_result("FAILED"),
        ]
    }

    assert requirement_status(tests, results) == "FAILED"
