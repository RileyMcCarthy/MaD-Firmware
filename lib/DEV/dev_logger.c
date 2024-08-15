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
#include "IO_staticQueue.h"
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
    bool loggingEnabled;
    char fileName[255];
} dev_logger_channelInput_S;

typedef struct
{
    dev_logger_channelInput_S stagedInput;
    dev_logger_channelInput_S input;

    bool queueEmpty;
    bool queueFull;
    FILE *file;

    dev_logger_state_E state;
    IO_staticQueue queue;
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
        if (dev_logger_data.channelData[channel].input.loggingEnabled)
        {
            desiredState = DEV_LOGGER_STATE_OPEN;
        }
        break;
    case DEV_LOGGER_STATE_OPEN:
        if (dev_logger_data.channelData[channel].file != NULL)
        {
            desiredState = DEV_LOGGER_STATE_WAITING;
        }
        else
        {
            desiredState = DEV_LOGGER_STATE_INIT;
        }
        break;
    case DEV_LOGGER_STATE_WAITING:
        if (dev_logger_data.channelData[channel].input.loggingEnabled == false)
        {
            desiredState = DEV_LOGGER_STATE_CLOSE;
        }
        else if (dev_logger_data.channelData[channel].queueEmpty == false)
        {
            desiredState = DEV_LOGGER_STATE_WRITE_DATA;
        }
        else
        {
            // wait for data
        }
        break;
    case DEV_LOGGER_STATE_WRITE_DATA:
        if (dev_logger_config.channelConfig[channel].isSingleShot)
        {
            desiredState = DEV_LOGGER_STATE_CLOSE;
        }
        else
        {
            desiredState = DEV_LOGGER_STATE_WAITING;
        }
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
        if (dev_logger_data.channelData[channel].file == NULL)
        {
            DEBUG_INFO("DEV_LOGGER: Failed to open file %s\n", dev_logger_data.channelData[channel].input.fileName);
        }
        else
        {
            DEBUG_INFO("DEV_LOGGER: Opened file %s\n", dev_logger_data.channelData[channel].input.fileName);
        }
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
    dev_logger_data.channelData[channel].queueEmpty = IO_staticQueue_isempty(&dev_logger_data.channelData[channel].queue);
    dev_logger_data.channelData[channel].queueFull = IO_staticQueue_isfull(&dev_logger_data.channelData[channel].queue);
    DEV_LOGGER_LOCK_REL();
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
        IO_staticQueue_init_lock(&dev_logger_data.channelData[channel].queue,
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
            DEBUG_INFO("Transitioning from %d -> %d\n", dev_logger_data.channelData[channel].state, desiredState);
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
        dev_logger_data.channelData[channel].stagedInput.loggingEnabled = true;
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
        const bool fileNameEmpty = strncmp(dev_logger_data.channelData[channelToAppend].stagedInput.fileName, "", 255) == 0;
        const bool loggingEnabled = dev_logger_data.channelData[channelToAppend].stagedInput.loggingEnabled;
        if (loggingEnabled == false && fileNameEmpty == false)
        {
            strncpy(dev_logger_data.channelData[channel].stagedInput.fileName, dev_logger_data.channelData[channelToAppend].stagedInput.fileName, sizeof(dev_logger_data.channelData[channel].stagedInput.fileName) - 1);
            DEBUG_INFO("DEV_LOGGER: Appending channel %d with file %s\n", channel, dev_logger_data.channelData[channel].stagedInput.fileName);
            dev_logger_data.channelData[channel].stagedInput.loggingEnabled = true;
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
        dev_logger_data.channelData[channel].stagedInput.loggingEnabled = false;
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
        success = IO_staticQueue_push(&dev_logger_data.channelData[channel].queue, data);
    }
    return success;
}

/**********************************************************************
 * End of File
 **********************************************************************/
