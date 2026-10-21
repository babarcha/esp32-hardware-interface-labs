
# BMP280 HIL — Test Strategy and Requirements Verification

## 1. Purpose

This document defines the verification strategy for the
ESP32 BMP280 I2C Hardware-in-the-Loop (HIL) project.

It describes:

- Verification objectives and scope
- Simulation and physical HIL testing
- Verification infrastructure unit tests
- Requirements traceability
- JUnit XML execution evidence
- Requirement status evaluation
- Quality gates and acceptance criteria
- Continuous integration responsibilities
- Known verification limitations

The strategy separates evidence of software correctness
from evidence of real hardware interaction.

## 2. System Under Test

The system under test (SUT) includes:

- ESP32 firmware built using ESP-IDF
- ESP32 I2C master peripheral
- I2C bus abstraction
- Physical BMP280 sensor
- Host-facing serial command interface

The host-side Python test environment communicates with
the ESP32 through USB serial using PySerial.

The reference hardware configuration uses:

- I2C SDA: GPIO21
- I2C SCL: GPIO22
- BMP280 address: 0x76
- BMP280 chip ID: 0x58
- Serial baud rate: 115200

The SUT boundary is important: host-side verification
tools support testing but are not part of the firmware
being verified.

## 3. Verification Objectives

The verification process aims to establish that:

1. The host can communicate with the ESP32.
2. The firmware reports sensor readiness.
3. The BMP280 can be identified.
4. Sensor measurements are returned and parsed.
5. Invalid commands are handled predictably.
6. Recovery operations behave as specified.
7. Repeated transactions remain operational.
8. Host-side protocol parsing handles errors.
9. Requirements are mapped to executable tests.
10. Verification results can be evaluated automatically.

Passing these tests demonstrates the exercised behaviors
under the recorded test conditions.

It does not establish exhaustive correctness.

## 4. Verification Levels

### 4.1 Simulation Tests

Simulation tests exercise host-side behavior without
connecting to the physical ESP32.

They include protocol parsing and error handling.

Examples include:

- Serial response parsing
- Ignoring non-protocol boot messages
- Communication timeout handling
- Empty response handling
- Invalid measurement handling
- Sensor error response handling
- Recovery failure response handling

Simulation tests are marked:

    @pytest.mark.simulation

Reference baseline:

    8 tests passed

### 4.2 Physical Hardware-in-the-Loop Tests

Physical HIL tests execute against a real ESP32 and
BMP280 sensor.

They cover:

- Connectivity and readiness
- Temperature and pressure measurements
- Sensor identity and address
- Command protocol behavior
- Sensor recovery
- Repeated communication
- Invalid input and robustness

Reference baseline:

    20 tests passed

These tests provide evidence of actual firmware,
peripheral, and sensor interaction.

They do not independently validate electrical timing,
signal integrity, or environmental calibration.

### 4.3 Verification Infrastructure Unit Tests

These tests verify the tools used to evaluate test
results and requirements.

They exercise:

- JUnit XML parsing
- Malformed XML handling
- Duplicate execution detection
- Conflicting execution results
- Requirement status evaluation
- Requirements mapping quality gates
- Test reference validity
- Optional verification evidence gate

Reference baseline:

    40 tests passed

Testing the verification infrastructure reduces the
risk of incorrect verification reports.

However, successful unit tests do not prove that
every possible reporting defect has been eliminated.

## 5. Test Classification

The HIL suite uses pytest markers:

    simulation
    hardware
    requirement(id)

The conftest.py collection hook automatically marks
unmarked HIL tests as hardware tests.

Tests explicitly marked as simulation are not
automatically assigned the hardware marker.

This convention requires discipline: any new test
that must run without hardware should be explicitly
marked as simulation.

Verify hardware test collection:

    python -m pytest -c pytest.ini --collect-only -q -m hardware tests

Expected reference count:

    20 selected

Verify simulation test collection:

    python -m pytest -c pytest.ini --collect-only -q -m simulation tests

Expected reference count:

    8 selected

## 6. Requirements Traceability

The requirements source is:

    requirements/bmp280_requirements.csv

Tests declare their requirements using pytest markers.

Example:

    @pytest.mark.requirement("REQ-HIL-001")
    def test_ping(device):
        assert device.ping() is True

The traceability generator reads the requirements CSV
and collects pytest requirement markers.

It produces:

    reports/requirements-traceability.csv
    reports/requirements-coverage.json

The generator rejects:

- Duplicate requirement IDs in the source catalog
- Empty requirement IDs
- Invalid requirement marker arguments
- References to unknown requirement IDs

Requirements without mapped tests are identified as
UNMAPPED in the generated reports.

## 7. Mapping Coverage

Mapping coverage is calculated as:

    Mapping coverage =
        (Mapped requirements / Total requirements) * 100

Reference baseline:

    Total requirements: 10
    Mapped requirements: 10
    Unmapped requirements: 0
    Mapping coverage: 100%

Mapping coverage does not establish that the tests
were executed or passed.

It establishes only that each requirement has at
least one associated test.

## 8. Execution Evidence

Test execution produces JUnit XML reports.

The evidence generator reads:

    reports/simulation-results.xml
    reports/hardware-results.xml

It combines execution results with the requirements
traceability matrix.

JUnit test identifiers are normalized to pytest
node IDs, such as:

    tests/test_connectivity.py::test_ping

The generator detects:

- Passing executions
- Failed executions
- Skipped executions
- Missing execution records
- Duplicate execution records
- Malformed JUnit XML
- Unsupported XML root elements
- Missing testcase identifiers

