#ifndef app_message_slave_RX_H
#define app_message_slave_RX_H
//
// Created by Riley McCarthy on 25/04/24.
// @brief Handles the reception of messages from the serial port, can trigger TX messages
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdint.h>
#include "IO_protocol.h"
/**********************************************************************
 * Constants
 **********************************************************************/
#define APP_MESSAGE_SLAVE_RX_BUFFER_SIZE 1023
#define APP_MESSAGE_SLAVE_TX_BUFFER_SIZE 1023
/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef enum
{
    APP_MESSAGE_SLAVE_STATE_INIT,
    APP_MESSAGE_SLAVE_STATE_WAITING,         // wait for sync or interval
    APP_MESSAGE_SLAVE_STATE_READING_COMMAND, // get read/write command
    APP_MESSAGE_SLAVE_STATE_READING_HEADER,  // get length to read (do we always have length?)
    APP_MESSAGE_SLAVE_STATE_READING_DATA,    // reading data of length
    APP_MESSAGE_SLAVE_STATE_READING_CRC,     // check crc
    APP_MESSAGE_SLAVE_STATE_READING_PROCESS, // decode the JSON data
    APP_MESSAGE_SLAVE_STATE_RESPOND,         // Respond to command with data
    APP_MESSAGE_SLAVE_STATE_AWK,             // Send AWK
    APP_MESSAGE_SLAVE_STATE_COUNT,
} app_message_slave_state_E;

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
void app_messageSlave_init(int lock);
void app_messageSlave_run(void);

bool app_messageSlave_requestMotionEnabled(void);
bool app_messageSlave_requestTestMode(void);
/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* app_message_slave_RX_H */
