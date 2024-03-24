import MaDSim
from time import sleep

# The base port should be sent to firmware cause it sometimes has conflicts with existing ports
# MAYBE ONCE STABLE RUN IN DOCKER
socket_port_base = 9200

# Build firmware
firmware = MaDSim.FirmwareRunner("emulator", "../")
firmware.build()

force_gauge_pin = 0
forceGauge = MaDSim.SimulatedADS122U04()
force_async_server = MaDSim.AsyncSocketServer("localhost", socket_port_base + force_gauge_pin)
MaDSim.AsyncConnector(forceGauge, force_async_server)
force_async_server.run()

rpi_pin = 53
#rpi_serial = MaDSim.VirtualSerialPort("rpi_client", "rpi") run it manually, its easier lol
sleep(3)
rpi_async_serial_server = MaDSim.AsyncSerialServer("rpi_client")
rpi_async_server = MaDSim.AsyncSocketServer("localhost", socket_port_base + rpi_pin)
MaDSim.AsyncConnector(rpi_async_serial_server, rpi_async_server)
#rpi_serial.start()
rpi_async_serial_server.run()
rpi_async_server.run()

sleep(1)

firmware.run()

try:
    while True:
        if not firmware.is_running():
            MaDSim.logger.error("Firmware has stopped, Exiting server process")
            break
        #if not rpi_serial.is_running():
        #    MaDSim.logger.error("RPI VSP has stopped, Exiting server process")
        #    break
        if not rpi_async_serial_server.is_running():
            MaDSim.logger.error("RPI async serial server has stopped, Exiting server process")
            break
        if not rpi_async_server.is_running():
            MaDSim.logger.error("RPI async server has stopped, Exiting server process")
            break
        if not force_async_server.is_running():
            MaDSim.logger.error("Force async server has stopped, Exiting server process")
            break
        if not MaDSim.async_handler.is_running():
            MaDSim.logger.error("Async handler has stopped, Exiting server process")
            break
        sleep(0.1)
finally:
    firmware.stop()
    rpi_async_server.stop()
    force_async_server.stop()
    MaDSim.async_handler.stop_loop()