#ifndef DEV_LOGGER_H
#define DEV_LOGGER_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdint.h>
#include <stdio.h>

#include "dev_logger_config.h"
#include "app_monitor.h"
#include "lib_staticQueue.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

#define DEV_LOGGER_CHANNEL_DATA_DEFINE(channel, type, bufferSize, writeTypeStr, nameformat) \
    static type dev_logger_##channel##_dataBuffer[bufferSize];                              \
    static const char dev_logger_##channel##_writeType[] = writeTypeStr;                    \
    static const char dev_logger_##channel##_nameFormat[] = nameformat;                     \
    static bool dev_logger_##channel##Format(FILE *file, lib_staticQueue_S *queue)

#define DEV_LOGGER_CHANNEL_CREATE(channel, singleshot)                                            \
    {                                                                                             \
        dev_logger_##channel##Format,                                                             \
        dev_logger_##channel##_writeType,                                                         \
        dev_logger_##channel##_dataBuffer,                                                        \
        sizeof(dev_logger_##channel##_dataBuffer) / sizeof(dev_logger_##channel##_dataBuffer[0]), \
        sizeof(dev_logger_##channel##_dataBuffer[0]),                                             \
        singleshot,                                                                               \
        dev_logger_##channel##_nameFormat,                                                        \
    }
/**********************************************************************
 * Typedefs
 **********************************************************************/
typedef enum
{
    DEV_LOGGER_STATE_INIT,       // no file opened
    DEV_LOGGER_STATE_OPEN,       // open or create file
    DEV_LOGGER_STATE_WAITING,    // waiting for data
    DEV_LOGGER_STATE_WRITE_DATA, // write data
    DEV_LOGGER_STATE_CLOSE,      // close file
} dev_logger_state_E;
// Need to write header that is sent from main program.
// might make struct for data or just use raw json
// for now make struct a json of 512 bytes or something

typedef struct
{
    bool (*format)(FILE *, lib_staticQueue_S *);
    const char *const writeType;
    void *const queueBuffer;
    const uint32_t queueBufferSize;
    const uint32_t queueBufferItemSize;
    const bool isSingleShot;
    const char *nameFormat;
} dev_logger_channelConfig_S;

typedef struct
{
    dev_logger_channelConfig_S channelConfig[DEV_LOGGER_CHANNEL_COUNT];
} dev_logger_config_S;
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void dev_logger_init(int lock);
void dev_logger_run(void);

bool dev_logger_start(dev_logger_channel_E channel, const char *fileName);
bool dev_logger_append(dev_logger_channel_E channel, dev_logger_channel_E channelToAppend);
bool dev_logger_stop(dev_logger_channel_E channel);
bool dev_logger_push(dev_logger_channel_E channel, void *data, uint32_t size);

/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* DEV_LOGGER_H */
