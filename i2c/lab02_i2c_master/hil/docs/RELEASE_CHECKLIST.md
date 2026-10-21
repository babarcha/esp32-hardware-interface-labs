
# BMP280 HIL — v1.0 Release Checklist

## 1. Purpose

This document defines the release-readiness procedure
for the ESP32 BMP280 Hardware-in-the-Loop (HIL)
verification project.

A release is accepted only after:

- Source and documentation review
- Firmware build verification
- Hardware connectivity confirmation
- Simulation regression
- Verification-tool unit testing
- Physical HIL regression
- Requirements traceability generation
- Verification evidence evaluation
- Quality-gate acceptance
- GitHub Actions confirmation
- Git repository integrity checks

The target version is v1.0.0.

This version identifies the BMP280 HIL verification
baseline within the larger ESP32 hardware interface
labs repository.

It does not imply that every UART, SPI, or CAN lab
has completed equivalent physical verification.

## 2. Reference Environment

| Component | Reference |
|---|---|
| Operating system | Windows 11 |
| MCU | ESP32 |
| Sensor | BMP280 |
| ESP-IDF | 5.5.5 |
| Python | 3.14.7 |
| pytest | 9.1.1 |
| PySerial | 3.5 |
| pytest-html | 4.2.0 |
| Serial port | COM3 |
| Baud rate | 115200 |
| I2C SDA | GPIO21 |
| I2C SCL | GPIO22 |
| Sensor address | 0x76 |
| Sensor chip ID | 0x58 |

The CI environment uses Ubuntu 24.04 and Python 3.11.

## 3. Source Control Checks

Run from the repository root:

    git status
    git branch --show-current
    git log -1 --oneline
    git remote -v

Expected:

- Working on main for the final release.
- All intended changes committed.
- Correct GitHub remote.
- No uncommitted changes.

Check whitespace errors:

    git diff --check

Check staged changes:

    git diff --cached --check

Confirm the local branch is synchronized:

    git fetch origin
    git status -sb

Do not create a release tag from an unknown or
uncommitted source state.

## 4. Documentation Checks

Confirm the following documents exist:

    hil/docs/ARCHITECTURE.md
    hil/docs/HARDWARE_SETUP.md
    hil/docs/TEST_STRATEGY.md
    hil/docs/CI_AND_QUALITY_GATES.md
    hil/docs/RELEASE_CHECKLIST.md

Review the documentation for:

- Correct hardware wiring
- Correct firmware paths
- Correct command examples
- Accurate test counts
- Accurate CI limitations
- Valid internal links
- No obsolete validation-status claims

The repository root README and I2C lab README
should accurately describe the BMP280 extension.

## 5. Firmware Build Verification

Activate the ESP-IDF environment.

Navigate to:

    i2c/lab02_i2c_master

Run:

    idf.py build

Acceptance:

- Build completes successfully.
- No unresolved build errors.
- Firmware corresponds to the intended source revision.

Flash:

    idf.py -p COM3 flash

Monitor:

    idf.py -p COM3 monitor

Confirm:

- Firmware starts.
- I2C bus initializes.
- BMP280 is detected.
- Sensor communication is operational.

Exit monitor:

    Ctrl + ]

Close the monitor before running HIL tests.

## 6. Python Environment Verification

Activate the Python virtual environment:

    .\.venv-hil\Scripts\Activate.ps1

Verify:

    python --version
    python -m pytest --version
    python -m pip show pyserial pytest-html

Record the versions used for the release.

A reproducible release should also record
the complete dependency set.

Suggested command:

    python -m pip freeze > reports/pip-freeze.txt

Run this command from the HIL directory
after ensuring reports/ exists.

The generated dependency record should be
retained with the release evidence.

## 7. Physical Hardware Verification

Confirm:

- ESP32 connected to USB.
- BMP280 powered at a compatible voltage.
- Common ground connected.
- SDA connected to GPIO21.
- SCL connected to GPIO22.
- Correct sensor address configured.
- No serial monitor holding COM3.

List available serial ports:

    python -m serial.tools.list_ports

Expected reference port:

    COM3

The actual port may differ between machines.

## 8. Test Inventory Verification

Navigate to:

    i2c/lab02_i2c_master/hil

Run:

    python -m pytest -c pytest.ini --collect-only -q -m simulation tests

Reference baseline:

    8 selected

Run:

    python -m pytest -c pytest.ini --collect-only -q -m hardware tests

Reference baseline:

    20 selected

Unexpected changes to test counts must be
investigated before release approval.

## 9. Simulation Regression

Run:

    python -m pytest -c pytest.ini -v -m simulation tests --junitxml=reports/simulation-results.xml

Acceptance:

    8 passed

The report must correspond to the current run.

## 10. Verification Infrastructure Regression

Run:

    python -m pytest -c pytest.ini -v tools/tests --junitxml=reports/verification-tools-results.xml

Acceptance:

    40 passed

This validates the current evidence-processing
and quality-gate test baseline.

