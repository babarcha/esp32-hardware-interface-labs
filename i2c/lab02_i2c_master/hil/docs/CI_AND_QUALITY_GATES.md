
# BMP280 HIL — CI/CD and Quality Gates

## 1. Purpose

This document describes the Continuous Integration (CI)
workflow for the ESP32 BMP280 Hardware-in-the-Loop (HIL)
verification project.

The CI workflow provides automated feedback on:

- Python syntax correctness
- Hardware-independent simulation tests
- Verification infrastructure unit tests
- Requirements-to-test mapping
- Test reference validity
- Machine-readable verification reports

Physical HIL testing is performed separately because
GitHub-hosted runners cannot access the local ESP32
and BMP280 sensor.

## 2. CI Technology Stack

| Component | Technology |
|---|---|
| Source control | Git and GitHub |
| CI platform | GitHub Actions |
| Runner | Ubuntu 24.04 |
| CI Python | Python 3.11 |
| Test framework | pytest |
| Serial library | PySerial |
| HTML reports | pytest-html |
| Machine-readable results | JUnit XML |
| Quality gates | Custom Python scripts |
| Artifacts | GitHub Actions artifacts |

The local reference environment uses Windows 11
and Python 3.14.7.

## 3. Workflow Location

The workflow definition is stored at:

    .github/workflows/bmp280-hil-ci.yml

Workflow name:

    BMP280 Python HIL CI

The workflow defines two independent jobs:

1. Python Simulation and Unit Tests
2. Requirements Quality Gates

Both jobs execute on GitHub-hosted Ubuntu runners.

## 4. Workflow Triggers

### Push

The workflow runs on pushes to:

