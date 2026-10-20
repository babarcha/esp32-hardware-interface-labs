import math
import time


def test_temperature_range(device):
    measurement = device.read_measurement()

    assert math.isfinite(measurement.temperature_c)
    assert -40.0 <= measurement.temperature_c <= 85.0


def test_pressure_range(device):
    measurement = device.read_measurement()

    assert math.isfinite(measurement.pressure_hpa)
    assert 300.0 <= measurement.pressure_hpa <= 1100.0


def test_repeated_measurements(device):
    measurements = []

    for _ in range(5):
        measurement = device.read_measurement()
        measurements.append(measurement)
        time.sleep(0.5)

    assert len(measurements) == 5

    for measurement in measurements:
        assert math.isfinite(measurement.temperature_c)
        assert math.isfinite(measurement.pressure_hpa)

    temperatures = [
        m.temperature_c for m in measurements
    ]

    pressures = [
        m.pressure_hpa for m in measurements
    ]

    # Broad limits suitable for a stationary lab setup.
    assert max(temperatures) - min(temperatures) < 10.0
    assert max(pressures) - min(pressures) < 20.0
