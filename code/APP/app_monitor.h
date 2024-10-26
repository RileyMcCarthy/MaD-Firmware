#ifndef APP_MONITOR_H
#define APP_MONITOR_H
//
// Created by Riley McCarthy on 25/04/24.
// @brief This module is responsible for aggregating sensor measurements (time, position, force).
// This module is thread-safe.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdbool.h>
#include <stdint.h>
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
    APP_MONITOR_INIT,
    APP_MONITOR_READY,
    APP_MONITOR_LOGGING,
    APP_MONITOR_COUNT,
} app_monitor_state_t;

typedef struct
{
    int32_t force;    // mN
    int32_t position; // um
    uint32_t time;    // us
    uint32_t index;   // sample id, should determine overflow at 1000sps
    int32_t setpoint; // um
} app_monitor_sample_t;

typedef enum
{
    APP_MONITOR_LOGGING_STATE_IDLE,
    APP_MONITOR_LOGGING_STATE_RUNNING,
    APP_MONITOR_LOGGING_STATE_STOPPING,
    APP_MONITOR_LOGGING_STATE_COUNT,
} app_monitor_loggingState_E;

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void app_monitor_init(int lock);
void app_monitor_run(void);

void app_monitor_getSample(app_monitor_sample_t *sample);
int32_t app_monitor_getForce(void);
int32_t app_monitor_getPosition(void);

void app_monitor_setGaugeLength(void);
void app_monitor_setGaugeForce(void);

/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* APP_MONITOR_H */
