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

#define DEV_LOGGER_CHANNEL_CREATE(channel)                                                        \
    {                                                                                             \
        dev_logger_##channel##Format,                                                             \
        dev_logger_##channel##_writeType,                                                         \
        dev_logger_##channel##_dataBuffer,                                                        \
        sizeof(dev_logger_##channel##_dataBuffer) / sizeof(dev_logger_##channel##_dataBuffer[0]), \
        sizeof(dev_logger_##channel##_dataBuffer[0]),                                             \
        dev_logger_##channel##_nameFormat,                                                        \
    }
/**********************************************************************
 * Typedefs
 **********************************************************************/
typedef enum
{
    DEV_LOGGER_STATE_INIT,          // no file opened
    DEV_LOGGER_STATE_OPEN,          // open or create file
    DEV_LOGGER_STATE_WAITING,       // waiting for data
    DEV_LOGGER_STATE_WRITE_DATA,    // write data
    DEV_LOGGER_STATE_WRITE_COMMENT, // write comment
    DEV_LOGGER_STATE_CLOSE,         // close file
} dev_logger_state_E;

typedef struct
{
    bool (*format)(FILE *, lib_staticQueue_S *);
    const char *const writeType;
    void *const queueBuffer;
    const uint32_t queueBufferSize;
    const uint32_t queueBufferItemSize;
    const char *nameFormat;
} dev_logger_channelConfig_S;

typedef struct
{
    dev_logger_channelConfig_S channelConfig[DEV_LOGGER_CHANNEL_COUNT];
} dev_logger_config_S;
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
// HI FUTURE riley, main issue right now is solving how messageSlave will interact with the logger
// either addComment is blocking or we need way to close file after we are done with it
// I think blocking is best... but we need to make it fast.
void dev_logger_init(int lock);
void dev_logger_run(void);

// we have buffered logging + single shot comments (blocking)
bool dev_logger_open(dev_logger_channel_E channel, const char *fileName);
bool dev_logger_reopen(dev_logger_channel_E channel);
bool dev_logger_close(dev_logger_channel_E channel);
bool dev_logger_push(dev_logger_channel_E channel, void *data, uint32_t size);
bool dev_logger_isEmpty(dev_logger_channel_E channel);
bool dev_logger_addComment(dev_logger_channel_E channel, const char *comment, uint32_t size);
/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* DEV_LOGGER_H */
