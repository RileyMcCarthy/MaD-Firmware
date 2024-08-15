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

// https://stackoverflow.com/questions/51731313/cross-platform-crc8-function-c-and-python-parity-check
#include "IO_CRC.h"

uint8_t IO_protocol_private_crc8(uint8_t *addr, uint16_t len)
{
    uint8_t crc = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t inbyte = addr[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

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
    uint8_t crc = IO_protocol_private_crc8((uint8_t *)buf, size);
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
    if (IO_protocol_private_recieveByte(&byte))
    {
        length = byte;
        // if (IO_protocol_recieveByte(fds, &byte))
        //{
        //     length |= (byte << 8);
        // } not supported
    }
    return length;
}

uint16_t IO_protocol_recieveData(char *buf, uint16_t size)
{
    uint16_t length = 0U;
    uint8_t byte = 0U;
    uint8_t bytesAvilable = fds_avilable(&IO_protocol_data.fds);
    if (bytesAvilable < size)
    {
        size = bytesAvilable;
    }

    for (int i = 0; i < size; i++)
    {
        if (IO_protocol_private_recieveByte(&byte))
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
