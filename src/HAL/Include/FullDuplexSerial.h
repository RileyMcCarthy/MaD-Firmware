#ifndef FullDuplexSerial_H
#define FullDuplexSerial_H
#include <stdint.h>
#include <stdbool.h>
#include "FullDuplexSerial_private.h"

bool fds_start(FullDuplexSerial *self, uint8_t rxpin, uint8_t txpin, uint8_t mode, uint32_t baud);
void fds_stop(FullDuplexSerial *self);
bool fds_rxcheck(FullDuplexSerial *self);
uint8_t fds_rx(FullDuplexSerial *self);
uint8_t fds_rxtime(FullDuplexSerial *self, uint32_t timeout);
void fds_tx(FullDuplexSerial *self, uint8_t c);

#endif // FullDuplexSerial_H
