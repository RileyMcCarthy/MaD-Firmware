//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "dev_stepper.h"

#include <propeller2.h>
#include <smartpins.h>
#include "Debug.h"
/**********************************************************************
 * Constants
 **********************************************************************/
// I am thinking we use the same method as before
// use waitx for slow moves and pulsecount for fast moves
// the only difference is we have a run1us method
// this requires we run at 100us :)
// for 16 bit signal as max speed or 65535 steps.
// with clockfreq of 180000000, we will need
// 364 us per pulse.
// this means dev_positionMotor should run in own cog.
/*********************************************************************
 * Macros
 **********************************************************************/
#define SM_LOCK_REQ() _locktry(dev_stepper_data.lock)
#define SM_LOCK_REQ_BLOCK()        \
    while (SM_LOCK_REQ() == false) \
        ;
#define SM_LOCK_REL() _lockrel(dev_stepper_data.lock)

// Hardware uses 16 bit value for clockcycles per half pulse cycle
#define DEV_STEPPER_MIN_HARDWARE_SPEED (65535U * 2U)
/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef struct
{
    int32_t targetSteps;
    uint32_t stepsPerSecond;
    uint32_t clockCyclesPerStep;
} dev_stepper_move_S;

typedef struct
{
    dev_stepper_move_S move;
    bool enabled;
} dev_stepper_channelRequest_S;

typedef struct
{
    bool ready;
} dev_stepper_channelOutput_S;

typedef struct
{
    dev_stepper_channelRequest_S request;
    dev_stepper_channelRequest_S stagedRequest;
    dev_stepper_channelOutput_S output;
    dev_stepper_channelOutput_S stagedOutput;

    dev_stepper_state_E state;

    dev_stepper_move_S currentMove;
    int32_t currentSteps;

    uint32_t startx;
    int32_t startSteps;
    int32_t totalSteps;
    bool moveComplete;
} dev_stepper_channelData_S;

typedef struct
{
    dev_stepper_channelData_S channels[DEV_STEPPER_CHANNEL_COUNT];
    int lock;
} dev_stepper_data_S;
/**********************************************************************
 * External Variables
 **********************************************************************/
extern dev_stepper_channelConfig_S dev_stepper_channelConfig[DEV_STEPPER_CHANNEL_COUNT];
/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
static dev_stepper_data_S dev_stepper_data;
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/
//// HIIII RILEY TMRW, I need to make sure we consume the "staged" request into a single
// move request by saving the target, speed etc. so we finish the move before it changes
static void dev_stepper_private_hardwarePulse(dev_stepper_channel_E ch, uint32_t steps, uint32_t clockCyclesPerStep)
{
    _pinstart(dev_stepper_channelConfig[ch].pinStep, P_TRANSITION | P_OE, clockCyclesPerStep >> 1, 0);
    _wypin(dev_stepper_channelConfig[ch].pinStep, steps);
    dev_stepper_data.channels[ch].startx = _cnt();
    dev_stepper_data.channels[ch].startSteps = dev_stepper_data.channels[ch].currentSteps;
    dev_stepper_data.channels[ch].totalSteps = steps;
}

static int32_t dev_stepper_private_computeHardwarePulseCount(dev_stepper_channel_E ch)
{
    const int32_t deltaClockCycles = (_cnt() - dev_stepper_data.channels[ch].startx);
    const int32_t deltaSteps = deltaClockCycles / dev_stepper_data.channels[ch].currentMove.clockCyclesPerStep;
    const int32_t deltaStepsLimited = deltaSteps > dev_stepper_data.channels[ch].totalSteps ? dev_stepper_data.channels[ch].totalSteps : deltaSteps;
    const int32_t currentSteps = dev_stepper_data.channels[ch].startSteps + deltaStepsLimited;
    return currentSteps;
}

static void dev_stepper_private_stageRequest(dev_stepper_channel_E ch)
{
    SM_LOCK_REQ_BLOCK();
    dev_stepper_data.channels[ch].stagedRequest = dev_stepper_data.channels[ch].request;
    SM_LOCK_REL();
}

static void dev_stepper_private_stageOutput(dev_stepper_channel_E ch)
{
    SM_LOCK_REQ_BLOCK();
    dev_stepper_data.channels[ch].stagedOutput = dev_stepper_data.channels[ch].output;
    SM_LOCK_REL();
}

