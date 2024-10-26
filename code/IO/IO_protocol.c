//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <string.h>

#include "IO_protocol.h"
#include "FullDuplexSerial.h"
#include "IO_digitalPin.h"
#include "lib_utility.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/
typedef struct
{
    FullDuplexSerial fds;
} IO_protocol_data_S;
/**********************************************************************
 * External Variables
 **********************************************************************/

/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
IO_protocol_data_S IO_protocol_data;
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

static bool IO_protocol_private_recieveByte(uint8_t *byte)
{
    bool ret = false;
    if (fds_avilable(&IO_protocol_data.fds) > 0)
    {
        *byte = fds_rx(&IO_protocol_data.fds);
        ret = true;
    }
    return ret;
}
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void IO_protocol_init()
{
    fds_start(&IO_protocol_data.fds, RPI_RX, RPI_TX, 0, 115200);
}

bool IO_protocol_send(IO_protocol_command_E cmd, uint8_t *buf, uint16_t size)
{
    bool ret = true;
    fds_tx(&IO_protocol_data.fds, 0x55);
    fds_tx(&IO_protocol_data.fds, cmd);
    fds_tx(&IO_protocol_data.fds, size);
    fds_tx(&IO_protocol_data.fds, size >> 8);

    for (int i = 0; i < size; i++)
    {
        fds_tx(&IO_protocol_data.fds, buf[i]);
    }
    uint8_t crc = lib_utility_CRC8((uint8_t *)buf, size);
    fds_tx(&IO_protocol_data.fds, crc);
    return ret;
}

bool IO_protocol_recieveSync()
{
    uint8_t sync = 0U;
    bool ret = false;
    if (IO_protocol_private_recieveByte(&sync))
    {
        if (sync == 0x55)
        {
            ret = true;
        }
    }
    return ret;
}

bool IO_protocol_recieveCommand(bool *isWrite, IO_protocol_command_E *command)
{
    uint8_t byte = 0U;
    bool ret = false;
    if (IO_protocol_private_recieveByte(&byte))
    {
        *isWrite = ((byte & CMD_WRITE) == CMD_WRITE);
        byte = byte & ~CMD_WRITE;
        if (byte < IO_PROTOCOL_COMMAND_COUNT)
        {
            ret = true;
            *command = (IO_protocol_command_E)byte;
        }
        else
        {
            *command = IO_PROTOCOL_COMMAND_ERROR; // Error value
        }
    }
    else
    {
        *command = IO_PROTOCOL_COMMAND_SNA;
    }
    return ret;
}

uint16_t IO_protocol_recieveLength()
{
    uint16_t length = 0U;
    uint8_t byte = 0U;
    const uint8_t bytesAvilable = fds_avilable(&IO_protocol_data.fds);
    if (bytesAvilable >= 2)
    {
        if (IO_protocol_private_recieveByte(&byte))
        {
            length = byte;
        }
        if (IO_protocol_private_recieveByte(&byte))
        {
            length |= (byte << 8);
        }
    }
    return length;
}

uint16_t IO_protocol_recieveString(char *buf, uint16_t size)
{
    uint16_t length = 0U;
    char byte = 0U;
    uint8_t bytesAvilable = fds_avilable(&IO_protocol_data.fds);
    if (bytesAvilable < size)
    {
        size = bytesAvilable;
    }

    for (int i = 0; i < size; i++)
    {
        if (IO_protocol_private_recieveByte((uint8_t *)&byte))
        {
            strncat(buf, &byte, 1);
            length++;
        }
    }
    return length;
}
/**********************************************************************
 * End of File
 **********************************************************************/
