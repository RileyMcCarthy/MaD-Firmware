//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <string.h>
#include <stdlib.h>
#include <propeller2.h>
#include <smartpins.h>
#include <math.h>

#include "app_motion.h"
#include "app_notification.h"

#include "dev_stepper.h"
#include "dev_nvram.h"

#include "lib_staticQueue.h"
#include "IO_digitalPin.h"
#include "lib_timer.h"

#include "Debug.h"
#include "StateMachine.h"
#include "watchdog.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/
#define MOTION_MANUAL_BUFFER_SIZE 100
#define MOTION_TEST_BUFFER_SIZE 100

#define APP_MOTION_LOCK_REQ() _locktry(app_motion_data.lock)
#define APP_MOTION_LOCK_REQ_BLOCK()        \
    while (APP_MOTION_LOCK_REQ() == false) \
        ;
#define APP_MOTION_LOCK_REL() _lockrel(app_motion_data.lock)
/**********************************************************************
 * Typedefs
 **********************************************************************/
// getters from other functions
typedef struct
{
    bool motionEnabled;
    MotionMode motionMode;
    bool hasManualMove;
    bool hasTestMove;
    int32_t positionSteps;
    bool atTarget;
} app_motion_dataInputs_t;

typedef struct
{
    int32_t setpoint; // um
    int32_t position; // um
} app_motion_outputs_t;

typedef struct
{
    app_motion_dataInputs_t inputs;
    lib_staticQueue_S testQueue;
    lib_staticQueue_S manualQueue;

    bool moveComplete;
    bool absoluteMode;
    lib_timer_S dwellTimer;
    int32_t stepsPerMM;
    app_motion_move_t currentMove;
    app_motion_state_E state;
    int lock;

    app_motion_outputs_t stagedOutputs;
    app_motion_outputs_t outputs;

    app_motion_move_t manualBuffer[MOTION_MANUAL_BUFFER_SIZE];
    app_motion_move_t testBuffer[MOTION_TEST_BUFFER_SIZE];
} app_motion_data_t;
/**********************************************************************
 * Variable Definitions
 **********************************************************************/
static app_motion_data_t app_motion_data;
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/
static void app_motion_private_processInputs();
static void app_motion_private_processOutputs();
static app_motion_state_E app_motion_private_getDesiredState();
static void app_motion_private_exitAction();
static void app_motion_private_entryAction();
static void app_motion_private_runAction();
static void app_motion_private_moveManager_entryAction(void);
static bool app_motion_private_moveManager_runAction(void);
/**********************************************************************
 * Private Functions
 **********************************************************************/

static void app_motion_private_processInputs()
{
    MachineState machineState;
    get_machine_state(&machineState);
    app_motion_data.inputs.motionEnabled = machineState.motionParameters.status == MOTIONSTATUS_ENABLED;
    app_motion_data.inputs.motionMode = machineState.motionParameters.mode;
    app_motion_data.inputs.positionSteps = dev_stepper_getSteps(DEV_STEPPER_CHANNEL_MAIN);
    app_motion_data.inputs.atTarget = dev_stepper_atTarget(DEV_STEPPER_CHANNEL_MAIN);
}

static void app_motion_private_processOutputs()
{
    // need memcmp to keep this cog from blocking if nothing has changed
    if (memcmp(&app_motion_data.outputs, &app_motion_data.stagedOutputs, sizeof(app_motion_outputs_t)) != 0)
    {
        APP_MOTION_LOCK_REQ_BLOCK();
        memcpy(&app_motion_data.outputs, &app_motion_data.stagedOutputs, sizeof(app_motion_outputs_t));
        APP_MOTION_LOCK_REL();
    }
}

