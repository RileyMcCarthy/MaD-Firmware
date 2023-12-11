#include "FullDuplexSerial.h"
#include <stdlib.h>

void serial_start(Serial *self, uint8_t rxpin, uint8_t txpin, uint8_t mode, uint32_t baud)
{
    self->start(rxpin, txpin, mode, baud);
}

void serial_stop(Serial *self)
{
    self->stop();
}

int serial_rxcheck(Serial *self)
{
    return self->rxcheck();
}
int serial_rx(Serial *self)
{
    return self->rx();
}
int serial_rxtime(Serial *self, uint32_t timeout)
{
    return self->rxtime(timeout);
}
void serial_tx(Serial *self, uint8_t c)
{
    self->tx(c);
}

