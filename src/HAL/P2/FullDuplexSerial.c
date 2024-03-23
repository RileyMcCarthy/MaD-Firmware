#include "FullDuplexSerial.h"
#include <stdlib.h>

bool fds_start(FullDuplexSerial *self, uint8_t rxpin, uint8_t txpin, uint8_t mode, uint32_t baud)
{
    return (self->start(rxpin, txpin, mode, baud) > 0);
}

void fds_stop(FullDuplexSerial *self)
{
    self->stop();
}

int fds_rxcheck(FullDuplexSerial *self)
{
    return self->rxcheck();
}
int fds_rx(FullDuplexSerial *self)
{
    return self->rx();
}
int fds_rxtime(FullDuplexSerial *self, uint32_t timeout)
{
    return self->rxtime(timeout);
}
void fds_tx(FullDuplexSerial *self, uint8_t c)
{
    self->tx(c);
}

