import asyncio
import socket
import threading
from . import logger, sim_loop
import asyncio
from .Async import AsyncHandler

class AsyncSocketServer(AsyncHandler, asyncio.Protocol):
    def __init__(self, host='localhost', port=8888):
        self.host = host
        self.port = port
        self.server = None
        self.running = False
        self.transport = None
        self.lock = asyncio.Lock()
    
    # override of AsyncHandler method
    async def rx(self, data: bytes) -> bytes:
        try:
            await self.send(data)
        except Exception as e:
            logger.error(f"Error sending data: {e}")
        return None

    def connection_made(self, transport):
        logger.info(f'Socket connection made: {self.host}:{self.port}')
        self.transport = transport

    def data_received(self, data):
        resp = self.tx(data)
        logger.info(f'Received data from force gauge: {data}')
        if resp is not None:
            self.send(resp)

    def send(self, data):
            if self.transport is not None:
                logger.info(f'Sending data over transport: {data}')
                self.transport.write(data)

    def connection_lost(self, exc):
        print(f'The server closed the connection: {self.host}:{self.port}')

    async def start_server(self):
        self.server = await sim_loop.create_server(
            lambda: self,
            self.host,
            self.port
        )
        logger.info(f'Socket server started on {self.host}:{self.port}')
    
    def is_running(self):
        return self.running

    def run(self):
        try:
            logger.info('Starting socket server')
            self.running = True
            asyncio.run_coroutine_threadsafe(self.start_server(), sim_loop)
        except RuntimeError:
            logger.warn('No event loop found')
    
    def stop(self):
        if self.server is not None:
            self.server.close()
            self.server = None
        self.running = False
        logger.info(f'Socket server stopped on {self.host}:{self.port}')