The evidence generator produces:

    reports/requirements-verification.csv
    reports/verification-evidence.json

## 9. Requirement Verification Status

A requirement can receive one of five statuses.

### PASSED

All mapped tests have exactly one passing
execution record.

### FAILED

At least one mapped test has a failed execution.

A failure takes precedence over other statuses.

### INCOMPLETE

Execution evidence exists but is insufficient
for a passing result.

Examples:

- A mapped test was skipped.
- Some mapped tests executed but others did not.
- A mapped test has duplicate execution records.

### NOT_EXECUTED

None of the mapped tests has an execution record.

### UNMAPPED

The requirement has no associated tests.

## 10. Verification Coverage

Verification coverage is calculated as:

    Verification coverage =
        (PASSED requirements / Total requirements) * 100

Reference baseline:

    Total requirements: 10
    Verified requirements: 10
    Verification coverage: 100%

Reference status counts:

    PASSED: 10
    FAILED: 0
    INCOMPLETE: 0
    NOT_EXECUTED: 0
    UNMAPPED: 0

This is different from test pass percentage.

One requirement may have multiple mapped tests,
and one test may contribute evidence to multiple
requirements.

## 11. Quality Gates

### G1 — Requirements Mapping

Checks that every requirement has at least one
mapped test.

Also detects duplicate requirement IDs in the
traceability report.

### G2 — Test Reference Validity

Checks that every mapped test ID exists in the
pytest collection inventory.

A requirement referencing a nonexistent test
must fail G2.

### G3 — Verification Evidence

Checks that every requirement in the verification
report has PASSED status.

G3 is enabled with:

    --require-verification

The current implementation checks the statuses
in the verification CSV.

It does not independently validate report freshness
or ensure that the verification report contains
exactly the same requirement IDs as the source catalog.

Therefore, G3 should only be used with freshly
generated and manually confirmed evidence.

## 12. Local Verification Procedure

Run commands from:

    i2c/lab02_i2c_master/hil

### Step 1: Simulation

    python -m pytest -c pytest.ini -v -m simulation tests --junitxml=reports/simulation-results.xml

### Step 2: Verification-tool unit tests

    python -m pytest -c pytest.ini -v tools/tests --junitxml=reports/verification-tools-results.xml

### Step 3: Physical HIL

Connect the ESP32/BMP280 and close the serial monitor.

    python -m pytest -c pytest.ini -v -m hardware tests --port COM3 --junitxml=reports/hardware-results.xml

### Step 4: Traceability

    python tools/generate_traceability.py

### Step 5: Verification evidence

    python tools/generate_verification_evidence.py

### Step 6: Quality gates

    python tools/check_quality_gates.py --require-verification

All steps must complete successfully before claiming
the local verification baseline is reproduced.

## 13. Continuous Integration Strategy

GitHub Actions runs two jobs.

### Python Simulation and Unit Tests

Executes:

- Python syntax validation
- Simulation tests
- Verification-tool unit tests

### Requirements Quality Gates

Executes:

- Requirements traceability generation
- G1 mapping gate
- G2 test reference validity gate

GitHub-hosted runners do not have access to
the physical ESP32/BMP280 setup.

Therefore:

- Hardware tests do not run in hosted CI.
- G3 is not enforced in hosted CI.
- Physical HIL evidence is generated locally.

Both jobs completed successfully in the
Pass 9.11 baseline.

## 14. Failure Handling

A verification failure should be investigated
before regenerating a final evidence package.

Recommended diagnostic order:

1. Check the failing pytest assertion.
2. Determine whether the failure is in firmware,
   physical hardware, host protocol, or test code.
3. Reproduce the failure with a focused test.
4. Review serial communication and sensor responses.
5. Correct the underlying problem.
6. Rerun the relevant tests.
7. Execute the complete regression suite.
8. Regenerate verification evidence.

Do not manually edit JUnit XML to change results.

Do not treat missing hardware evidence as passed.

## 15. Release Acceptance Criteria

The Pass 9.11 reference baseline is:

| Verification item | Expected |
|---|---:|
| Simulation tests | 8 passed |
| Hardware HIL tests | 20 passed |
| Verification-tool unit tests | 40 passed |
| Requirements mapped | 10/10 |
| Requirements PASSED | 10/10 |
| G1 | PASS |
| G2 | PASS |
| G3 | PASS |
| GitHub Actions jobs | 2 successful |

These counts describe the current baseline.

If requirements or tests change, the baseline
must be updated after successful re-verification.

## 16. Verification Limitations

The current suite does not independently establish:

- Electrical I2C timing compliance
- Signal integrity
- BMP280 calibration accuracy
- Long-duration reliability
- Operation over a wide temperature range
- Safety certification
- Test report freshness
- Firmware binary provenance

These areas require additional methods or
instrumentation beyond the present test suite.

## 17. Future Improvements

Potential improvements include:

- Hardware-connected GitHub Actions runner
- Automated evidence freshness validation
- Firmware commit and binary hash recording
- Complete requirement-set consistency checking
- Logic analyzer integration
- Controlled hardware fault injection
- Long-duration soak testing
- Versioned release evidence packages

## 18. Summary

The BMP280 HIL project combines:

- Embedded firmware testing
- Host-side simulation
- Physical sensor verification
- Verification-tool unit testing
- Requirements traceability
- Machine-readable execution evidence
- Automated quality gates
- Continuous integration

The principal engineering benefit is the separation
of test execution, evidence evaluation, and
requirements acceptance.

This provides a repeatable verification workflow
that can be extended to other embedded peripherals.
