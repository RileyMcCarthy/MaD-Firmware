#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
#include <stdbool.h>

typedef struct __using("Propeller2/FullDuplexSerial/jm_fullduplexserial.spin2") Serial;

void serial_start(Serial *self, uint8_t rxpin, uint8_t txpin, uint8_t mode, uint32_t baud);
void serial_stop(Serial *self);
int serial_rxcheck(Serial *self);
int serial_rx(Serial *self);
int serial_rxtime(Serial *self, uint32_t timeout);
void serial_tx(Serial *self, uint8_t c);

#endif // SERIAL_H
