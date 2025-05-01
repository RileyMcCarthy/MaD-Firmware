//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "HAL_serial.h"

#include "smartpins.h"
#include "propeller2.h"
#include <stdlib.h>
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
    const HW_pin_E rx;
    const HW_pin_E tx;
    const int32_t baud;
} HAL_serial_channelConfig_S;

/**********************************************************************
 * External Variables
 **********************************************************************/

/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
static HAL_serial_channelConfig_S HAL_serial_channelConfig[HAL_SERIAL_CHANNEL_COUNT] = {
    {HW_PIN_FORCE_GAUGE_RX, HW_PIN_FORCE_GAUGE_TX, 115200},
};
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

void HAL_serial_private_TXFlush(HAL_serial_channel_E channel)
{
    // flush the transmit buffer
    while (_pinr(HAL_serial_channelConfig[channel].tx) == 0)
        ;
}

uint8_t HAL_serial_private_readByte(HAL_serial_channel_E channel)
{
    uint8_t byte = 0U;
    if (channel < HAL_SERIAL_CHANNEL_COUNT)
    {
        // read the byte from the smartpin
        byte = _rdpin(HAL_serial_channelConfig[channel].rx) >> 24;
    }
    return byte;
}

bool HAL_serial_private_readAvailable(HAL_serial_channel_E channel)
{
    bool dataAvailable = false;
    if (channel < HAL_SERIAL_CHANNEL_COUNT)
    {
        dataAvailable = (_pinr(HAL_serial_channelConfig[channel].rx) != 0);
    }
    return dataAvailable;
}

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void HAL_serial_start(HAL_serial_channel_E channel)
{
    if (channel < HAL_SERIAL_CHANNEL_COUNT)
    {
        // calculate delay between bits
        const int32_t bitPeriod = (_clockfreq() / HAL_serial_channelConfig[channel].baud);

        // calculate smartpin mode for 8 bits per character
        const int32_t bitMode = 7 + (bitPeriod << 16);

        // set up the transmit pin
        _pinstart(HAL_serial_channelConfig[channel].tx, P_OE | P_ASYNC_TX, bitMode, 0);

        // set up the receive pin
        _pinstart(HAL_serial_channelConfig[channel].rx, P_ASYNC_RX, bitMode, 0);
    }
}

void HAL_serial_stop(HAL_serial_channel_E channel)
{
    if (channel < HAL_SERIAL_CHANNEL_COUNT)
    {
        // turn off the smartpin
        _pinclear(HAL_serial_channelConfig[channel].rx);
        _pinclear(HAL_serial_channelConfig[channel].tx);
    }
}

void HAL_serial_transmitData(HAL_serial_channel_E channel, const uint8_t *const data, const uint8_t len)
{
    if (channel < HAL_SERIAL_CHANNEL_COUNT)
    {
        // write the bytes to the smartpin
        for (uint8_t i = 0; i < len; i++)
        {
            _wypin(HAL_serial_channelConfig[channel].tx, &data[i]);

            // wait for the byte to be sent
            HAL_serial_private_TXFlush(channel);
        }
    }
}

bool HAL_serial_recieveDataTimeout(HAL_serial_channel_E channel, uint8_t *const data, uint8_t len, uint32_t timeout_us)
{
    if ((channel < HAL_SERIAL_CHANNEL_COUNT) && (data != NULL))
    {
        // wait for the byte to be received
        const int32_t startTime = _getus();
        for (uint8_t i = 0; i < len; i++)
        {
            while (HAL_serial_private_readAvailable(channel) == false)
            {
                if ((_getus() - startTime) > timeout_us)
                {
                    return false;
                }
            }

            // read the byte from the smartpin
            data[i] = HAL_serial_private_readByte(channel);
        }
        return true;
    }
    return false;
}

/**********************************************************************
 * End of File
 **********************************************************************/
