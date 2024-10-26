# First-time Setup

## Platform.io

Platform.io must be installed inside Visual Studio Code to build firmware: https://docs.platformio.org/en/latest/integration/ide/vscode.html#installation

Make sure to remove previous Platform Propeller installations under Platform.io-> Home -> Platforms

Install the Propeller Platform.io platform using the instructions at the link below
https://github.com/RileyMcCarthy/platform-propeller

Once the custom Platform.io platform is installed clone this repository

`https://github.com/RileyMcCarthy/MaD-Firmware.git`

To build the firmware open the MaD-Firmware folder to compile and upload!

# Firmware setup

## Abstraction Layers

### HW
The hardware layer defines the PCBA board layout including:
1. Pin Definitions
2. Clock Frequency
3. Board layout

### HAL
The hardware abstraction layer is used to generalize the software from the hardware. Only this layer is alowed to call HW functions inside propeller2.h and write to registers.
1. GPIO
2. hardware serial
3. hardware encoder
4. I2C

### IO
The input/output layer is used to define tools used to interact with HAL or define protocols of communcation.
1. CRC
2. protocol
3. Static Queue
4. Timer

### DEV
The device layer can combine the HAL + IO layer to develop drivers for devices.
1. Stepper motor
2. COG Manager (should be replaced by RTOS eventually)
3. Logger
4. NVRAM (only implements SD card nvram)
