
def test_ping(device):
    assert device.command("PING") == "PONG"


def test_status(device):
    response = device.command("GET_STATUS")
    assert response == "STATUS=READY"


def test_repeated_ping(device):
    for _ in range(5):
        assert device.command("PING") == "PONG"