static app_motion_state_E app_motion_private_getDesiredState()
{
    app_motion_state_E desiredState = app_motion_data.state;
    if (app_motion_data.inputs.motionEnabled == false)
    {
        desiredState = APP_MOTION_DISABLED;
    }
    else
    {
        switch (app_motion_data.state)
        {
        case APP_MOTION_INIT:
            desiredState = APP_MOTION_DISABLED;
            break;
        case APP_MOTION_DISABLED:
            if (app_motion_data.inputs.motionEnabled)
            {
                desiredState = APP_MOTION_WAITING;
            }
            break;
        case APP_MOTION_WAITING:
            if ((app_motion_data.inputs.motionMode == MODE_MANUAL) && lib_staticQueue_pop(&app_motion_data.manualQueue, &app_motion_data.currentMove))
            {
                desiredState = APP_MOTION_MOVING_MANUAL;
            }
            else if ((app_motion_data.inputs.motionMode == MODE_TEST_RUNNING) && lib_staticQueue_pop(&app_motion_data.testQueue, &app_motion_data.currentMove))
            {
                desiredState = APP_MOTION_MOVING_TEST;
            }
            break;
        case APP_MOTION_MOVING_MANUAL:
        case APP_MOTION_MOVING_TEST:
            if (app_motion_data.moveComplete)
            {
                desiredState = APP_MOTION_WAITING;
            }
            break;
        case APP_MOTION_COUNT:
        default:
            break;
        }
    }
    return desiredState;
}

static void app_motion_private_exitAction()
{
    switch (app_motion_data.state)
    {
    case APP_MOTION_INIT:
        break;
    case APP_MOTION_DISABLED:
        dev_stepper_enable(DEV_STEPPER_CHANNEL_MAIN, true);
        break;
    case APP_MOTION_WAITING:
        break;
    case APP_MOTION_MOVING_MANUAL:
    case APP_MOTION_MOVING_TEST:
        lib_timer_stop(&app_motion_data.dwellTimer);
        break;
    case APP_MOTION_COUNT:
        break;
    default:
        break;
    }
}

static void app_motion_private_entryAction()
{
    switch (app_motion_data.state)
    {
    case APP_MOTION_INIT:
        break;
    case APP_MOTION_DISABLED:
        dev_stepper_enable(DEV_STEPPER_CHANNEL_MAIN, false);
        break;
    case APP_MOTION_WAITING:
        break;
    case APP_MOTION_MOVING_MANUAL:
    case APP_MOTION_MOVING_TEST:
        app_motion_data.moveComplete = false;
        app_motion_private_moveManager_entryAction();
        break;
    case APP_MOTION_COUNT:
        break;
    default:
        break;
    }
}

static void app_motion_private_runAction()
{
    switch (app_motion_data.state)
    {
    case APP_MOTION_INIT:
        break;
    case APP_MOTION_DISABLED:
        break;
    case APP_MOTION_WAITING:
        break;
    case APP_MOTION_MOVING_MANUAL:
    case APP_MOTION_MOVING_TEST:
        app_motion_data.moveComplete = app_motion_private_moveManager_runAction();
        break;
    case APP_MOTION_COUNT:
        break;
    default:
        break;
    }
}

static void app_motion_private_moveManager_entryAction(void)
{
    DEBUG_INFO("Processing move: %d\n", app_motion_data.currentMove.g);
    switch (app_motion_data.currentMove.g)
    {
    case G0_RAPID_MOVE:
    case G1_LINEAR_MOVE:
        if (app_motion_data.currentMove.f == 0U)
        {
            DEBUG_WARNING("G0/G1 Command has zero feedrate: %d\n", app_motion_data.currentMove.f);
            return;
        }
        int32_t steps = (app_motion_data.currentMove.x * app_motion_data.stepsPerMM) / 1000;
        const int32_t feedrate = (app_motion_data.currentMove.f * app_motion_data.stepsPerMM) / 1000;
        if (app_motion_data.absoluteMode == false)
        {
            steps += app_motion_data.inputs.positionSteps;
        }
        DEBUG_INFO("G0 command moving to steps %d at %d steps/s", steps, feedrate);
        dev_stepper_move(DEV_STEPPER_CHANNEL_MAIN, steps, feedrate);
        break;
    case G2_CW_ARC_MOVE:
    case G3_CCW_ARC_MOVE:
    case G4_DWELL:
        DEBUG_INFO("G4 command pausing for %d ms", app_motion_data.currentMove.p);
        lib_timer_init(&app_motion_data.dwellTimer, app_motion_data.currentMove.p);
        lib_timer_start(&app_motion_data.dwellTimer);
        break;
    case G28_HOME:
    case G90_ABSOLUTE:
        DEBUG_INFO("%s", "Setting absolute mode\n");
        app_motion_data.absoluteMode = true;
        break;
    case G91_INCREMENTAL:
        DEBUG_INFO("%s", "Setting incremental mode\n");
        app_motion_data.absoluteMode = false;
        break;
    case G122_STOP:
        app_notification_send(APP_NOTIFICATION_TYPE_INFO, "%s", "Test Complete!");
        state_machine_set(PARAM_MOTION_MODE, MODE_TEST);
        break;
    default:
        break;
    }
}

