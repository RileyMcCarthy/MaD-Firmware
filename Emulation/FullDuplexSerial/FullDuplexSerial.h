#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
#include <stdbool.h>
#include "StaticQueue.h"

#define RX_BUFFER_SIZE 1024
#define TX_BUFFER_SIZE 1024

typedef struct Serial_e {
    uint8_t rxpin;
    uint8_t txpin;
    uint8_t mode;
    uint32_t baud;
    int32_t socket_id;
    int cog_id;
    long stack[1024];
    StaticQueue rx_queue;
    StaticQueue tx_queue;
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    uint8_t tx_buffer[TX_BUFFER_SIZE];
    bool isRunning;
} Serial;

void serial_start(Serial *self, uint8_t rxpin, uint8_t txpin, uint8_t mode, uint32_t baud);
void serial_stop(Serial *self);
bool serial_rxcheck(Serial *self);
uint8_t serial_rx(Serial *self);
uint8_t serial_rxtime(Serial *self, uint32_t timeout);
void serial_tx(Serial *self, uint8_t c);

#endif // SERIAL_H
