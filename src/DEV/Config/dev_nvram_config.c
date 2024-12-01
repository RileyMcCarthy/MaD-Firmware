//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdlib.h>
#include "dev_nvram.h"
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
 * Variable Definitions
 **********************************************************************/

DEV_NVRAM_CHANNEL_DATA_CREATE(MachineProfile) = {
    "Default", // name
    {
        "DYN4",    // motorType
        0,         // maxMotorTorque
        0,         // maxMotorRPM
        0,         // gearDiameter
        0,         // gearPitch
        0,         // systemIntertia
        0,         // staticTorque
        0,         // load
        "Encoder", // positionEncoderType
        0,         // encoderStepsPermm
        0,         // servoStepPermm
        "DS2",     // forceGauge
        0,         // forceGaugeGain
        0,         // forceGaugeOffset
    },
    {
        0, // minPosition
        0, // maxPosition
        0, // maxVelocity
        0, // maxAcceleration
        0, // maxForceTensile
        0, // maxForceCompression
        0, // forceGaugeNeutralOffset
    },
};

// should rename to have prefix like lib or app or io etc
const dev_nvram_config_t dev_nvram_config = {
    {DEV_NVRAM_CHANNEL_CONFIG_CREATE(MachineProfile, SD_CARD_MOUNT_PATH "MachineProfile.json", true)},
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
