
"""
Pass 9.11.2 — Production JUnit Parser Unit Tests.

Validate the real JUnit XML parser used by
generate_verification_evidence.py.

No physical hardware is required.
"""

import pytest

from tools.generate_verification_evidence import (
    normalize_test_id,
    parse_junit_file,
    requirement_status,
)


pytestmark = pytest.mark.simulation


def parse_xml(tmp_path, xml_text):
    """Write synthetic JUnit XML and parse it."""

    junit_file = tmp_path / "synthetic-results.xml"
    junit_file.write_text(xml_text, encoding="utf-8")

    return parse_junit_file(junit_file)


def test_parse_passing_test(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature"/>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    test_id = "tests/test_sensor.py::test_temperature"

    assert results[test_id][0]["status"] == "PASSED"


def test_parse_failed_test(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature">
            <failure message="Out of range"/>
        </testcase>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    test_id = "tests/test_sensor.py::test_temperature"

    assert results[test_id][0]["status"] == "FAILED"


def test_parse_error_result(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature">
            <error message="Serial connection failed"/>
        </testcase>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    test_id = "tests/test_sensor.py::test_temperature"

    assert results[test_id][0]["status"] == "FAILED"


def test_parse_skipped_test(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature">
            <skipped message="Hardware unavailable"/>
        </testcase>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    test_id = "tests/test_sensor.py::test_temperature"

    assert results[test_id][0]["status"] == "SKIPPED"


def test_parse_multiple_tests(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature"/>
        <testcase classname="tests.test_sensor"
                  name="test_pressure"/>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    assert len(results) == 2


def test_detect_duplicate_executions(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature"/>
        <testcase classname="tests.test_sensor"
                  name="test_temperature"/>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    test_id = "tests/test_sensor.py::test_temperature"

    assert len(results[test_id]) == 2

    assert requirement_status(
        [test_id], results
    ) == "INCOMPLETE"


def test_detect_conflicting_results(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature"/>
        <testcase classname="tests.test_sensor"
                  name="test_temperature">
            <failure message="Measurement failed"/>
        </testcase>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    test_id = "tests/test_sensor.py::test_temperature"

    assert requirement_status(
        [test_id], results
    ) == "FAILED"


def test_reject_malformed_xml(tmp_path):
    xml = "<testsuite><testcase></testsuite>"

    with pytest.raises(ValueError, match="Malformed"):
        parse_xml(tmp_path, xml)


def test_reject_unsupported_xml(tmp_path):
    xml = """
    <report>
        <testcase classname="tests.test_sensor"
                  name="test_temperature"/>
    </report>
    """

    with pytest.raises(ValueError, match="Unsupported"):
        parse_xml(tmp_path, xml)


def test_reject_missing_testcase_name(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"/>
    </testsuite>
    """

    with pytest.raises(ValueError, match="missing"):
        parse_xml(tmp_path, xml)


def test_nested_testsuites(tmp_path):
    xml = """
    <testsuites>
        <testsuite name="simulation">
            <testcase classname="tests.test_sim"
                      name="test_simulation"/>
        </testsuite>
        <testsuite name="hardware">
            <testcase classname="tests.test_hw"
                      name="test_hardware"/>
        </testsuite>
    </testsuites>
    """

    results = parse_xml(tmp_path, xml)

    assert len(results) == 2


def test_missing_execution_evidence():
    test_id = "tests/test_sensor.py::test_temperature"

    assert requirement_status(
        [test_id], {}
    ) == "NOT_EXECUTED"


def test_normalize_test_id():
    test_id = normalize_test_id(
        "tests.test_connectivity",
        "test_ping",
    )

    assert test_id == "tests/test_connectivity.py::test_ping"


def test_reject_missing_classname(tmp_path):
    xml = """
    <testsuite>
        <testcase name="test_temperature"/>
    </testsuite>
    """

    with pytest.raises(ValueError, match="missing"):
        parse_xml(tmp_path, xml)


def test_empty_testsuite(tmp_path):
    xml = "<testsuite name='empty'/>"

    results = parse_xml(tmp_path, xml)

    assert len(results) == 0


def test_execution_source_recorded(tmp_path):
    xml = """
    <testsuite>
        <testcase classname="tests.test_sensor"
                  name="test_temperature"/>
    </testsuite>
    """

    results = parse_xml(tmp_path, xml)

    test_id = "tests/test_sensor.py::test_temperature"

    assert (
        results[test_id][0]["source"]
        == "synthetic-results.xml"
    )
