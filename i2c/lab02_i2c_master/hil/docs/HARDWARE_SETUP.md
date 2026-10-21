
# BMP280 HIL — Hardware Setup and Reproducibility

## 1. Purpose

This guide explains how to reproduce the ESP32/BMP280
Hardware-in-the-Loop (HIL) verification environment.

It covers physical wiring, firmware preparation, Python
dependencies, serial communication, test execution,
verification evidence, and troubleshooting.

The validated reference environment is:

- Windows 11
- ESP32 development board
- BMP280 pressure/temperature sensor
- ESP-IDF v5.5.5
- Python 3.14.7
- pytest 9.1.1
- PySerial 3.5
- pytest-html 4.2.0
- USB serial port COM3
- UART baud rate 115200

Other compatible environments may work but have not
been established by this particular validation record.

## 2. Required Hardware

- ESP32 development board
- BMP280 sensor breakout module
- USB data cable
- Breadboard
- Male-to-male or male-to-female jumper wires
- Windows PC with an available USB port

Optional diagnostic equipment:

- Multimeter
- Logic analyzer
- Oscilloscope

The optional instruments are not required to execute
the current automated test suite.

## 3. Electrical Connections

The firmware uses:

- I2C master: I2C_NUM_0
- SDA: GPIO21
- SCL: GPIO22
- Sensor address: 0x76
- BMP280 chip ID: 0x58

### Wiring

| BMP280 | ESP32 | Function |
|---|---|---|
| VCC/VIN | 3V3 | Power |
| GND | GND | Ground |
| SDA/SDI | GPIO21 | I2C data |
| SCL/SCK | GPIO22 | I2C clock |
| CS/CSB (if exposed) | 3V3 | I2C mode |
| SDO (if exposed) | GND | Address 0x76 |

Important:

- Power off the board before changing wiring.
- Use a 3.3 V-compatible sensor breakout.
- Never assume a module tolerates 5 V.
- Check the breakout pin labels and datasheet.
- SDA and SCL require appropriate pull-up resistors.
- Many breakouts contain pull-ups already.
- Avoid unnecessary parallel pull-up resistors.
- The ESP32 and sensor must share a common ground.

### Logical Wiring Diagram

```text
ESP32                         BMP280
-----                         ------

3V3   ----------------------> VCC
GND   ----------------------> GND
GPIO21 ---------------------> SDA
GPIO22 ---------------------> SCL

3V3   ----------------------> CSB
GND   ----------------------> SDO
        (if these pins are exposed)
```

CSB must be high for I2C operation.

SDO low selects address 0x76.
SDO high selects address 0x77.

Some four-pin breakouts have these signals
configured internally.

## 4. Sensor Identification

The expected BMP280 I2C address is:

    0x76

The expected BMP280 chip ID is:

    0x58

The chip ID is distinct from the I2C address.

The address identifies the device on the bus.

The chip ID identifies the sensor type.

A device acknowledging address 0x76 does not
by itself prove that it is a BMP280.

## 5. Repository Layout

```text
esp32-hardware-interface-labs/
|
+-- i2c/
    |
    +-- lab02_i2c_master/
        |
        +-- main/
        |   +-- ESP32 firmware source
        |
        +-- hil/
            |
            +-- conftest.py
            +-- pytest.ini
            +-- serial_device.py
            +-- tests/
            +-- tools/
            +-- docs/
            +-- reports/ (generated)
```

## 6. Prepare ESP-IDF on Windows

Open an ESP-IDF-enabled PowerShell terminal.

Alternatively, activate the installed ESP-IDF
environment using its export.ps1 script.

Verify:

    idf.py --version

The reference environment uses ESP-IDF v5.5.5.

Navigate to the firmware project:

    cd C:\Users\babar\esp32-hardware-interface-labs\i2c\lab02_i2c_master

Build:

    idf.py build

Flash the connected ESP32:

    idf.py -p COM3 flash

Open the serial monitor:

    idf.py -p COM3 monitor

Exit the monitor using:

    Ctrl + ]

Important: close the serial monitor before
running Python HIL tests.

Only one process should control COM3 at a time.

## 7. Create the Python Environment

Open PowerShell at the repository root:

    cd C:\Users\babar\esp32-hardware-interface-labs

Create the virtual environment if it does
not already exist:

    py -m venv .venv-hil

Activate:

    .\.venv-hil\Scripts\Activate.ps1

Install the required packages:

    python -m pip install --upgrade pip

    python -m pip install pytest pytest-html pyserial

Verify:

    python --version
    python -m pytest --version
    python -m pip show pyserial pytest-html

The validated local Python version was 3.14.7.

The GitHub Actions workflow uses Python 3.11,
so successful CI results also demonstrate the
hardware-independent checks in that environment.

## 8. Confirm Serial Connectivity

List serial ports:

    python -m serial.tools.list_ports

Expected in the reference setup:

    COM3

The actual port may differ.

The HIL host communicates using:

- 115200 baud
- newline-delimited ASCII commands
- serial responses prefixed with HIL:

The SerialDevice class:

1. Opens the serial port.
2. Waits for the ESP32 startup/reset.
3. Clears stale input.
4. Sends a newline-terminated command.
5. Ignores non-HIL boot logs.
6. Parses the HIL response.
7. Raises an exception on timeout or protocol error.

## 9. Host/Firmware Protocol

The Python host uses commands including:

    PING
    GET_STATUS
    GET_SENSOR_INFO
    READ_SENSOR
    REINIT_SENSOR

Example:

    Host: PING
    ESP32: HIL:PONG

For measurements, the host expects fields:

    TEMP_C
    PRESSURE_HPA

The exact response contract is implemented
in serial_device.py and the firmware.