static bool app_motion_private_moveManager_runAction(void)
{
    bool moveComplete = false;
    switch (app_motion_data.currentMove.g)
    {
    case G0_RAPID_MOVE:
    case G1_LINEAR_MOVE:
        if (app_motion_data.inputs.atTarget)
        {
            moveComplete = true;
        }
        break;
    case G2_CW_ARC_MOVE:
    case G3_CCW_ARC_MOVE:
    case G4_DWELL:
        if (lib_timer_expired(&app_motion_data.dwellTimer))
        {
            moveComplete = true;
        }
        break;
    case G28_HOME:
    case G90_ABSOLUTE:
    case G91_INCREMENTAL:
    case G122_STOP:
        moveComplete = true;
        break;
    default:
        break;
    }
    return moveComplete;
}

/**********************************************************************
 * Function Definitions
 **********************************************************************/

void app_motion_init(int lock)
{
    app_motion_data.lock = lock;
    app_motion_data.absoluteMode = true; // Default absolute cordinates
    MachineProfile machineProfile;
    dev_nvram_getChannelData(DEV_NVRAM_CHANNEL_MACHINE_PROFILE, &machineProfile, sizeof(MachineProfile));
    app_motion_data.stepsPerMM = machineProfile.configuration.servoStepPermm;
    (void)lib_staticQueue_init_lock(&app_motion_data.manualQueue, app_motion_data.manualBuffer, MOTION_MANUAL_BUFFER_SIZE, sizeof(app_motion_move_t), lock);
    (void)lib_staticQueue_init_lock(&app_motion_data.testQueue, app_motion_data.testBuffer, MOTION_TEST_BUFFER_SIZE, sizeof(app_motion_move_t), lock);
}

void app_motion_run()
{
    app_motion_private_processInputs();
    app_motion_state_E desiredState = app_motion_private_getDesiredState();

    app_motion_private_runAction();
    if (desiredState != app_motion_data.state)
    {
        DEBUG_INFO("Transitioning from %d -> %d\n", app_motion_data.state, desiredState);
        app_motion_private_exitAction();
        app_motion_data.state = desiredState;
        app_motion_private_entryAction();
    }

    app_motion_private_processOutputs();
}

bool app_motion_addTestMove(app_motion_move_t *command)
{
    return lib_staticQueue_push(&app_motion_data.testQueue, command);
}

bool app_motion_addManualMove(app_motion_move_t *command)
{
    return lib_staticQueue_push(&app_motion_data.manualQueue, command);
}

void app_motion_clearMoveQueue()
{
    lib_staticQueue_empty(&app_motion_data.testQueue);
    lib_staticQueue_empty(&app_motion_data.manualQueue);
}

uint32_t app_motion_getSetpoint()
{
    APP_MOTION_LOCK_REQ_BLOCK();
    uint32_t setpoint = app_motion_data.outputs.setpoint;
    APP_MOTION_LOCK_REL();
    return setpoint;
}

uint32_t app_motion_getPosition()
{
    APP_MOTION_LOCK_REQ_BLOCK();
    uint32_t position = app_motion_data.outputs.position;
    APP_MOTION_LOCK_REL();
    return position;
}
