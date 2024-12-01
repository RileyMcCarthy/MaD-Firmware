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
#define DEV_LOGGER_COMMENT_SIZE 512
/*********************************************************************
 * Macros
 **********************************************************************/
#define DEV_LOGGER_LOCK_REQ() _locktry(dev_logger_data.lock)
#define DEV_LOGGER_LOCK_REQ_BLOCK()        \
    while (DEV_LOGGER_LOCK_REQ() == false) \
    {                                      \
    }
#define DEV_LOGGER_LOCK_REL() _lockrel(dev_logger_data.lock)

#define DEV_LOGGER_LOCKED_INPUT(channel) dev_logger_data.channelData[channel].externalInput
#define DEV_LOGGER_INTERNAL_INPUT(channel) dev_logger_data.channelData[channel].input

#define DEV_LOGGER_LOCKED_REQUEST(channel) dev_logger_data.channelData[channel].externalRequest
#define DEV_LOGGER_INTERNAL_REQUEST(channel) dev_logger_data.channelData[channel].request
/**********************************************************************
 * Typedefs
 **********************************************************************/
typedef struct
{
    char fileName[255];
    char comment[DEV_LOGGER_COMMENT_SIZE];
} dev_logger_channelInput_S;

typedef struct
{
    bool enable;
    bool disable;
    bool writeComment;
} dev_logger_channelRequest_S;

typedef struct
{
    dev_logger_channelInput_S externalInput;
    dev_logger_channelInput_S input;

    dev_logger_channelRequest_S externalRequest;
    dev_logger_channelRequest_S request;

    bool writeComment;
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
        if (DEV_LOGGER_INTERNAL_REQUEST(channel).enable)
        {
            desiredState = DEV_LOGGER_STATE_OPEN;
        }
        break;
    case DEV_LOGGER_STATE_OPEN:
        if (dev_logger_data.channelData[channel].file != NULL)
        {
            desiredState = DEV_LOGGER_STATE_WAITING;
            DEBUG_INFO("DEV_LOGGER: Opened file %s\n", DEV_LOGGER_INTERNAL_INPUT(channel).fileName);
        }
        else
        {
            desiredState = DEV_LOGGER_STATE_INIT;
            DEBUG_ERROR("DEV_LOGGER: Failed to open file %s, make sure the directory exists\n", DEV_LOGGER_INTERNAL_INPUT(channel).fileName);
        }
        break;
    case DEV_LOGGER_STATE_WAITING:
        if (dev_logger_data.channelData[channel].queueEmpty == false)
        {
            desiredState = DEV_LOGGER_STATE_WRITE_DATA;
        }
        else if (DEV_LOGGER_INTERNAL_REQUEST(channel).writeComment)
        {
            desiredState = DEV_LOGGER_STATE_WRITE_COMMENT;
        }
        else if (DEV_LOGGER_INTERNAL_REQUEST(channel).disable)
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
    case DEV_LOGGER_STATE_WRITE_COMMENT:
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
        DEBUG_INFO("%s", "DEV_LOGGER: Initializing\n");
        break;
    case DEV_LOGGER_STATE_OPEN:
        DEBUG_INFO("DEV_LOGGER: Opening file %s\n", DEV_LOGGER_INTERNAL_INPUT(channel).fileName);
        DEBUG_INFO("DEV_LOGGER: Write type %s\n", dev_logger_config.channelConfig[channel].writeType);
        dev_logger_data.channelData[channel].file = fopen(DEV_LOGGER_INTERNAL_INPUT(channel).fileName, dev_logger_config.channelConfig[channel].writeType);
        break;
    case DEV_LOGGER_STATE_WAITING:
        break;
    case DEV_LOGGER_STATE_WRITE_DATA:
        dev_logger_config.channelConfig[channel].format(dev_logger_data.channelData[channel].file, &dev_logger_data.channelData[channel].queue);
        break;
    case DEV_LOGGER_STATE_WRITE_COMMENT:
        DEBUG_INFO("DEV_LOGGER: Writing comment to file %s\n", DEV_LOGGER_INTERNAL_INPUT(channel).fileName);
        fprintf(dev_logger_data.channelData[channel].file, "# %s\n", DEV_LOGGER_INTERNAL_INPUT(channel).comment);
        break;
    case DEV_LOGGER_STATE_CLOSE:
        DEBUG_INFO("DEV_LOGGER: Closing file %s\n", DEV_LOGGER_INTERNAL_INPUT(channel).fileName);
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
        DEV_LOGGER_INTERNAL_REQUEST(channel).enable = false;
        break;
    case DEV_LOGGER_STATE_OPEN:
        break;
    case DEV_LOGGER_STATE_WAITING:
        break;
    case DEV_LOGGER_STATE_WRITE_DATA:
        break;
    case DEV_LOGGER_STATE_WRITE_COMMENT:
        DEV_LOGGER_INTERNAL_REQUEST(channel).writeComment = false;
        break;
    case DEV_LOGGER_STATE_CLOSE:
        DEV_LOGGER_INTERNAL_REQUEST(channel).disable = false;
        break;
    default:
        break;
    }
}

