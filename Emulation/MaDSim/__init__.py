import logging
import sys

logger = logging.getLogger(__name__)

logging.basicConfig(
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler("MaDSim.log", mode="w"),
    ],
    level=logging.INFO,
    format="%(asctime)s.%(msecs)03d - %(levelname)s - %(message)s",
    datefmt="%H:%M:%S",
)

from MaDSim.AsyncHandler import AsyncLoopHandler
async_handler = AsyncLoopHandler()
async_handler.start_loop()
sim_loop = async_handler.get_loop()
assert sim_loop is not None

from MaDSim.VirtualSerialPort import VirtualSerialPort
from MaDSim.PIORunner import FirmwareRunner
from MaDSim.Async import AsyncHandler, AsyncConnector
from MaDSim.AsyncSocket import AsyncSocketServer
from MaDSim.ForceGauge import SimulatedADS122U04
