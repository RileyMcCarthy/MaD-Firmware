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

/**********************************************************************
 * External Variables
 **********************************************************************/

/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/

DEV_LOGGER_CHANNEL_DATA_DEFINE(SAMPLE_DATA, dev_logger_testSample_S, 64, "a+", SD_CARD_MOUNT_PATH "/tests/%s.csv")
{
    bool result = false;
    dev_logger_testSample_S sample;
    if (lib_staticQueue_pop(queue, &sample))
    {
        fprintf(file, "%d,%d,%d,%d\n", sample.time, sample.index, sample.force, sample.position);
        result = true;
    }
    return result;
}

DEV_LOGGER_CHANNEL_DATA_DEFINE(SAMPLE_DATA_HEADER, dev_logger_testSampleHeader_S, 2, "w", SD_CARD_MOUNT_PATH "/test/%s.csv")
{
    bool result = false;
    dev_logger_testSampleHeader_S header;
    if (lib_staticQueue_pop(queue, &header))
    {
        fprintf(file, "%s\n", header.header);
        fprintf(file, "time, index, force, position\n");
        result = true;
    }
    return result;
}

dev_logger_config_S dev_logger_config = {
    {
        DEV_LOGGER_CHANNEL_CREATE(SAMPLE_DATA, false),
        DEV_LOGGER_CHANNEL_CREATE(SAMPLE_DATA_HEADER, true),
    },
};

/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

/**********************************************************************
 * End of File
 **********************************************************************/
