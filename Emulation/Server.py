#
#   Hello World client in Python
#   Connects REQ socket to tcp://localhost:5555
#   Sends "Hello" to server, expects "World" back
#
import socket
import subprocess
import threading
import select
import time
import serial
import logging
import sys
import struct
from ForceGauge import SimulatedADS122U04

logger = logging.getLogger(__name__)

logging.basicConfig(
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler("your_log_file_name.log", mode="w"),
    ],
    level=logging.INFO,
    format="%(asctime)s.%(msecs)03d - %(levelname)s - %(message)s",
    datefmt="%H:%M:%S",
)

# Run socat command in the background to create virtual serial ports
socat_command = "socat -d -d pty,raw,echo=0,link=/tmp/virtual_port1 pty,raw,echo=0,link=/tmp/virtual_port2"
subprocess.Popen(socat_command, shell=True)

HOST = '127.0.0.1'  # IP address where the server is running
PORT = 1234        # Port number the server is listening on

# Build firmware
try:
  subprocess.check_call(['pio', 'run', '-e', 'emulator', '--project-dir', '../'])
except subprocess.CalledProcessError:
  # There was an error - command exited with non-zero code
    logger.error("Error building firmware")
    exit(1)

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen()
logger.info(f"Server is listening on {HOST}:{PORT}")

# Run firmware
import subprocess
import threading
from time import sleep

# Function to read and print the output of the C program
def read_output(process):
    while True:
        if process.poll() is not None:
            logger.error("C program terminated with exit code: " + str(process.poll()))
            return
        output = process.stdout.readline()
        logger.info("Firmware: "+output.strip())
        sleep(0.01)

# Start the C program as a subprocess
c_process = subprocess.Popen(f'../.pio/build/emulator/program', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True, universal_newlines=True)

# Create a thread to continuously read and print the output
output_thread = threading.Thread(target=read_output, args=(c_process,))
output_thread.start()

logger.info("Starting server process")

def handle_rpi_data(client_socket):
    ser = serial.Serial('/tmp/virtual_port1', 9600, timeout=0)
    while True:
        data = client_socket.recv(1024)
        if not data:
            break  # Exit the loop if no data is received
        # write to serial port mad_simulator using pyserial
        ser.write(data)

        # read from serial port mad_simulator using pyserial
        data = ser.read(1024)
        if not data:
            time.sleep(0.001)
            continue
        # send data back to client
        client_socket.send(data)
        #sleep thread
        time.sleep(0.001)

forceGauge = SimulatedADS122U04(logger)
def handle_force_gauge_data(client_socket):
    while True:
        try:
            received_data = client_socket.recv(1)
        except Exception as e:
            logger.warning(f"Error recieving data: {e}")
            break

        if not received_data:
            break  # Break the loop if no more data is received

        res = forceGauge.process_serial_data(received_data[0])

        if res is not None:
            logger.info(f"Received complete frame: {res}")
            logger.info(f"Sending response: {res}")
            try:
                client_socket.send(struct.pack('!B', res))
                pass
            except Exception as e:
                print(f"Error sending data: {e}")
                # Handle the exception as needed
        time.sleep(0.01)

def handle_force_gauge_data_continuous(client_socket):
    while True:
        try:
            client_socket.send(struct.pack('!B', 5))
            client_socket.send(struct.pack('!B', 5))
            client_socket.send(struct.pack('!B', 5))
            pass
        except Exception as e:
            print(f"Error sending data: {e}")
            break
        time.sleep(0.01) # 1000sps

def handle_client(client_socket, client_id):
    logger.info(f"Started thread for client {client_id}")
    try:
        if client_id == "Serial-53-55":
            handle_rpi_data(client_socket)
        elif client_id == "Serial-0-2":
            handle_force_gauge_data(client_socket)
        elif client_id == "Serial-0":
            handle_force_gauge_data_continuous(client_socket)
        else:
            logger.error(f"Unknown client ID '{client_id}'")
    finally:
        logger.error(f"Connection from client {client_id} closed")
        client_socket.close()

try:
    while True:
        client_socket, addr = server_socket.accept()
        id_data = b''  # Initialize an empty bytes object to store the received data
        while True:
            chunk = client_socket.recv(1)  # Receive one byte at a time
            if not chunk or chunk == b'\n':
                break  # Stop if no data received or newline is encountered
            id_data += chunk
            
        client_id = id_data.decode()
        logger.info(f"Accepted connection from {addr} with ID {client_id}")
        # Start a new thread for the client
        client_thread = threading.Thread(target=handle_client, args=(client_socket, client_id))
        client_thread.start()
except KeyboardInterrupt:
    logger.info("Exiting Program")
    server_socket.close()
    c_process.terminate()