- main
- feature/**

For push events, relevant file changes must match
the configured path filters.

### Pull Request

The workflow runs for pull requests when relevant
files are changed.

### Manual Execution

The workflow supports:

    workflow_dispatch

This allows a developer to start a run manually
from the GitHub Actions interface.

### Path Filters

The configured paths are:

    i2c/lab02_i2c_master/hil/**
    .github/workflows/bmp280-hil-ci.yml

Changes outside these paths do not automatically
trigger this particular workflow.

For example, a change only to the repository-root
README.md does not match these path filters.

## 5. Workflow Permissions

The workflow declares:

    permissions:
      contents: read

This grants the workflow read access to repository
contents without requesting unnecessary write access.

The jobs do not need to modify repository files
on GitHub.

## 6. Job 1 — Python Simulation and Unit Tests

Job identifier:

    python-tests

Display name:

    Python Simulation and Unit Tests

### Step 1: Checkout

The workflow retrieves the repository using:

    actions/checkout@v4

### Step 2: Python Setup

The runner installs Python 3.11 using:

    actions/setup-python@v5

### Step 3: Install Dependencies

The workflow installs:

    pytest
    pytest-html
    pyserial

### Step 4: Python Syntax Validation

Command:

    python -m compileall -q tools tests

This checks that Python source files in the selected
directories can be compiled.

It is not a static-analysis or linting replacement.

### Step 5: Simulation Tests

Command:

    python -m pytest -c pytest.ini -v -m simulation tests

The actual CI command also generates JUnit XML
and a self-contained HTML report.

Reference baseline:

    8 simulation tests passed

### Step 6: Verification Infrastructure Tests

Command:

    python -m pytest -c pytest.ini -v tools/tests

Reference baseline:

    40 unit tests passed

These tests verify the evidence-processing and
quality-gate implementation.

### Step 7: Upload Test Reports

The workflow uploads:

    bmp280-python-test-reports

The artifact contains:

    ci-simulation-results.xml
    ci-simulation-report.html
    ci-tools-results.xml
    ci-tools-report.html

Artifacts are retained for 14 days.

The upload step uses:

    if: always()

This means the upload step is attempted even if
an earlier test step fails.

It does not guarantee that every expected report
exists or that artifact upload itself succeeds.

## 7. Job 2 — Requirements Quality Gates

Job identifier:

    requirements-quality

Display name:

    Requirements Quality Gates

### Step 1: Checkout

The workflow retrieves the repository.

### Step 2: Python Setup

The runner installs Python 3.11.

### Step 3: Dependencies

The workflow installs:

    pytest
    pyserial

### Step 4: Traceability Generation

Command:

    python tools/generate_traceability.py

The generator reads:

    requirements/bmp280_requirements.csv

It collects requirement markers from the pytest
test inventory.

Generated reports:

    requirements-traceability.csv
    requirements-coverage.json

### Step 5: Execute Quality Gates

Command:

    python tools/check_quality_gates.py

This runs G1 and G2.

The command does not enable G3.

### Step 6: Display Gate Results

The workflow prints:

    reports/quality-gates.json

if that file exists.

This makes the gate results visible in the
GitHub Actions job log.

### Step 7: Upload Requirements Reports

Artifact name:

    bmp280-requirements-quality-reports

Files:

    requirements-traceability.csv
    requirements-coverage.json
    quality-gates.json

Retention period:

    14 days

## 8. Independent Job Execution

The two jobs do not declare a dependency using
the GitHub Actions needs keyword.

Therefore, GitHub Actions can schedule them
independently and potentially run them in parallel.

Each job has its own runner and filesystem.

The requirements-quality job regenerates its
own traceability report.

It does not consume the test-report artifact
from the python-tests job.

This reduces coupling between the jobs.

## 9. Quality Gate G1 — Requirements Mapping

Purpose:

Ensure that every requirement in the traceability
report has at least one mapped test.

The gate also rejects:

- Empty traceability reports
- Empty requirement IDs
- Duplicate requirement IDs
- Requirements without mapped tests

A requirement without a mapped test produces
a G1 failure.

Reference baseline:

    G1_requirements_mapping: PASS

## 10. Quality Gate G2 — Test Reference Validity

Purpose:

Ensure that each mapped test actually exists
in the collected pytest test inventory.

The quality-gate script collects tests using:

    pytest --collect-only -q

It then compares the collected pytest node IDs
against the test IDs in the traceability matrix.

A nonexistent test reference produces a G2 failure.

Reference baseline:

    G2_test_reference_validity: PASS

G2 checks the existence of mapped tests.

It does not prove that those tests passed.

## 11. Quality Gate G3 — Verification Evidence

Purpose:

Ensure that every requirement in the verification
report has PASSED status.

G3 is optional and is not executed by the
GitHub-hosted workflow.

Enable G3 locally using:

    python tools/check_quality_gates.py --require-verification

The script reads:

    reports/requirements-verification.csv

It rejects requirements whose verification_status
is not PASSED.

Reference local baseline:

    G3_verification_evidence: PASS

### Current Limitations

The current G3 implementation does not independently
establish that:

- The verification report is fresh.
- The report matches the current firmware revision.
- The verification report contains exactly the same
  requirement IDs as the source requirements catalog.
- All evidence files originate from one recorded
  release-verification session.

These limitations must be considered when interpreting
a passing G3 result.

## 12. Difference Between G1, G2, and G3

| Gate | Main Question |
|---|---|
| G1 | Does every requirement have a mapped test? |
| G2 | Does every mapped test actually exist? |
| G3 | Does every reported requirement have PASSED status? |

These checks address different risks.

A requirement may be correctly mapped and reference
a real test, while that test still fails on hardware.

Therefore:

    G1 PASS + G2 PASS

does not imply:

    G3 PASS

Similarly, a G3 PASS must be interpreted in the
context of the evidence-generation limitations.

## 13. CI Failure Behavior

### Simulation Test Failure

If a simulation test fails, the corresponding
pytest command exits unsuccessfully.

The Python test job fails.

### Verification Tool Unit Test Failure

If an infrastructure unit test fails, the
Python test job fails.

### Requirements Mapping Failure

If G1 fails, the quality-gate script exits
with a nonzero status.

The requirements-quality job fails.

### Invalid Test Reference

If G2 detects a nonexistent test reference,
the quality-gate script exits unsuccessfully.

### Missing Traceability Input

If a required report is missing, the quality-gate
script records an error and exits unsuccessfully.

### Artifact Upload

Artifact-upload steps use:

    if: always()

This allows an upload attempt even when earlier
verification steps fail.

Missing files may still prevent complete artifact
collection.

## 14. Local Release Verification

The full local verification procedure includes
the physical hardware tests.

Run from:

    i2c/lab02_i2c_master/hil

### Simulation

    python -m pytest -c pytest.ini -v -m simulation tests --junitxml=reports/simulation-results.xml

### Verification Tool Unit Tests

    python -m pytest -c pytest.ini -v tools/tests --junitxml=reports/verification-tools-results.xml

### Physical HIL

    python -m pytest -c pytest.ini -v -m hardware tests --port COM3 --junitxml=reports/hardware-results.xml

### Traceability

    python tools/generate_traceability.py

### Verification Evidence

    python tools/generate_verification_evidence.py

### All Three Gates

    python tools/check_quality_gates.py --require-verification

The physical hardware regression must be executed
against the intended firmware build.

## 15. Baseline Verification Results

The verified Pass 9.11 baseline was:

| Check | Result |
|---|---|
| Simulation | 8/8 passed |
| Verification-tool unit tests | 40/40 passed |
| Physical HIL | 20/20 passed |
| Requirements mapped | 10/10 |
| Requirements PASSED | 10/10 |
| Local G1 | PASS |
| Local G2 | PASS |
| Local G3 | PASS |
| GitHub Actions | Both jobs successful |

These results describe a completed verification
session, not a permanent guarantee that all future
commits will pass.

## 16. CI Artifacts and Evidence

GitHub Actions artifacts make reports accessible
after workflow execution.

Developers can inspect:

- XML test results
- HTML test reports
- Requirements traceability
- Mapping coverage
- Quality-gate results

The workflow retains these artifacts for 14 days.

The artifacts are not a permanent evidence archive.

A formal release process should preserve relevant
verification evidence separately if longer-term
retention is required.

## 17. Security and Reliability Considerations

The workflow uses read-only repository permissions.

It does not require:

- Deployment credentials
- Cloud service secrets
- Physical hardware credentials
- Repository write access

Future improvements could include:

- Pinning GitHub Actions to immutable commit SHAs
- Locking Python dependency versions
- Adding static analysis and linting
- Adding dependency vulnerability scanning
- Using a dedicated hardware-connected CI runner
- Recording firmware binary hashes
- Recording evidence-generation timestamps
- Archiving release evidence with version metadata

## 18. Known CI Limitations

- GitHub-hosted CI does not execute physical HIL tests.
- G3 is not enforced in hosted CI.
- Python dependencies are not version-pinned.
- The workflow currently uses checkout@v4
  and setup-python@v5.
- GitHub has reported Node.js 20 deprecation warnings
  for the current Actions dependencies.
- Reports are retained for 14 days.
- The CI workflow does not build ESP32 firmware.
- There is no dedicated hardware CI runner.
- Electrical I2C compliance is not tested by CI.

These are improvement opportunities rather than
evidence that the current CI run failed.

## 19. Interview Explanation

The project uses a two-job GitHub Actions workflow.

The first job checks Python syntax and executes
hardware-independent simulation and verification-tool
unit tests.

The second job generates the requirements traceability
matrix and enforces two quality gates: all requirements
must be mapped, and all mapped test references must exist.

Physical HIL tests run locally against the ESP32 and
BMP280 because hosted CI runners do not have access
to the hardware.

After the physical tests, JUnit execution reports are
combined with requirement mappings to generate
verification evidence.

A third quality gate then checks that the reported
requirements have PASSED status.

This separates continuous software verification
from physical hardware verification while preserving
a consistent requirements-based acceptance workflow.

## 20. Summary

The BMP280 HIL CI pipeline provides:

- Repeatable hardware-independent testing
- Automated requirements mapping validation
- Early detection of invalid test references
- Machine-readable test reports
- Downloadable CI artifacts
- Explicit separation of local physical HIL testing

The principal architectural benefit is that
test execution, traceability, and release verification
are treated as related but distinct activities.
