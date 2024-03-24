import subprocess
import os
from . import logger
from .Async import AsyncHandler

SERIAL_PORT_PATH = "/tmp/tty."

class VirtualSerialPort(AsyncHandler):
    def __init__(self, link1, link2):
        self.link1 = link1
        self.link2 = link2
        self.process = None
    
    @staticmethod
    def kill_socat():
        subprocess.run(['pkill', '-f', 'socat'])

    def start(self):
        if self.is_running():
            logger.warning("Virtual serial port is already running")
        else:
            port1 = f"{SERIAL_PORT_PATH}{self.link1}"
            port2 = f"{SERIAL_PORT_PATH}{self.link2}"
            command = f"socat -d -d pty,raw,echo=0,link={port1} pty,raw,echo=0,link={port2}"
            self.process = subprocess.Popen(command,shell=True, stderr=subprocess.PIPE)
            logger.info(command)

    def stop(self):
        if self.process:
            self.process.terminate()
            self.process = None

    def is_running(self):
        return self.process and self.process.poll() is None
    
    def rx(self, data):
        return self.send(data)
    
    def send(self, data):
        if self.is_running():
            port = f"{SERIAL_PORT_PATH}{self.link1}"
            with open(port, 'wb') as f:
                f.write(data)
        else:
            logger.error("Virtual serial port is not running")
            return None