#ifndef DEV_LOGGER_CONFIG_H
#define DEV_LOGGER_CONFIG_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "dev_logger.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef enum
{
    DEV_LOGGER_CHANNEL_SAMPLE_DATA,
    DEV_LOGGER_CHANNEL_SAMPLE_DATA_HEADER,
    DEV_LOGGER_CHANNEL_COUNT,
} dev_logger_channel_E;

typedef struct
{
    int32_t force;    // mN
    int32_t position; // um
    uint32_t time;    // us
    uint32_t index;   // sample id, should determine overflow at 1000sps
    int32_t setpoint; // um
} dev_logger_testSample_S;

typedef struct
{
    char header[255];
} dev_logger_testSampleHeader_S;

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* DEV_LOGGER_CONFIG_H */
