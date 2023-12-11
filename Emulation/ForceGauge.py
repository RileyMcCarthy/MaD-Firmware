class SimulatedADS122U04:
    STATUS_SUCCESS = "SUCCESS"
    STATUS_INSUFFICIENT_DATA = "INSUFFICIENT_DATA"
    STATUS_INVALID_SYNC_BYTE = "INVALID_SYNC_BYTE"
    STATUS_INVALID_COMMAND = "INVALID_COMMAND"

    def __init__(self, logger):
        self.registers = {
            0x00: 0x00,  # Configuration Register 0
            0x01: 0x00,  # Configuration Register 1
            0x02: 0x00,  # Configuration Register 2
            0x03: 0x00,  # Configuration Register 3
            # Add more registers as needed
        }
        self.data = []
        self.logger = logger

    def clear_data(self):
        self.data.clear()

    def write_register(self, register, value):
        # Simulate writing to registers
        self.registers[register] = value

    def read_register(self, register):
        # Simulate reading from registers
        return self.registers.get(register, 0)

    def process_serial_data(self, byte):
        res = None
        # Append the byte to the bytearray
        self.data.append(byte)
        self.logger.info(f'Current data: {self.data}')
        if len(self.data) == 1:
            if self.data[0] != 0x55:
                self.data.clear()
                self.logger.warning("byte not 0x55")
        elif len(self.data) in [2,3]:
            command = (self.data[1] >> 4) & 0x0F  # Mask the upper nibble
            register_address = (self.data[1] >> 1) & 0x0F  # Mask the lower nibble
            if self.data[1] == 0x01:
                # some command, do something
                self.logger.info("executing command 0x01")
                self.clear_data()
            elif self.data[1] == 0x06:
                # some command, do something
                self.logger.info("executing command 0x06")
                self.clear_data()
            elif self.data[1] == 0x08:
                # some command, do something
                self.logger.info("executing command 0x08")
                self.clear_data()
            elif command == 0b0010:
                # Simulate read operation
                res = self.read_register(register_address)
                self.logger.info(f"Read register {register_address} with value {res}")
                self.clear_data()
            elif command == 0b0100:
                # Simulate write operation
                if len(self.data) == 3:  # Adjusted length for the value bytes
                    value = self.data[2]
                    self.write_register(register_address, value)
                    self.logger.info(f"Wrote value {value} to register {register_address}")
                    self.clear_data()
            else:
                self.logger.info("invalid command")
                self.clear_data()
        else:
            self.logger.info("no commands with 4 bytes")
            self.clear_data()
        return res
