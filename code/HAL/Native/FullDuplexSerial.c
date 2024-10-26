#include "FullDuplexSerial.h"
#include <stdlib.h>
#include <propeller2.h>
#include <stdio.h>
#include "Debug.h"

#define SERIAL_MEMORY_SIZE 1024

static void fds_thread(void *arg)
{
    FullDuplexSerial *self = (FullDuplexSerial *)arg;
    self->isRunning = true;
    while (self->isRunning)
    {
        uint8_t rxbyte = 0;
        if (socketio_receive(self->socket_id, &rxbyte, 1) == 1)
        {
            lib_staticQueue_push(&(self->rx_queue), &rxbyte);
            // printf("Received byte: %d\n", rxbyte);
        }
        if (lib_staticQueue_isempty(&(self->tx_queue)) == false)
        {
            uint8_t byte = 0;
            lib_staticQueue_pop(&(self->tx_queue), &byte);
            socketio_send(self->socket_id, byte);
        }
        // printf("Serial thread\n");
    }
}

bool fds_start(FullDuplexSerial *self, uint8_t rxpin, uint8_t txpin, uint8_t mode, uint32_t baud)
{
    bool result = false;
    if (self != NULL)
    {
        self->rxpin = rxpin;
        DEBUG_INFO("Serial started: %d\n", rxpin);
        self->socket_id = get_pin_socketid(rxpin);

        if (self->socket_id > 0)
        {
            // Initialize the queues
            lib_staticQueue_init(&(self->rx_queue), self->rx_buffer, RX_BUFFER_SIZE, sizeof(uint8_t));
            lib_staticQueue_init(&(self->tx_queue), self->tx_buffer, TX_BUFFER_SIZE, sizeof(uint8_t));

            self->cog_id = _cogstart_C(fds_thread, self, &(self->stack[0]), sizeof(long) * SERIAL_MEMORY_SIZE);
            result = (self->cog_id > 0);
        }
        else
        {
            perror("Unable to open socket port\n");
        }
    }
    return result;
}

void fds_stop(FullDuplexSerial *self)
{
    if (self->cog_id > 0)
    {
        _cogstop(self->cog_id);
    }
    self->isRunning = false;
    DEBUG_INFO("Serial stopped: %d\n", self->rxpin);
}

int32_t fds_rxcheck(FullDuplexSerial *self)
{
    uint8_t byte = 0;
    if (lib_staticQueue_pop(&(self->tx_queue), &byte) == false)
    {
        byte = -1;
    }
    return byte;
}

uint32_t fds_avilable(FullDuplexSerial *self)
{
    return lib_staticQueue_count(&(self->rx_queue));
}

uint8_t fds_rx(FullDuplexSerial *self)
{
    uint8_t byte = 0;
    while (lib_staticQueue_isempty(&(self->rx_queue)))
    {
        // wait for data
    }
    lib_staticQueue_pop(&(self->rx_queue), &byte);
    return byte;
}

uint8_t fds_rxtime(FullDuplexSerial *self, uint32_t timeout)
{
    // use serial rxcheck and serial rx and _getms() to implement this
    uint32_t start = _getms();
    while (lib_staticQueue_isempty(&(self->rx_queue)))
    {
        if ((_getms() - start) > timeout)
        {
            DEBUG_ERROR("fds rx timeout on pin %d\n", self->rxpin);
            return 0;
        }
    }
    return fds_rx(self);
}

void fds_tx(FullDuplexSerial *self, uint8_t byte)
{
    if (self)
    {
        lib_staticQueue_push(&(self->tx_queue), &byte);
    }
    return;
}

void fds_flush(FullDuplexSerial *self)
{
    lib_staticQueue_empty(&(self->rx_queue));
}
