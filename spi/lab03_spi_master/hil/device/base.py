
from abc import ABC, abstractmethod


class BaseDevice(ABC):
    """Common interface for HIL and simulated devices."""

    @abstractmethod
    def connect(self) -> None:
        pass

    @abstractmethod
    def command(self, request: str) -> str:
        pass

    @abstractmethod
    def close(self) -> None:
        pass

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
