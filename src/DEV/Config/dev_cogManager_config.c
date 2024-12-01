//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdlib.h>

#include "dev_cogManager_config.h"
#include "dev_cogManager.h"
#include "app_monitor.h"
#include "app_motion.h"
#include "app_notification.h"
#include "app_messageSlave.h"

#include "IO_logger.h"
#include "dev_stepper.h"
#include "dev_forceGauge.h"
#include "ControlSystem.h"

#include "IO_protocol.h"

#include "watchdog.h"
#include "IO_Debug.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/
// technically cog0 (wardware cog) is not a channel with stack. It is a special case. should not be handled by the cogManager
DEV_COGMANAGER_CHANNEL_CREATE_INIT(MONITOR, 1024U)
{
    DEBUG_INFO("%s", "Monitor cog init\n");
    app_monitor_init(lock);
}

DEV_COGMANAGER_CHANNEL_CREATE_RUN(MONITOR)
{
    DEBUG_INFO("%s", "Monitor cog running\n");
    while (1)
    {
        app_monitor_run();
        watchdog_kick(WATCHDOG_CHANNEL_MONITOR);
    }
}

DEV_COGMANAGER_CHANNEL_CREATE_INIT(MOTOR, 1024U)
{
    DEBUG_INFO("%s", "Stepper cog initializing\n");
    dev_stepper_init(lock);
}

DEV_COGMANAGER_CHANNEL_CREATE_RUN(MOTOR)
{
    DEBUG_INFO("%s", "Stepper cog running\n");
    while (1)
    {
        dev_stepper_run();
        watchdog_kick(WATCHDOG_CHANNEL_MOTOR);
    }
}

DEV_COGMANAGER_CHANNEL_CREATE_INIT(COMMUNICATION, 1024)
{
    DEBUG_INFO("%s", "Communication cog initializing\n");
    IO_protocol_init();
    app_messageSlave_init(lock);
    app_notification_init(lock);
    app_motion_init(lock); // temp spot until control is fixed
}

DEV_COGMANAGER_CHANNEL_CREATE_RUN(COMMUNICATION)
{
    DEBUG_INFO("%s", "Communication cog running\n");
    while (1)
    {
        app_notification_run();
        app_messageSlave_run();
        app_motion_run();
        watchdog_kick(WATCHDOG_CHANNEL_COMMUNICATION);
    }
}

DEV_COGMANAGER_CHANNEL_CREATE_INIT(CONTROL, 1024)
{
    DEBUG_INFO("%s", "Control cog initializing\n");
}

DEV_COGMANAGER_CHANNEL_CREATE_RUN(CONTROL)
{
    DEBUG_INFO("%s", "Control cog running\n");
    control_cog_run();
}

DEV_COGMANAGER_CHANNEL_CREATE_INIT(LOGGER, 1024)
{
    DEBUG_INFO("%s", "Logger cog initializing\n");
    IO_logger_init(lock);
}

DEV_COGMANAGER_CHANNEL_CREATE_RUN(LOGGER)
{
    DEBUG_INFO("%s", "Logger cog running\n");
    while (1)
    {
        IO_logger_run();
        watchdog_kick(WATCHDOG_CHANNEL_LOGGER);
    }
}

DEV_COGMANAGER_CHANNEL_CREATE_INIT(FORCEGAUGE, 1024)
{
    DEBUG_INFO("%s", "Force gauge cog initializing\n");
    dev_forceGauge_init(lock);
}

DEV_COGMANAGER_CHANNEL_CREATE_RUN(FORCEGAUGE)
{
    DEBUG_INFO("%s", "Force gauge cog running\n");
    while (1)
    {
        dev_forceGauge_run();
        watchdog_kick(WATCHDOG_CHANNEL_FORCEGAUGE);
    }
}

const dev_cogManager_config_S dev_cogManager_config = {
    {
        DEV_COGMANAGER_CHANNEL_CONFIG_CREATE(MONITOR),
        DEV_COGMANAGER_CHANNEL_CONFIG_CREATE(MOTOR),
        DEV_COGMANAGER_CHANNEL_CONFIG_CREATE(COMMUNICATION),
        DEV_COGMANAGER_CHANNEL_CONFIG_CREATE(CONTROL),
        DEV_COGMANAGER_CHANNEL_CONFIG_CREATE(LOGGER),
        DEV_COGMANAGER_CHANNEL_CONFIG_CREATE(FORCEGAUGE),
    },
};
/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/

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
