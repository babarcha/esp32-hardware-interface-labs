
# BMP280 HIL Verification — System Architecture

## 1. Purpose

This project demonstrates end-to-end verification of an
ESP32-based BMP280 environmental sensor interface.

It combines embedded firmware, physical I2C communication,
host-side Python testing, requirements traceability,
verification evidence, and automated CI quality gates.

The objective is to distinguish three types of evidence:

1. Software behavior verified without physical hardware.
2. Embedded behavior verified against a physical device.
3. Requirements verified through traceable test results.

## 2. System Context

The system under test (SUT) consists of an ESP32
running ESP-IDF firmware and communicating with a
BMP280 sensor through I2C.

A Windows host communicates with the ESP32 through
a USB serial connection.

```text
Windows 11 Host
|
+-- Python / pytest
|   |
|   +-- Simulation Tests
|   |
|   +-- Verification Tool Unit Tests
|   |
|   +-- Hardware HIL Tests
|       |
|       +-- PySerial
|           |
|           +-- USB / UART (COM3)
|               |
|               v
|          ESP32 Firmware
|               |
|               +-- Command Handling
|               |
|               +-- I2C Bus Abstraction
|                   |
|                   +-- ESP-IDF I2C Driver
|                       |
|                       +-- GPIO21 (SDA)
|                       +-- GPIO22 (SCL)
|                           |
|                           v
|                       BMP280 Sensor
```

## 3. Software Responsibilities

### Embedded firmware

The firmware is responsible for:

- Configuring the ESP32 I2C master.
- Communicating with the BMP280.
- Handling host commands.
- Reporting sensor identity and measurements.
- Responding to invalid requests.
- Supporting recovery and repeated transactions.

### Python HIL test suite

The host-side tests are responsible for:

- Exercising the firmware command interface.
- Checking sensor responses.
- Validating measurement ranges.
- Checking communication robustness.
- Testing recovery behavior.
- Recording execution results.

### Simulation tests

Simulation tests verify host-side parsing, protocol
handling, and error behavior without requiring an ESP32.

### Verification tools

The verification tools:

- Collect requirements-to-test mappings.
- Generate traceability reports.
- Parse JUnit XML execution evidence.
- Evaluate requirement verification status.
- Apply quality gates.

## 4. Verification Architecture

```text
Requirements
    |
    v
Test-to-Requirement Mapping
    |
    v
Traceability Generator
    |
    +----------------------+
    |                      |
    v                      v
Simulation Tests      Hardware HIL Tests
    |                      |
    v                      v
JUnit XML              JUnit XML
    |                      |
    +-----------+----------+
                |
                v
      Evidence Generator
                |
                v
      Verification Reports
                |
                v
          Quality Gates
```

## 5. Test Classification

The HIL test suite uses pytest markers.

- `simulation`: tests that do not require physical hardware.
- `hardware`: tests that communicate with the real ESP32.

The `conftest.py` collection hook classifies unmarked
HIL tests as hardware tests.

This convention is restricted to the HIL test suite.
New simulation tests must be explicitly marked.

## 6. Requirements Quality Gates

### G1 — Requirements Mapping

Every requirement must have at least one mapped test.
Duplicate requirement IDs are rejected.

### G2 — Test Reference Validity

Every mapped test ID must exist in the collected
pytest test inventory.

### G3 — Verification Evidence

Every requirement in the verification report must
have PASSED status.

G3 is enabled explicitly for local release validation
after generating execution evidence.

Current G3 implementation does not independently
validate evidence freshness or the complete equality
of requirement ID sets across all source reports.

## 7. Continuous Integration

GitHub Actions runs two jobs:

1. Python Simulation and Unit Tests.
2. Requirements Quality Gates.

GitHub-hosted runners do not have access to the
physical ESP32/BMP280 hardware.

Therefore, physical HIL testing is executed locally,
while simulation, verification-tool unit tests, and
G1/G2 checks run in CI.

## 8. Verified Baseline

Final Pass 9.11 local results:

| Test Category | Passed |
|---|---:|
| Simulation | 8 |
| Hardware HIL | 20 |
| Verification infrastructure | 40 |
| Total | 68 |

Requirements:
- Total: 10
- Mapped: 10
- Verified PASSED: 10

Local quality gates:
- G1: PASS
- G2: PASS
- G3: PASS

The GitHub Actions workflow completed successfully
with both jobs passing.

These results describe the verified Pass 9.11 baseline;
future code changes require regression re-execution.

## 9. Architectural Decisions

### Separate host tests from embedded firmware

This enables independent test development and
execution without rebuilding firmware for every
test case.

### Use pytest markers

Markers allow the same repository to support
hardware-independent CI and physical HIL testing.

### Use JUnit XML as execution evidence

JUnit provides machine-readable results that can
be processed independently of console output.

### Keep generated reports outside Git

Reports are regenerated for each verification run.
CI uploads reports as workflow artifacts.

### Separate G1/G2 from G3

Requirements mapping can be checked without hardware.

Complete verification requires real hardware execution
and must not be inferred from simulation-only CI.

## 10. Known Limitations

- Physical HIL execution currently requires a
  locally connected ESP32/BMP280 setup.
- COM3 is the current Windows port, not a universal
  device identifier.
- The HIL setup does not independently measure
  electrical bus timing or signal integrity.
- Verification evidence freshness requires manual
  confirmation.
- No GitHub-hosted hardware runner is configured.

## 11. Future Improvements

Potential future extensions include:

- Dedicated hardware CI runner.
- Logic-analyzer-based I2C verification.
- Firmware build identification in evidence reports.
- Automated report freshness checks.
- Stronger requirement-set consistency validation.
- Hardware fault-injection testing.
```

### Step C — Update the root README

Find the I²C entry in the root lab status table and replace its outdated status with:

```markdown
| 02 | I²C | Master bus, scanning, BMP280 integration, physical HIL, requirements verification | BMP280 hardware HIL validated; 20/20 hardware tests passed |
```

In the Lab 02 section, replace the statement that physical peripheral validation remains a future extension with:

```markdown
### BMP280 Physical Hardware Verification

The I²C lab has been extended with a physical BMP280 sensor
and a Python-based Hardware-in-the-Loop verification suite.

The completed Pass 9.11 baseline includes:

- 20 passing physical hardware tests.
- 8 passing hardware-independent simulation tests.
- 40 passing verification-tool unit tests.
- 10/10 requirements mapped and verified.
- G1, G2, and G3 quality gates passing locally.
- Two successful GitHub Actions CI jobs.

The HIL implementation and documentation are located in:

`i2c/lab02_i2c_master/hil/`

Other I²C peripherals have not been independently validated
by this BMP280 test suite.
```

This distinction is important: successful BMP280 validation does not establish that every possible I²C peripheral has been tested.

## 4. Verify and commit Pass 9.12.1

Run:

```powershell
cd C:\Users\babar\esp32-hardware-interface-labs

git status --short
git diff --check

git add README.md
git add i2c/lab02_i2c_master/hil/docs/ARCHITECTURE.md

git diff --cached --check
git diff --cached --stat
```

If the staged changes are correct:

```powershell
git commit -m "Document BMP280 HIL architecture and verified baseline"

git push origin main
```

## 5. Next documentation pass

**Pass 9.12.2 — Hardware Setup and Reproducibility** will document the ESP32-to-BMP280 breadboard wiring, sensor identification, serial communication, firmware flashing, Python environment setup, and troubleshooting.

For the v1.0 release, we'll also verify the documentation against the actual repository files and run the final regression once more before creating the Git tag.