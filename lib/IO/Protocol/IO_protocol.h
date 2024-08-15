#ifndef IO_protocol_H
#define IO_protocol_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdint.h>
#include <stdbool.h>
/**********************************************************************
 * Constants
 **********************************************************************/
#define CMD_WRITE 128
#define IO_protocol_COMMAND_VERSION 1

// Eventually not have these commands use use external structs
// should have own unique type with json ecoder/decoder
// should be able to make jsonv2 using static 512 buffer
// can unit test for size issues using max strlen of json
// mid protocol will then bascially remove encode/edecode from app_message
typedef enum
{
    IO_PROTOCOL_COMMAND_PING = 0,          // test communication
    IO_PROTOCOL_COMMAND_DATA = 1,          // send monitor data
    IO_PROTOCOL_COMMAND_STATE = 2,         // send machine state
    IO_PROTOCOL_COMMAND_MPROFILE = 3,      // send/recieve machine profile
    IO_PROTOCOL_COMMAND_MOTIONPROFILE = 6, // send/recieve motion profile
    IO_PROTOCOL_COMMAND_MOTIONMODE = 7,    // send/recieve motion mode
    IO_PROTOCOL_COMMAND_MOTIONSTATUS = 9,  // send/recieve motion status
    IO_PROTOCOL_COMMAND_MOVE = 10,         // start/send sending motion data
    IO_PROTOCOL_COMMAND_AWK = 11,          // send/recieve AWK
    IO_PROTOCOL_COMMAND_MANUAL_MOVE = 14,  // send manual move data
    IO_PROTOCOL_COMMAND_NOTIFICATION = 16, // send notification
    IO_PROTOCOL_COMMAND_START_TEST = 17,   // run test
    IO_PROTOCOL_COMMAND_TEST_HEADER = 19,  // send test header
    IO_PROTOCOL_COMMAND_SNA,
    IO_PROTOCOL_COMMAND_ERROR,
    IO_PROTOCOL_COMMAND_COUNT,
} IO_protocol_command_E;
// should prob not use json for serial
// should just send bits for "Enable motion", "Run Test", etc
// these will become outputs and used for state machine
/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
// need to figure out how to send and recieve data
// do I use non-blocking (need to save data to buffer and remember what stage we are at)
// do I use blocking (need to wait for data to be sent and received) before running next
// Send a command with a buffer of data (non-blocking)
// will need as state machine for a specific message (channels per message???)
// ok make each message channel with own state for sending/receiving
// will prob want different state for send and recv. should keep
// this file as protocol and implement state machine elsewhere
// app_message_slave_rx and app_message_slave_tx?
// app_notification is only tx
// need to figure out how to respond to commands like ping?
// will have types:
// write: writes data on mcu
// read: requests data from mcu
//

// This module is not THREADSAFE, it is assumed that the caller will handle the locking if required
void IO_protocol_init();
bool IO_protocol_send(IO_protocol_command_E cmd, uint8_t *buf, uint16_t size);
bool IO_protocol_recieveSync();
bool IO_protocol_recieveCommand(bool *isWrite, IO_protocol_command_E *command);
uint16_t IO_protocol_recieveLength();
uint16_t IO_protocol_recieveData(char *buf, uint16_t size);
/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* IO_protocol_H */