## 11. Physical HIL Regression

Run:

    python -m pytest -c pytest.ini -v -m hardware tests --port COM3 --junitxml=reports/hardware-results.xml

Acceptance:

    20 passed

The test run must use:

- The intended firmware build
- The connected ESP32
- The physical BMP280
- The correct serial port

Hardware failures must be investigated before
approving the release.

## 12. Requirements Traceability

Run:

    python tools/generate_traceability.py

Acceptance:

    Total requirements: 10
    Mapped requirements: 10
    Unmapped requirements: 0
    Mapping coverage: 100.0%

Generated files:

    reports/requirements-traceability.csv
    reports/requirements-coverage.json

## 13. Verification Evidence

Run:

    python tools/generate_verification_evidence.py

Acceptance:

    Total requirements: 10
    Verified requirements: 10
    Verification coverage: 100.0%

Status counts:

    PASSED: 10
    FAILED: 0
    INCOMPLETE: 0
    NOT_EXECUTED: 0
    UNMAPPED: 0

Generated files:

    reports/requirements-verification.csv
    reports/verification-evidence.json

## 14. Quality Gates

Run:

    python tools/check_quality_gates.py --require-verification

Acceptance:

    G1_requirements_mapping: PASS
    G2_test_reference_validity: PASS
    G3_verification_evidence: PASS

Important:

The current G3 gate checks the verification
statuses in the generated CSV.

It does not independently enforce evidence
freshness or complete requirement-set equality.

The release reviewer must therefore confirm
that the reports were regenerated from the
intended test execution session.

## 15. GitHub Actions Verification

Open the GitHub Actions page.

Locate:

    BMP280 Python HIL CI

Confirm:

- The intended commit triggered the workflow.
- Python Simulation and Unit Tests succeeded.
- Requirements Quality Gates succeeded.
- Reports were uploaded.
- No unexpected test failures occurred.

The workflow does not execute physical HIL
tests or the G3 release gate.

## 16. Release Evidence Package

The release evidence should contain:

- Source commit SHA
- Firmware build version
- Hardware configuration
- Python dependency versions
- Simulation JUnit XML
- Hardware JUnit XML
- Verification-tool JUnit XML
- Requirements traceability CSV
- Requirements coverage JSON
- Requirements verification CSV
- Verification evidence JSON
- Quality-gate results JSON
- GitHub Actions run URL
- Release approval date

The reports/ directory is ignored by Git.

Do not assume that local generated reports
are automatically published with the release.

If evidence must be retained, archive the
files separately.

## 17. Known Limitations

The v1.0 baseline does not establish:

- Full I2C electrical compliance
- Environmental calibration accuracy
- Long-duration reliability
- Hardware-independent execution of physical tests
- Automated evidence freshness enforcement
- Formal safety certification
- Physical SPI or CAN validation

These limitations should remain visible in
the release description.

## 18. Release Decision Checklist

### Documentation

- [ ] Architecture document reviewed
- [ ] Hardware setup guide reviewed
- [ ] Test strategy reviewed
- [ ] CI and quality-gates guide reviewed
- [ ] Release checklist reviewed
- [ ] Root README status updated
- [ ] I2C lab README status updated

### Firmware and Hardware

- [ ] Firmware build passed
- [ ] Firmware flashed successfully
- [ ] BMP280 detected
- [ ] Sensor chip ID verified
- [ ] Serial communication operational

### Testing

- [ ] 8 simulation tests passed
- [ ] 40 verification-tool unit tests passed
- [ ] 20 physical HIL tests passed

### Requirements

- [ ] 10/10 requirements mapped
- [ ] 10/10 requirements verified
- [ ] G1 passed
- [ ] G2 passed
- [ ] G3 passed
- [ ] Evidence freshness manually confirmed

### CI and Git

- [ ] Both GitHub Actions jobs passed
- [ ] Relevant CI artifacts available
- [ ] All changes committed
- [ ] Local branch synchronized with origin/main
- [ ] Working tree clean
- [ ] Release commit SHA recorded

### Release

- [ ] Release scope confirmed
- [ ] Known limitations documented
- [ ] Evidence package retained
- [ ] Release notes reviewed
- [ ] Version tag approved

## 19. Release Approval Record

Release version:

    v1.0.0

Release scope:

    BMP280 I2C HIL verification framework

Source commit SHA:

    TO BE RECORDED

Firmware build identifier:

    TO BE RECORDED

Verification date:

    TO BE RECORDED

CI run URL:

    TO BE RECORDED

Verification result:

    PENDING FINAL REGRESSION

Release approval:

    PENDING

## 20. Release Policy

The release tag must identify the exact
source revision that completed acceptance.

Do not create the tag before:

1. Documentation review.
2. Complete regression.
3. Requirements verification.
4. Quality-gate acceptance.
5. CI confirmation.
6. Release approval.

The tag and GitHub release are created in
Pass 9.12.6 after these checks are satisfied.
