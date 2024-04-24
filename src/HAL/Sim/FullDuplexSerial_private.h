#include <stdint.h>
#include <stdbool.h>
#include "StaticQueue.h"
#include "SocketIO.h"

#define RX_BUFFER_SIZE 1024
#define TX_BUFFER_SIZE 1024

typedef struct FullDuplexSerial_e {
    uint8_t rxpin;
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
} FullDuplexSerial;

