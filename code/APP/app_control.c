//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "app_control.h"
#include "app_monitor.h"
#include "app_messageSlave.h"

#include "dev_nvram.h"
#include "dev_cogManager.h"
#include "watchdog.h"

#include "HAL_GPIO.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/
#define APP_CONTROL_LOCK_REQ() _locktry(app_control_data.lock)
#define APP_CONTROL_LOCK_REQ_BLOCK()        \
    while (APP_CONTROL_LOCK_REQ() == false) \
        ;
#define APP_CONTROL_LOCK_REL() _lockrel(app_control_data.lock)
/**********************************************************************
 * Typedefs
 **********************************************************************/
typedef enum
{
    APP_CONTROL_ESD_NONE,
    APP_CONTROL_ESD_POWER,
    APP_CONTROL_ESD_UPPER,
    APP_CONTROL_ESD_LOWER,
    APP_CONTROL_ESD_SWITCH,
    APP_CONTROL_ESD_COUNT,
} app_control_ESDChain_E;

typedef struct
{
    int32_t forceMN;
    int32_t positionUM;
    MachineProfile machineProfile;
    bool motionReqEnabled;
    bool motionReqTest;
} app_control_input_S;

typedef struct
{
    bool selfCheckError;
    bool watchdogError;
    app_control_ESDChain_E ESDChainBroken;
    bool servoError;
    bool forceGaugeError;
} app_control_machineCheckData_S;

typedef struct
{
    bool sampleLengthError;
    bool sampleForceError;
    bool machineTensionError;
    bool machineCompressionError;
    bool machineUpperEndstopError;
    bool machineLowerEndstopError;
    bool machineDoorError;
} app_control_motionData_S;

typedef struct
{
    bool motionEnabled;
    bool testRunning;
} app_control_output_S;

typedef struct
{
    app_control_input_S input;
    app_control_machineCheckData_S machineCheckData;
    bool machineCheckError;
    app_control_motionData_S motionData;
    app_control_state_E state;

    int32_t lock;
} app_control_data_S;
/**********************************************************************
 * External Variables
 **********************************************************************/

/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
static app_control_data_S app_control_data;
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

void app_control_private_processInputs(void);
void app_control_private_processMachineCheck(void);
app_control_state_E app_control_private_getDesiredState(void);

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

void app_control_private_processInputs(void)
{
    app_control_data.input.forceMN = app_monitor_getForce();
    app_control_data.input.positionUM = app_monitor_getPosition();
    app_control_data.input.motionReqEnabled = app_messageSlave_requestMotionEnabled();
    app_control_data.input.motionReqTest = app_messageSlave_requestTestMode();
}

void app_control_private_processMachineCheck(void)
{
    app_control_data.machineCheckData.selfCheckError = dev_cogManager_isAllRunning();
    app_control_data.machineCheckData.watchdogError = watchdog_isAllAlive();
    if (HAL_GPIO_getActive(HAL_GPIO_ESD_POWER))
    {
        app_control_data.machineCheckData.ESDChainBroken = APP_CONTROL_ESD_POWER;
    }
    else if (HAL_GPIO_getActive(HAL_GPIO_ESD_SWITCH))
    {
        app_control_data.machineCheckData.ESDChainBroken = APP_CONTROL_ESD_SWITCH;
    }
    else if (HAL_GPIO_getActive(HAL_GPIO_ESD_UPPER))
    {
        app_control_data.machineCheckData.ESDChainBroken = APP_CONTROL_ESD_UPPER;
    }
    else if (HAL_GPIO_getActive(HAL_GPIO_ESD_LOWER))
    {
        app_control_data.machineCheckData.ESDChainBroken = APP_CONTROL_ESD_LOWER;
    }
    else
    {
        app_control_data.machineCheckData.ESDChainBroken = APP_CONTROL_ESD_NONE;
    }
    app_control_data.machineCheckData.servoError = false;      // TODO
    app_control_data.machineCheckData.forceGaugeError = false; // TODO

    app_control_data.machineCheckError = app_control_data.machineCheckData.selfCheckError ||
                                         app_control_data.machineCheckData.watchdogError ||
                                         (app_control_data.machineCheckData.ESDChainBroken != APP_CONTROL_ESD_NONE) ||
                                         app_control_data.machineCheckData.servoError ||
                                         app_control_data.machineCheckData.forceGaugeError;
}

app_control_state_E app_control_private_getDesiredState(void)
{
    app_control_state_E desiredState = app_control_data.state;
    if (app_control_data.machineCheckError)
    {
        desiredState = APP_CONTROL_STATE_MACHINE_CHECK;
    }
    else
    {
        switch (app_control_data.state)
        {
        case APP_CONTROL_STATE_MACHINE_CHECK:
            desiredState = APP_CONTROL_STATE_MOTION_MANUAL;
            break;
        case APP_CONTROL_STATE_MOTION_DISABLED:
            if (app_control_data.input.motionReqEnabled)
            {
                desiredState = APP_CONTROL_STATE_MOTION_MANUAL;
            }
            break;
        case APP_CONTROL_STATE_MOTION_MANUAL:
            if (app_control_data.input.motionReqEnabled == false)
            {
                desiredState = APP_CONTROL_STATE_MOTION_DISABLED;
            }
            else if (app_control_data.input.motionReqTest)
            {
                desiredState = APP_CONTROL_STATE_MOTION_TEST;
            }
            break;
        case APP_CONTROL_STATE_MOTION_TEST:
            if (app_control_data.input.motionReqEnabled == false)
            {
                desiredState = APP_CONTROL_STATE_MOTION_DISABLED;
            }
            else if (app_control_data.input.motionReqTest == false)
            {
                desiredState = APP_CONTROL_STATE_MOTION_MANUAL;
            }
        }
    }
}
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void app_control_init(int lock)
{
    app_control_data.lock = lock;
    app_control_data.state = APP_CONTROL_STATE_MACHINE_CHECK;
}

void app_control_run()
{
    app_control_private_processInputs();
    app_control_private_processMachineCheck();
    app_control_state_E desiredState = app_control_private_getDesiredState();
    if (desiredState != app_control_data.state)
    {
        app_control_data.state = desiredState;
    }
}

/**********************************************************************
 * End of File
 **********************************************************************/