Do not change field names without updating
the corresponding tests.

## 10. Run Simulation Tests

Navigate to:

    cd C:\Users\babar\esp32-hardware-interface-labs\i2c\lab02_i2c_master\hil

Run:

    python -m pytest -c pytest.ini -v -m simulation tests --junitxml=reports/simulation-results.xml

Expected reference baseline:

    8 passed

Simulation tests do not require an ESP32.

## 11. Run Verification-Tool Unit Tests

Run:

    python -m pytest -c pytest.ini -v tools/tests --junitxml=reports/verification-tools-results.xml

Expected reference baseline:

    40 passed

These tests verify the evidence parser,
requirement-status logic, and quality gates.

## 12. Run Physical HIL Tests

Confirm:

- ESP32 connected
- BMP280 wired correctly
- Compatible firmware flashed
- COM3 available
- ESP-IDF monitor closed

Run:

    python -m pytest -c pytest.ini -v -m hardware tests --port COM3 --junitxml=reports/hardware-results.xml

Expected reference baseline:

    20 passed

These tests exercise physical communication
with the ESP32 and BMP280.

The test suite covers connectivity, sensor
identity, measurements, protocol behavior,
recovery, and robustness.

## 13. Generate Traceability

Run:

    python tools/generate_traceability.py

Expected:

    Total requirements: 10
    Mapped requirements: 10
    Unmapped requirements: 0
    Mapping coverage: 100.0%

## 14. Generate Verification Evidence

After fresh simulation and hardware runs:

    python tools/generate_verification_evidence.py

Expected reference baseline:

    Total requirements: 10
    Verified requirements: 10
    Verification coverage: 100.0%

## 15. Execute Quality Gates

Run:

    python tools/check_quality_gates.py --require-verification

Expected:

    G1_requirements_mapping: PASS
    G2_test_reference_validity: PASS
    G3_verification_evidence: PASS

G3 must only be treated as meaningful when
the underlying JUnit reports are known to
come from the intended current test runs.

## 16. Generated Reports

The reports directory contains generated
verification artifacts such as:

- simulation-results.xml
- hardware-results.xml
- verification-tools-results.xml
- requirements-traceability.csv
- requirements-coverage.json
- requirements-verification.csv
- verification-evidence.json
- quality-gates.json

Generated reports are excluded from Git
using .gitignore.

The GitHub Actions workflow uploads its
own CI reports as workflow artifacts.

## 17. Troubleshooting

### COM3 cannot be opened

Possible causes:

- ESP-IDF monitor still running
- Another terminal has opened COM3
- Incorrect port
- USB cable problem
- Missing USB-to-UART driver

Check:

    python -m serial.tools.list_ports

Close competing serial applications.

### Sensor not detected

Check:

- Sensor power
- Common ground
- SDA and SCL connections
- Sensor address
- CSB and SDO configuration
- Pull-up resistors
- Firmware configuration

### Sensor ID mismatch

A valid I2C acknowledgment is not
sufficient to confirm BMP280 identity.

Verify that the device reports chip ID 0x58.

### Hardware tests are deselected

Check pytest marker collection:

    python -m pytest -c pytest.ini --collect-only -q -m hardware tests

Expected:

    20/28 tests collected

Simulation selection:

    python -m pytest -c pytest.ini --collect-only -q -m simulation tests

Expected:

    8/28 tests collected

### Hardware tests time out

Check:

- Correct firmware
- Baud rate
- Serial port
- HIL response prefix
- ESP32 startup/reset
- USB connection

### Requirements verification incomplete

Check that both JUnit files exist and
contain results from the current test run.

Regenerate traceability and evidence.

Do not reuse an empty hardware-results.xml
from a zero-selected-test run.

## 18. Verification Boundaries

The successful tests demonstrate the
behavior exercised by the test suite.

They do not independently establish:

- Electrical signal integrity
- I2C rise/fall-time compliance
- Long-duration reliability
- Environmental calibration accuracy
- Operation across all BMP280 breakouts
- Compliance with a safety standard

Additional instrumentation and testing
would be needed for those claims.

## 19. Reproducibility Checklist

- [ ] Correct ESP32 firmware built
- [ ] Firmware flashed successfully
- [ ] BMP280 connected at address 0x76
- [ ] Sensor ID verified as 0x58
- [ ] Serial port identified
- [ ] Python dependencies installed
- [ ] 8 simulation tests passed
- [ ] 40 verification-tool tests passed
- [ ] 20 hardware tests passed
- [ ] 10 requirements mapped
- [ ] 10 requirements verified
- [ ] G1, G2, and G3 passed
- [ ] GitHub Actions jobs passed
```

## 4. Update the existing I²C lab README

I also checked the repository's `i2c/lab02_i2c_master/README.md`. Its *Current Limitation* section still states that no external I²C peripheral was available and physical device transactions remain unvalidated.

That statement should be retained as historical context only, not as the current project status.

Replace that section with:

<CodeBlock language="markdown" editable>
## Physical Peripheral Validation — BMP280

The original I2C bus abstraction and scanner were initially
developed without an external peripheral.

The lab has since been extended with a physical BMP280
sensor and a Python-based HIL verification framework.

The verified Pass 9.11 baseline includes:

- 20 passing physical HIL tests.
- 8 passing simulation tests.
- 40 passing verification-tool unit tests.
- 10/10 requirements mapped.
- 10/10 requirements verified.
- G1, G2, and G3 quality gates passing locally.
- Both GitHub Actions CI jobs passing.

See:

- `hil/docs/ARCHITECTURE.md`
- `hil/docs/HARDWARE_SETUP.md`

The BMP280 validation does not establish compatibility
with every possible I2C peripheral.