static dev_stepper_state_E dev_stepper_private_getDesiredState(dev_stepper_channel_E ch)
{
    dev_stepper_state_E desiredState = dev_stepper_data.channels[ch].state;
    const bool isMoveSlow = (dev_stepper_data.channels[ch].stagedRequest.move.clockCyclesPerStep < DEV_STEPPER_MIN_HARDWARE_SPEED);
    switch (dev_stepper_data.channels[ch].state)
    {
    case DEV_STEPPER_STATE_INIT:
        if (dev_stepper_data.channels[ch].stagedRequest.enabled)
        {
            desiredState = DEV_STEPPER_STATE_STOPPED;
        }
        break;
    case DEV_STEPPER_STATE_STOPPED:
        if (dev_stepper_data.channels[ch].stagedRequest.enabled == false)
        {
            desiredState = DEV_STEPPER_STATE_INIT;
        }
        else if (dev_stepper_data.channels[ch].stagedRequest.move.targetSteps > dev_stepper_data.channels[ch].currentSteps)
        {
            desiredState = isMoveSlow ? DEV_STEPPER_STATE_MOVING_SLOW_CW : DEV_STEPPER_STATE_MOVING_CW;
        }
        else if (dev_stepper_data.channels[ch].stagedRequest.move.targetSteps < dev_stepper_data.channels[ch].currentSteps)
        {
            desiredState = isMoveSlow ? DEV_STEPPER_STATE_MOVING_SLOW_CCW : DEV_STEPPER_STATE_MOVING_CCW;
        }
        else
        {
            // At target
        }
        break;
    case DEV_STEPPER_STATE_MOVING_CW:
    case DEV_STEPPER_STATE_MOVING_CCW:
    case DEV_STEPPER_STATE_MOVING_SLOW_CW:
    case DEV_STEPPER_STATE_MOVING_SLOW_CCW:
        if (dev_stepper_data.channels[ch].stagedRequest.enabled == false)
        {
            desiredState = DEV_STEPPER_STATE_INIT;
        }
        else if (dev_stepper_data.channels[ch].moveComplete)
        {
            desiredState = DEV_STEPPER_STATE_STOPPED;
        }
        break;
    case DEV_STEPPER_STATE_COUNT:
    default:
        break;
    }
    return desiredState;
}

static void dev_stepper_private_exitAction(dev_stepper_channel_E ch)
{
    switch (dev_stepper_data.channels[ch].state)
    {
    case DEV_STEPPER_STATE_INIT:
        break;
    case DEV_STEPPER_STATE_STOPPED:
        break;
    case DEV_STEPPER_STATE_MOVING_CW:
        _pinclear(dev_stepper_channelConfig[ch].pinStep);
        break;
    case DEV_STEPPER_STATE_MOVING_CCW:
        _pinclear(dev_stepper_channelConfig[ch].pinStep);
        break;
    case DEV_STEPPER_STATE_MOVING_SLOW_CW:
        break;
    case DEV_STEPPER_STATE_MOVING_SLOW_CCW:
        break;
    case DEV_STEPPER_STATE_COUNT:
    default:
        break;
    }
}

static void dev_stepper_private_entryAction(dev_stepper_channel_E ch)
{
    switch (dev_stepper_data.channels[ch].state)
    {
    case DEV_STEPPER_STATE_INIT:
        break;
    case DEV_STEPPER_STATE_STOPPED:
        break;
    case DEV_STEPPER_STATE_MOVING_CW:
        dev_stepper_data.channels[ch].moveComplete = false;
        dev_stepper_data.channels[ch].currentMove = dev_stepper_data.channels[ch].stagedRequest.move;
        _pinl(dev_stepper_channelConfig[ch].pinDirection);
        dev_stepper_private_hardwarePulse(ch, dev_stepper_data.channels[ch].currentMove.targetSteps - dev_stepper_data.channels[ch].currentSteps, dev_stepper_data.channels[ch].currentMove.clockCyclesPerStep);
        break;
    case DEV_STEPPER_STATE_MOVING_CCW:
        dev_stepper_data.channels[ch].moveComplete = false;
        dev_stepper_data.channels[ch].currentMove = dev_stepper_data.channels[ch].stagedRequest.move;
        _pinh(dev_stepper_channelConfig[ch].pinDirection);
        dev_stepper_private_hardwarePulse(ch, dev_stepper_data.channels[ch].currentSteps - dev_stepper_data.channels[ch].currentMove.targetSteps, dev_stepper_data.channels[ch].currentMove.clockCyclesPerStep);
        break;
    case DEV_STEPPER_STATE_MOVING_SLOW_CW:
        dev_stepper_data.channels[ch].moveComplete = false;
        dev_stepper_data.channels[ch].currentMove = dev_stepper_data.channels[ch].stagedRequest.move;
        _pinl(dev_stepper_channelConfig[ch].pinDirection);
        break;
    case DEV_STEPPER_STATE_MOVING_SLOW_CCW:
        dev_stepper_data.channels[ch].moveComplete = false;
        dev_stepper_data.channels[ch].currentMove = dev_stepper_data.channels[ch].stagedRequest.move;
        _pinh(dev_stepper_channelConfig[ch].pinDirection);
        break;
    case DEV_STEPPER_STATE_COUNT:
    default:
        break;
    }
}

