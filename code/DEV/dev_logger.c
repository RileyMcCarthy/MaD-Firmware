//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "propeller2.h"
#include <string.h>
#include "Debug.h"

#include "dev_logger.h"
#include "lib_staticQueue.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/
#define DEV_LOGGER_LOCK_REQ() _locktry(dev_logger_data.lock)
#define DEV_LOGGER_REQ_BLOCK()             \
    while (DEV_LOGGER_LOCK_REQ() == false) \
        ;
#define DEV_LOGGER_LOCK_REL() _lockrel(dev_logger_data.lock)
/**********************************************************************
 * Typedefs
 **********************************************************************/
typedef struct
{
    bool requestEnabled;
    char fileName[255];
} dev_logger_channelInput_S;

typedef struct
{
    dev_logger_channelInput_S stagedInput;
    dev_logger_channelInput_S input;

    bool enabled;
    bool queueEmpty;
    bool queueFull;
    FILE *file;

    dev_logger_state_E state;
    lib_staticQueue_S queue;
} dev_logger_channelData_S;

typedef struct
{
    dev_logger_channelData_S channelData[DEV_LOGGER_CHANNEL_COUNT];
    int32_t lock;
} dev_logger_data_S;

/**********************************************************************
 * External Variables
 **********************************************************************/
extern dev_logger_config_S dev_logger_config;
/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
static dev_logger_data_S dev_logger_data;
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

static dev_logger_state_E dev_logger_getDesiredState(dev_logger_channel_E channel)
{
    dev_logger_state_E desiredState = dev_logger_data.channelData[channel].state;
    switch (dev_logger_data.channelData[channel].state)
    {
    case DEV_LOGGER_STATE_INIT:
        if (dev_logger_data.channelData[channel].input.requestEnabled)
        {
            desiredState = DEV_LOGGER_STATE_OPEN;
        }
        break;
    case DEV_LOGGER_STATE_OPEN:
        if (dev_logger_data.channelData[channel].file != NULL)
        {
            desiredState = DEV_LOGGER_STATE_WAITING;
            DEBUG_INFO("DEV_LOGGER: Opened file %s\n", dev_logger_data.channelData[channel].input.fileName);
        }
        else
        {
            desiredState = DEV_LOGGER_STATE_INIT;
            DEBUG_ERROR("DEV_LOGGER: Failed to open file %s, make sure the directory exists\n", dev_logger_data.channelData[channel].input.fileName);
        }
        break;
    case DEV_LOGGER_STATE_WAITING:
        if (dev_logger_data.channelData[channel].queueEmpty == false)
        {
            desiredState = DEV_LOGGER_STATE_WRITE_DATA;
        }
        else if (dev_logger_data.channelData[channel].input.requestEnabled == false)
        {
            // Queue is empty and logging is disabled
            desiredState = DEV_LOGGER_STATE_CLOSE;
        }
        else
        {
            // wait for data
        }
        break;
    case DEV_LOGGER_STATE_WRITE_DATA:
        desiredState = DEV_LOGGER_STATE_WAITING;
        break;
    case DEV_LOGGER_STATE_CLOSE:
        desiredState = DEV_LOGGER_STATE_INIT;
        break;
    default:
        break;
    }
    return desiredState;
}

static void dev_logger_private_entryAction(dev_logger_channel_E channel)
{
    switch (dev_logger_data.channelData[channel].state)
    {
    case DEV_LOGGER_STATE_INIT:
        dev_logger_stop(channel);
        DEBUG_INFO("%s", "DEV_LOGGER: Initializing\n");
        break;
    case DEV_LOGGER_STATE_OPEN:
        DEBUG_INFO("DEV_LOGGER: Opening file %s\n", dev_logger_data.channelData[channel].input.fileName);
        DEBUG_INFO("DEV_LOGGER: Write type %s\n", dev_logger_config.channelConfig[channel].writeType);
        dev_logger_data.channelData[channel].file = fopen(dev_logger_data.channelData[channel].input.fileName, dev_logger_config.channelConfig[channel].writeType);
        break;
    case DEV_LOGGER_STATE_WAITING:
        break;
    case DEV_LOGGER_STATE_WRITE_DATA:
        // DEBUG_INFO("DEV_LOGGER: Writing data to file %s\n", dev_logger_data.channelData[channel].input.fileName);
        dev_logger_config.channelConfig[channel].format(dev_logger_data.channelData[channel].file, &dev_logger_data.channelData[channel].queue);
        break;
    case DEV_LOGGER_STATE_CLOSE:
        DEBUG_INFO("DEV_LOGGER: Closing file %s\n", dev_logger_data.channelData[channel].input.fileName);
        fclose(dev_logger_data.channelData[channel].file);
        dev_logger_data.channelData[channel].file = NULL;
        break;
    default:
        break;
    }
}