static void dev_logger_private_stageInputs(dev_logger_channel_E channel)
{
    DEV_LOGGER_LOCK_REQ_BLOCK();
    memcpy(&DEV_LOGGER_INTERNAL_INPUT(channel), &DEV_LOGGER_LOCKED_INPUT(channel), sizeof(dev_logger_channelInput_S));
    if (DEV_LOGGER_LOCKED_REQUEST(channel).writeComment)
    {
        DEV_LOGGER_INTERNAL_REQUEST(channel).writeComment = true;
        DEV_LOGGER_LOCKED_REQUEST(channel).writeComment = false;
    }
    if (DEV_LOGGER_LOCKED_REQUEST(channel).enable)
    {
        DEV_LOGGER_INTERNAL_REQUEST(channel).enable = true;
        DEV_LOGGER_LOCKED_REQUEST(channel).enable = false;
    }
    if (DEV_LOGGER_LOCKED_REQUEST(channel).disable)
    {
        DEV_LOGGER_INTERNAL_REQUEST(channel).enable = false;
        DEV_LOGGER_LOCKED_REQUEST(channel).disable = false;
    }
    DEV_LOGGER_LOCK_REL();

    // Static queue is thread safe
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

bool dev_logger_open(dev_logger_channel_E channel, const char *fileName)
{
    bool success = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        DEV_LOGGER_LOCK_REQ_BLOCK();
        DEV_LOGGER_LOCKED_REQUEST(channel).enable = true;
        snprintf(DEV_LOGGER_LOCKED_INPUT(channel).fileName, sizeof(DEV_LOGGER_LOCKED_INPUT(channel).fileName), dev_logger_config.channelConfig[channel].nameFormat, fileName);
        DEBUG_INFO("DEV_LOGGER: Starting channel %d with file %s\n", channel, DEV_LOGGER_LOCKED_INPUT(channel).fileName);
        success = true;
        DEV_LOGGER_LOCK_REL();
    }
    return success;
}

bool dev_logger_reopen(dev_logger_channel_E channel)
{
    bool success = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        DEV_LOGGER_LOCK_REQ_BLOCK();
        DEV_LOGGER_LOCKED_REQUEST(channel).enable = true;
        DEBUG_INFO("DEV_LOGGER: Reopening channel %d with file %s\n", channel, DEV_LOGGER_LOCKED_INPUT(channel).fileName);
        success = true;
        DEV_LOGGER_LOCK_REL();
    }
    return success;
}

bool dev_logger_addComment(dev_logger_channel_E channel, const char *comment, uint32_t size)
{
    bool success = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        if ((dev_logger_data.channelData[channel].writeComment == false) &&
            (size < DEV_LOGGER_COMMENT_SIZE))
        {
            DEV_LOGGER_LOCK_REQ_BLOCK();
            DEV_LOGGER_LOCKED_REQUEST(channel).writeComment = true;
            // could use a blocking function to save buffering memory
            strncpy(DEV_LOGGER_LOCKED_INPUT(channel).comment, comment, size);
            DEBUG_INFO("DEV_LOGGER: Adding comment to channel %d: %s\n", channel, DEV_LOGGER_LOCKED_INPUT(channel).comment);
            success = true;
            DEV_LOGGER_LOCK_REL();
        }
    }
    return success;
}

/*
 * @brief Stop logging to the file, complete writing the queue and close the file.
 */
bool dev_logger_close(dev_logger_channel_E channel)
{
    bool success = false;
    if (channel < DEV_LOGGER_CHANNEL_COUNT)
    {
        DEV_LOGGER_LOCK_REQ_BLOCK();
        DEV_LOGGER_LOCKED_REQUEST(channel).disable = true;
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
