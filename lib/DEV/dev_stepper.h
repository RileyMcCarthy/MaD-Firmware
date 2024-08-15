#ifndef DEV_STEPPER_H
#define DEV_STEPPER_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdbool.h>
#include <stdint.h>

#include "dev_stepper_config.h"
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
    DEV_STEPPER_STATE_INIT,
    DEV_STEPPER_STATE_STOPPED,
    DEV_STEPPER_STATE_MOVING_CW,
    DEV_STEPPER_STATE_MOVING_CCW,
    DEV_STEPPER_STATE_MOVING_SLOW_CW,
    DEV_STEPPER_STATE_MOVING_SLOW_CCW,
    DEV_STEPPER_STATE_COUNT,
} dev_stepper_state_E;

typedef struct
{
    const uint8_t pinEnable;
    const uint8_t pinStep;
    const uint8_t pinDirection;
} dev_stepper_channelConfig_S;
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
void dev_stepper_init(int lock);
void dev_stepper_run();

bool dev_stepper_move(dev_stepper_channel_E ch, int32_t targetSteps, uint32_t stepsPerSecond);

int32_t dev_stepper_getSteps(dev_stepper_channel_E ch);
dev_stepper_state_E dev_stepper_getState(dev_stepper_channel_E ch);
int32_t dev_stepper_getTarget(dev_stepper_channel_E ch);
void dev_stepper_enable(dev_stepper_channel_E ch, bool enabled);
/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* DEV_STEPPER_H */
