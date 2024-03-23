import serial
import time
import logging
import asyncio

class FullDuplexSerial:
    def __init__(self, serial_port="/tmp/virtual_port1", baud_rate=115200, data_received_callback=None):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.serial = None
        self.logger = logging.getLogger(__name__)
        self.data_received_callback = data_received_callback

    def connect(self):
        try:
            if self.serial is not None and self.serial.isOpen():
                self.serial.close()
            self.logger.info(f"Connecting to Serial port: {self.serial_port} with baud: {self.baud_rate}")
            self.serial = serial.Serial(self.serial_port, self.baud_rate, timeout=0, write_timeout=0, inter_byte_timeout=1)
            self.serial.reset_input_buffer()
            self.logger.info(f"Serial connected using: {self.serial.name}")
            return True
        except serial.SerialException as error:
            self.logger.error(f"Unable to open serial: {error}")
            return False

    def connect_blocking(self, delay=1):
        while True:
            if self.connect():
                return
            asyncio.sleep(delay)

    def write_byte(self, byte):
        try:
            self.serial.write(byte)
            return True
        except serial.SerialException as e:
            self.logger.warning(f"Error writing to serial: {e}")
            return False

    async def read_byte(self):
        try:
            data = self.serial.read(1)
            return data
        except serial.SerialException as e:
            self.logger.warning(f"Error reading from serial: {e}")
            return None

    async def handle_data(self):
        while True:
            data = await self.read_byte()
            if data:
                # Call the callback function with the received data
                if self.data_received_callback:
                    self.data_received_callback(data)
    
    def start(self):
        asyncio.run(self.handle_data())