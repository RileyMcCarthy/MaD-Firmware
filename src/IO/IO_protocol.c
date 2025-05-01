//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <string.h>

#include "IO_protocol.h"
#include "FullDuplexSerial.h"
#include "HW_pins.h"
#include "lib_utility.h"
#include <propeller2.h>
#include "IO_Debug.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/
#define IO_PROTOCOL_RECIEVE_TIMEOUT_MS 100U
/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef enum
{
    IO_PROTOCOL_RECIEVE_STATE_SYNC,
    IO_PROTOCOL_RECIEVE_STATE_TYPE,
    IO_PROTOCOL_RECIEVE_STATE_COMMAND,
    IO_PROTOCOL_RECIEVE_STATE_LENGTH,
    IO_PROTOCOL_RECIEVE_STATE_DATA,
    IO_PROTOCOL_RECIEVE_STATE_CRC,
    IO_PROTOCOL_RECIEVE_STATE_COUNT,
} IO_protocol_recieveState_E;

typedef struct
{
    uint32_t startms;
    IO_protocol_incommingType_E type;
    uint8_t command;
    uint16_t dataLength;
    uint16_t dataIndex;
    uint8_t crc;
    IO_protocol_recieveState_E state;
} IO_protocol_recieve_S;

typedef struct
{
    FullDuplexSerial fds;
    IO_protocol_recieve_S recieve;
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

static bool IO_protocol_private_timeout(void)
{
    return ((_getms() - IO_PROTOCOL_RECIEVE_TIMEOUT_MS) > IO_protocol_data.recieve.startms);
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

bool IO_protocol_private_recieveSync()
{
    uint8_t sync = 0U;
    bool ret = false;
    if (IO_protocol_private_recieveByte(&sync))
    {
        ret = (sync == 0x55);
    }
    return ret;
}

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void IO_protocol_init()
{
    fds_start(&IO_protocol_data.fds, HW_PIN_RPI_RX, HW_PIN_RPI_TX, 0, 115200);
}

IO_protocol_incommingType_E IO_protocol_recieveRequest(IO_protocol_readType_E *readType, IO_protocol_writeType_E *writeType, void *data, uint32_t *size, const uint16_t maxSize)
{
    IO_protocol_incommingType_E incommingType = IO_PROTOCOL_INCOMMING_TYPE_NONE;
    switch (IO_protocol_data.recieve.state)
    {
    case IO_PROTOCOL_RECIEVE_STATE_SYNC:
        if (IO_protocol_private_recieveSync())
        {
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_TYPE;
            IO_protocol_data.recieve.startms = _getms();
        }
        break;
    case IO_PROTOCOL_RECIEVE_STATE_TYPE:
        if (IO_protocol_private_recieveByte((uint8_t *)&IO_protocol_data.recieve.type))
        {
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_COMMAND;
        }
        else if (IO_protocol_private_timeout())
        {
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
        }
        break;
    case IO_PROTOCOL_RECIEVE_STATE_COMMAND:
        if (IO_protocol_private_recieveByte(&IO_protocol_data.recieve.command))
        {
            if (IO_protocol_data.recieve.type == IO_PROTOCOL_INCOMMING_TYPE_READ)
            {
                incommingType = IO_PROTOCOL_INCOMMING_TYPE_READ;
                *readType = IO_protocol_data.recieve.command;
                IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
            }
            else if (IO_protocol_data.recieve.type == IO_PROTOCOL_INCOMMING_TYPE_WRITE)
            {
                IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_LENGTH;
            }
        }
        else if (IO_protocol_private_timeout())
        {
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
        }
        break;
    case IO_PROTOCOL_RECIEVE_STATE_LENGTH:
        if (fds_avilable(&IO_protocol_data.fds) > 2U)
        {
            uint8_t bytes[2];
            bool recieveSuccess = IO_protocol_private_recieveByte(&bytes[0]);
            recieveSuccess &= IO_protocol_private_recieveByte(&bytes[1]);
            if (recieveSuccess)
            {
                const uint16_t dataLength = bytes[0] | (bytes[1] << 8);
                IO_protocol_data.recieve.dataLength = LIB_UTILITY_LIMIT(dataLength, 0U, maxSize);
                IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_DATA;
                IO_protocol_data.recieve.dataIndex = 0U;
                DEBUG_INFO("Recieved data length: %d\n", IO_protocol_data.recieve.dataLength);
            }
            else
            {
                // this really shouldnt happen
                DEBUG_ERROR("%s","Failed to recieve data length\n");
                IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
            }
        }
        else if (IO_protocol_private_timeout())
        {
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
        }
        break;
    case IO_PROTOCOL_RECIEVE_STATE_DATA:
        while (fds_avilable(&IO_protocol_data.fds) > 0)
        {
            const uint16_t index = LIB_UTILITY_LIMIT(IO_protocol_data.recieve.dataIndex, 0U, maxSize);
            if (IO_protocol_private_recieveByte(&data[index]))
            {
                IO_protocol_data.recieve.dataIndex++;
                if (IO_protocol_data.recieve.dataIndex >= IO_protocol_data.recieve.dataLength)
                {
                    IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_CRC;
                    break;
                }
            }
        }
        if (IO_protocol_private_timeout())
        {
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
        }
        break;
    case IO_PROTOCOL_RECIEVE_STATE_CRC:
        if (IO_protocol_private_recieveByte(&IO_protocol_data.recieve.crc))
        {
            const uint8_t crc = lib_utility_CRC8(data, IO_protocol_data.recieve.dataLength);
            if (crc == IO_protocol_data.recieve.crc)
            {
                incommingType = IO_PROTOCOL_INCOMMING_TYPE_WRITE;
                *writeType = IO_protocol_data.recieve.command;
                *size = LIB_UTILITY_LIMIT(IO_protocol_data.recieve.dataLength, 0U, maxSize);
            }
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
        }
        else if (IO_protocol_private_timeout())
        {
            IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
        }
        break;
    case IO_PROTOCOL_RECIEVE_STATE_COUNT:
    default:
        IO_protocol_data.recieve.state = IO_PROTOCOL_RECIEVE_STATE_SYNC;
        break;
    }
    return incommingType;
}

bool IO_protocol_respondNACK(IO_protocol_writeType_E originalRequest)
{
    bool ret = true;
    fds_tx(&IO_protocol_data.fds, 0x55);
    fds_tx(&IO_protocol_data.fds, IO_PROTOCOL_OUTGOING_TYPE_NACK);
    fds_tx(&IO_protocol_data.fds, originalRequest);
    return ret;
}

bool IO_protocol_respondACK(IO_protocol_writeType_E originalRequest)
{
    bool ret = true;
    fds_tx(&IO_protocol_data.fds, 0x55);
    fds_tx(&IO_protocol_data.fds, IO_PROTOCOL_OUTGOING_TYPE_ACK);
    fds_tx(&IO_protocol_data.fds, originalRequest);
    return ret;
}

bool IO_protocol_respondData(IO_protocol_readType_E originalRequest, uint8_t *data, uint16_t size)
{
    bool ret = true;
    fds_tx(&IO_protocol_data.fds, 0x55);
    fds_tx(&IO_protocol_data.fds, IO_PROTOCOL_OUTGOING_TYPE_DATA);
    fds_tx(&IO_protocol_data.fds, originalRequest);
    fds_tx(&IO_protocol_data.fds, size);
    fds_tx(&IO_protocol_data.fds, size >> 8);

    for (int i = 0; i < size; i++)
    {
        fds_tx(&IO_protocol_data.fds, data[i]);
    }
    uint8_t crc = lib_utility_CRC8((uint8_t *)data, size);
    fds_tx(&IO_protocol_data.fds, crc);
    return ret;
}

bool IO_protocol_sendNotification(uint8_t *data, uint16_t size)
{
    // need to figure out how to make this have same structure as respondData. I feel like it might be easier
    // also notifcations arent working, due to missing originalRequest. this causes the read byte to be large and stall UI recieving
    bool ret = true;
    fds_tx(&IO_protocol_data.fds, 0x55);
    fds_tx(&IO_protocol_data.fds, IO_PROTOCOL_OUTGOING_TYPE_NOTIFICATION);
    fds_tx(&IO_protocol_data.fds, 0); // this is a hack, dont hate me
    fds_tx(&IO_protocol_data.fds, size);
    fds_tx(&IO_protocol_data.fds, size >> 8);
    for (int i = 0; i < size; i++)
    {
        fds_tx(&IO_protocol_data.fds, data[i]);
    }
    uint8_t crc = lib_utility_CRC8((uint8_t *)data, size);
    fds_tx(&IO_protocol_data.fds, crc);
    return ret;
}

/**********************************************************************
 * End of File
 **********************************************************************/
