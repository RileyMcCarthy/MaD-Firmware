#include "FullDuplexSerial.h"
#include <stdlib.h>
#include "SocketIO.h"
#include <propeller2.h>
#include <stdio.h>

#define SERIAL_MEMORY_SIZE 1024

static void serial_thread(void *arg)
{
    Serial *self = (Serial *)arg;
    self->isRunning = true;
    while (self->isRunning)
    {
        uint8_t rxbyte = 1;
        if (socketio_receive(self->socket_id, &rxbyte, 1) == 1)
        {
            queue_push(&(self->rx_queue), &rxbyte);
        }
        if (queue_isempty(&(self->tx_queue)) == false)
        {
            uint8_t byte = 0;
            queue_pop(&(self->tx_queue), &byte);
            socketio_send(self->socket_id, byte);
        }
        //printf("Serial thread\n");
        _waitus(100);
    }
}

void serial_start(Serial *self, uint8_t rxpin, uint8_t txpin, uint8_t mode, uint32_t baud)
{
    if (self)
    {
        char id[64] = "\0";
        sprintf(id, "Serial-%u-%u\n", rxpin, txpin);
        self->socket_id = socketio_create_socket(id);

        if (self->socket_id == -1)
        {
            perror("Error creating socket");
            exit(1);
        }

        // Initialize the queues
        queue_init(&(self->rx_queue), self->rx_buffer, RX_BUFFER_SIZE, sizeof(uint8_t));
        queue_init(&(self->tx_queue), self->tx_buffer, TX_BUFFER_SIZE, sizeof(uint8_t));

        self->cog_id = _cogstart_C(serial_thread, self, &(self->stack[0]), sizeof(long) * SERIAL_MEMORY_SIZE);
    }
    return;
}

void serial_stop(Serial *self)
{
    self->isRunning = false;
    if (self->cog_id > 0)
    {
        _cogstop(self->cog_id);
    }
    socketio_close(self->socket_id);
    return;
}

bool serial_rxcheck(Serial *self)
{
    return queue_isempty(&(self->rx_queue)) == false;
}
uint8_t serial_rx(Serial *self)
{
    uint8_t byte;
    queue_pop(&(self->rx_queue), &byte);
    return byte;
}
uint8_t serial_rxtime(Serial *self, uint32_t timeout)
{
    // use serial rxcheck and serial rx and _getms() to implement this
    uint32_t start = _getms();
    while (queue_isempty(&(self->rx_queue)))
    {
        _waitms(1);
        if ((_getms() - start) > timeout)
        {
            return -1;
        }
    }
    return serial_rx(self);
}
void serial_tx(Serial *self, uint8_t byte)
{
    if (self)
    {
        queue_push(&(self->tx_queue), &byte);
    }
    return;
}