static void dev_logger_private_exitAction(dev_logger_channel_E channel)
{
    switch (dev_logger_data.channelData[channel].state)
    {
    case DEV_LOGGER_STATE_INIT:
        break;
    case DEV_LOGGER_STATE_OPEN:
        break;
    case DEV_LOGGER_STATE_WAITING:
        break;
    case DEV_LOGGER_STATE_WRITE_DATA:
        break;
    case DEV_LOGGER_STATE_CLOSE:
        break;
    default:
        break;
    }
}

static void dev_logger_private_stageInputs(dev_logger_channel_E channel)
{
    DEV_LOGGER_REQ_BLOCK();
    memcpy(&dev_logger_data.channelData[channel].input, &dev_logger_data.channelData[channel].stagedInput, sizeof(dev_logger_channelInput_S));
    DEV_LOGGER_LOCK_REL();
    dev_logger_data.channelData[channel].queueEmpty = lib_staticQueue_isempty(&dev_logger_data.channelData[channel].queue);
    dev_logger_data.channelData[channel].queueFull = lib_staticQueue_isfull(&dev_logger_data.channelData[channel].queue);
}

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
void dev_logger_init(int lock)
{
    dev_logger_data.lock = lock;
    for (dev_logger_channel_E channel = (dev_logger_channel_E)0U; channel < DEV_LOGGER_CHANNEL_COUNT; channel++)
    {
        dev_logger_data.channelData[channel].state = DEV_LOGGER_STATE_INIT;
        lib_staticQueue_init_lock(&dev_logger_data.channelData[channel].queue,
                                  dev_logger_config.channelConfig[channel].queueBuffer,
                                  dev_logger_config.channelConfig[channel].queueBufferSize,
                                  dev_logger_config.channelConfig[channel].queueBufferItemSize,
                                  dev_logger_data.lock);
    }
}

void dev_logger_run(void)
{
    for (dev_logger_channel_E channel = (dev_logger_channel_E)0U; channel < DEV_LOGGER_CHANNEL_COUNT; channel++)
    {
        dev_logger_private_stageInputs(channel);
        dev_logger_state_E desiredState = dev_logger_getDesiredState(channel);
        if (dev_logger_data.channelData[channel].state != desiredState)
        {
            // DEBUG_INFO("Transitioning from %d -> %d\n", dev_logger_data.channelData[channel].state, desiredState);
            dev_logger_private_exitAction(channel);
            dev_logger_data.channelData[channel].state = desiredState;
            dev_logger_private_entryAction(channel);
        }
    }
}

bool dev_logger_start(dev_logger_channel_E channel, const char *fileName)
{
    bool success = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        DEV_LOGGER_REQ_BLOCK();
        dev_logger_data.channelData[channel].stagedInput.requestEnabled = true;
        snprintf(dev_logger_data.channelData[channel].stagedInput.fileName, sizeof(dev_logger_data.channelData[channel].stagedInput.fileName), dev_logger_config.channelConfig[channel].nameFormat, fileName);
        DEBUG_INFO("DEV_LOGGER: Starting channel %d with file %s\n", channel, dev_logger_data.channelData[channel].stagedInput.fileName);
        success = true;
        DEV_LOGGER_LOCK_REL();
    }
    return success;
}

bool dev_logger_append(dev_logger_channel_E channel, dev_logger_channel_E channelToAppend)
{
    bool success = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        DEV_LOGGER_REQ_BLOCK();
        const bool loggingDisabled = dev_logger_data.channelData[channelToAppend].stagedInput.requestEnabled == false;
        if (loggingDisabled)
        {
            strncpy(dev_logger_data.channelData[channel].stagedInput.fileName, dev_logger_data.channelData[channelToAppend].stagedInput.fileName, sizeof(dev_logger_data.channelData[channel].stagedInput.fileName) - 1);
            DEBUG_INFO("DEV_LOGGER: Appending channel %d with file %s\n", channel, dev_logger_data.channelData[channel].stagedInput.fileName);
            dev_logger_data.channelData[channel].stagedInput.requestEnabled = true;
            success = true;
        }
        DEV_LOGGER_LOCK_REL();
    }
    return success;
}

bool dev_logger_stop(dev_logger_channel_E channel)
{
    bool success = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        DEV_LOGGER_REQ_BLOCK();
        dev_logger_data.channelData[channel].stagedInput.requestEnabled = false;
        success = true;
        DEV_LOGGER_LOCK_REL();
    }
    return success;
}

bool dev_logger_push(dev_logger_channel_E channel, void *data, uint32_t size)
{
    bool success = false;
    if ((channel < DEV_LOGGER_CHANNEL_COUNT) && (size == dev_logger_config.channelConfig[channel].queueBufferItemSize))
    {
        success = lib_staticQueue_push(&dev_logger_data.channelData[channel].queue, data);
    }
    return success;
}

bool dev_logger_isEmpty(dev_logger_channel_E channel)
{
    bool isEmpty = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        isEmpty = lib_staticQueue_isempty(&dev_logger_data.channelData[channel].queue);
    }
    return isEmpty;
}

/**********************************************************************
 * End of File
 **********************************************************************/
