//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <propeller2.h>
#include <stdlib.h>
#include "dev_cogManager.h"
#include "Debug.h"
#include "IO_CRC.h"
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
    dev_cogManager_state_E state;
    int cogid;
    int lockid;
    uint8_t crcLower;
    uint8_t crcUpper;
} dev_cogManager_channelData_S;

typedef struct
{
    dev_cogManager_channelData_S channels[DEV_COGMANAGER_CHANNEL_COUNT];
    int lock;
} dev_cogManager_data_S;

/**********************************************************************
 * External Variables
 **********************************************************************/
extern dev_cogManager_config_S dev_cogManager_config;
/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
static dev_cogManager_data_S dev_cogManager_data;
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

dev_cogManager_state_E dev_cogManager_getDesiredState(dev_cogManager_channel_E channel)
{
    dev_cogManager_state_E desiredState = dev_cogManager_data.channels[channel].state;
    switch (dev_cogManager_data.channels[channel].state)
    {
    case DEV_COGMANAGER_STATE_INIT:
        desiredState = DEV_COGMANAGER_STATE_INITIALIZE;
        break;
    case DEV_COGMANAGER_STATE_INITIALIZE:
        if (dev_cogManager_data.channels[channel].lockid == -1)
        {
            desiredState = DEV_COGMANAGER_STATE_ERROR;
            DEBUG_ERROR("Lock not created for channel %d\n", channel);
        }
        else
        {
            desiredState = DEV_COGMANAGER_STATE_BOOT;
        }
        break;
    case DEV_COGMANAGER_STATE_BOOT:
        if (dev_cogManager_data.channels[channel].cogid != -1)
        {
            desiredState = DEV_COGMANAGER_STATE_RUNNING;
        }
        break;
    case DEV_COGMANAGER_STATE_RUNNING:
        break;
    case DEV_COGMANAGER_STATE_ERROR:
        break;
    default:
        break;
    }
    return desiredState;
}

void dev_cogManager_entryAction(dev_cogManager_channel_E channel)
{
    switch (dev_cogManager_data.channels[channel].state)
    {
    case DEV_COGMANAGER_STATE_INIT:
        break;
    case DEV_COGMANAGER_STATE_INITIALIZE:
        dev_cogManager_data.channels[channel].lockid = _locknew();
        dev_cogManager_config.channels[channel].cogFunctionInit(dev_cogManager_data.channels[channel].lockid);
        break;
    case DEV_COGMANAGER_STATE_BOOT:
        dev_cogManager_data.channels[channel].cogid = _cogstart(dev_cogManager_config.channels[channel].cogFunctionRun,
                                                                NULL,
                                                                dev_cogManager_config.channels[channel].stack,
                                                                dev_cogManager_config.channels[channel].stackSize);
        break;
    case DEV_COGMANAGER_STATE_RUNNING:
        break;
    case DEV_COGMANAGER_STATE_ERROR:
        break;
    default:
        break;
    }
}

void dev_cogManager_exitAction(dev_cogManager_channel_E channel)
{
    switch (dev_cogManager_data.channels[channel].state)
    {
    case DEV_COGMANAGER_STATE_INIT:
        break;
    case DEV_COGMANAGER_STATE_INITIALIZE:
        break;
    case DEV_COGMANAGER_STATE_BOOT:
        break;
    case DEV_COGMANAGER_STATE_RUNNING:
        break;
    case DEV_COGMANAGER_STATE_ERROR:
        break;
    default:
        break;
    }
}

void dev_cogManager_runAction(dev_cogManager_channel_E channel)
{
    switch (dev_cogManager_data.channels[channel].state)
    {
    case DEV_COGMANAGER_STATE_INIT:
        break;
    case DEV_COGMANAGER_STATE_INITIALIZE:
        break;
    case DEV_COGMANAGER_STATE_BOOT:
        break;
    case DEV_COGMANAGER_STATE_RUNNING:
    {
        uint8_t crcLower = crc8(&dev_cogManager_config.channels[channel].lowerCanary[0], DEV_COGMANAGER_STACK_CANARY_SIZE);
        uint8_t crcUpper = crc8(&dev_cogManager_config.channels[channel].upperCanary[0], DEV_COGMANAGER_STACK_CANARY_SIZE);
        if (crcLower != dev_cogManager_data.channels[channel].crcLower)
        {
            DEBUG_ERROR("Stack overflow detected on channel %d\n", channel);
            // dev_cogManager_data.channels[channel].state = DEV_COGMANAGER_STATE_ERROR;
        }
        else if (crcUpper != dev_cogManager_data.channels[channel].crcUpper)
        {
            DEBUG_ERROR("Stack underflow detected on channel %d\n", channel);
            // dev_cogManager_data.channels[channel].state = DEV_COGMANAGER_STATE_ERROR;
        }
        else
        {
            // DEBUG_ERROR("Stack OK on channel %d: %d == %d\n", channel, crcLower, dev_cogManager_data.channels[channel].crcLower);
        }
    }
    break;
    case DEV_COGMANAGER_STATE_ERROR:
        break;
    default:
        break;
    }
}
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
void dev_cogManager_init(int lock)
{
    dev_cogManager_data.lock = lock;
    for (dev_cogManager_channel_E channel = (dev_cogManager_channel_E)0U; channel < DEV_COGMANAGER_CHANNEL_COUNT; channel++)
    {
        dev_cogManager_data.channels[channel].state = DEV_COGMANAGER_STATE_INIT;
        dev_cogManager_data.channels[channel].cogid = -1;
        dev_cogManager_data.channels[channel].crcLower = crc8(dev_cogManager_config.channels[channel].lowerCanary, DEV_COGMANAGER_STACK_CANARY_SIZE);
        dev_cogManager_data.channels[channel].crcUpper = crc8(dev_cogManager_config.channels[channel].upperCanary, DEV_COGMANAGER_STACK_CANARY_SIZE);
    }
}

void dev_cogManager_run(void)
{
    for (dev_cogManager_channel_E channel = (dev_cogManager_channel_E)0U; channel < DEV_COGMANAGER_CHANNEL_COUNT; channel++)
    {
        dev_cogManager_state_E currentState = dev_cogManager_data.channels[channel].state;
        dev_cogManager_state_E desiredState = dev_cogManager_getDesiredState(channel);
        if (desiredState != currentState)
        {
            dev_cogManager_exitAction(channel);
            dev_cogManager_data.channels[channel].state = desiredState;
            dev_cogManager_entryAction(channel);
        }
        dev_cogManager_runAction(channel);
    }
}
/**********************************************************************
 * End of File
 **********************************************************************/
