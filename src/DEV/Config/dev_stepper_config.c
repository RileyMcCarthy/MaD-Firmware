//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "dev_stepper_config.h"
#include "dev_stepper.h"
#include "HW_pins.h"
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
const dev_stepper_channelConfig_S dev_stepper_channelConfig[DEV_STEPPER_CHANNEL_COUNT] = {
    {
        HW_PIN_SERVO_ENA,
        HW_PIN_SERVO_PUL,
        HW_PIN_SERVO_DIR,
        HAL_PULSE_OUT_CHANNEL_SERVO,
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