static void dev_stepper_private_runAction(dev_stepper_channel_E ch)
{
    switch (dev_stepper_data.channels[ch].state)
    {
    case DEV_STEPPER_STATE_INIT:
        dev_stepper_data.channels[ch].output.ready = false;
        break;
    case DEV_STEPPER_STATE_STOPPED:
        dev_stepper_data.channels[ch].output.ready = true;
        break;
    case DEV_STEPPER_STATE_MOVING_CW:
    case DEV_STEPPER_STATE_MOVING_CCW:
        dev_stepper_data.channels[ch].moveComplete = (_pinr(dev_stepper_channelConfig[ch].pinStep) != 0);
        dev_stepper_data.channels[ch].currentSteps = dev_stepper_private_computeHardwarePulseCount(ch);
        dev_stepper_data.channels[ch].output.ready = true;
        break;
    case DEV_STEPPER_STATE_MOVING_SLOW_CW:
        _pinl(dev_stepper_channelConfig[ch].pinStep);
        _waitx(dev_stepper_data.channels[ch].currentMove.clockCyclesPerStep >> 1);
        _pinh(dev_stepper_channelConfig[ch].pinStep);
        _waitx(dev_stepper_data.channels[ch].currentMove.clockCyclesPerStep >> 1);
        dev_stepper_data.channels[ch].currentSteps++;
        dev_stepper_data.channels[ch].moveComplete = (dev_stepper_data.channels[ch].currentSteps == dev_stepper_data.channels[ch].currentMove.targetSteps);
        dev_stepper_data.channels[ch].output.ready = true;
        break;
    case DEV_STEPPER_STATE_MOVING_SLOW_CCW:
        _pinl(dev_stepper_channelConfig[ch].pinStep);
        _waitx(dev_stepper_data.channels[ch].currentMove.clockCyclesPerStep >> 1);
        _pinh(dev_stepper_channelConfig[ch].pinStep);
        _waitx(dev_stepper_data.channels[ch].currentMove.clockCyclesPerStep >> 1);
        dev_stepper_data.channels[ch].currentSteps--;
        dev_stepper_data.channels[ch].moveComplete = (dev_stepper_data.channels[ch].currentSteps == dev_stepper_data.channels[ch].currentMove.targetSteps);
        dev_stepper_data.channels[ch].output.ready = true;
        break;
    case DEV_STEPPER_STATE_COUNT:
    default:
        dev_stepper_data.channels[ch].output.ready = false;
        break;
    }
}

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
void dev_stepper_init(int lock)
{
    dev_stepper_data.lock = lock;
}

void dev_stepper_run()
{
    for (dev_stepper_channel_E ch = (dev_stepper_channel_E)0U; ch < DEV_STEPPER_CHANNEL_COUNT; ch++)
    {
        dev_stepper_private_stageRequest(ch);
        dev_stepper_state_E desiredState = dev_stepper_private_getDesiredState(ch);
        if (desiredState != dev_stepper_data.channels[ch].state)
        {
            DEBUG_INFO("Transitioning from %d -> %d\n", dev_stepper_data.channels[ch].state, desiredState);
            dev_stepper_private_exitAction(ch);
            dev_stepper_data.channels[ch].state = desiredState;
            dev_stepper_private_entryAction(ch);
        }
        dev_stepper_private_runAction(ch);
        dev_stepper_private_stageOutput(ch);
    }
}

bool dev_stepper_move(dev_stepper_channel_E ch, int32_t targetSteps, uint32_t stepsPerSecond)
{
    uint32_t clockCyclesPerStep = _clockfreq() / dev_stepper_data.channels[ch].request.move.stepsPerSecond;
    SM_LOCK_REQ_BLOCK();
    dev_stepper_data.channels[ch].request.move.targetSteps = targetSteps;
    dev_stepper_data.channels[ch].request.move.stepsPerSecond = stepsPerSecond;
    dev_stepper_data.channels[ch].request.move.clockCyclesPerStep = clockCyclesPerStep;
    SM_LOCK_REL();
    return true;
}

void dev_stepper_enable(dev_stepper_channel_E ch, bool enabled)
{
    SM_LOCK_REQ_BLOCK();
    dev_stepper_data.channels[ch].request.enabled = enabled;
    SM_LOCK_REL();
}

dev_stepper_state_E dev_stepper_getState(dev_stepper_channel_E ch)
{
    return dev_stepper_data.channels[ch].state;
}

int32_t dev_stepper_getSteps(dev_stepper_channel_E ch)
{
    return dev_stepper_data.channels[ch].currentSteps;
}

int32_t dev_stepper_getTarget(dev_stepper_channel_E ch)
{
    return dev_stepper_data.channels[ch].currentMove.targetSteps;
}

bool dev_stepper_atTarget(dev_stepper_channel_E ch)
{
    return dev_stepper_data.channels[ch].request.move.targetSteps == dev_stepper_data.channels[ch].currentSteps;
}

bool dev_stepper_isReady(dev_stepper_channel_E ch)
{
    bool ready;
    SM_LOCK_REQ_BLOCK();
    ready = dev_stepper_data.channels[ch].stagedOutput.ready;
    SM_LOCK_REL();
    return ready;
}

/**********************************************************************
 * End of File
 **********************************************************************/
