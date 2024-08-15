//
// Created by Riley McCarthy on 25/04/24.
//
/*
Clarification and Detailing of Homing, Coordinates and Lengths:
Definitions
- Positive X Direction is from the upper jaw towards the lower jaw, that is in the downward direction.

- Machine Position (NEW) is the distance of the LOWER jaw grip lip relative to the UPPER jaw grip lip.
  Would be useful to display this position.   Note that, in order to protect the force gauge,
  the machine position can never be zero as the limit upper switches are located
  in a positive X position so as to prevent the jaws from colliding.

- Machine Coordinate is relative to the location of the upper jaw grip lip,
  that is the stationary upper jaw grip lip is located at X = 0 in Machine Coordinates.

- Home (NEW) is a button in the ?? section that when clicked by the user will initiate the homing sequence as follows;
    First the machine will move in the negative at a feed rate of 5?? mm*s-1 until the upper inner endstop is activated (opened)
    the machine will then move in a positive direction by 5?? mm at a feed rate of 5 ?? mm*s-1
    then stop and move in the negative direction at a slow feed rate of 0.1?? mm*s-1
    until the inner endstop is activated (opened),
    the machine will then move in the positive direction by the Home Standoff distance at a feed rate of  5?? mm*s-1 and stop.
    Finally a Test Zero "function" (same action when the currently labeled "Zero Length" button) will be called,
    causing the Machine and Test Positions to be equivalent until such time as the user jogs
    the machine and establishes a new Test Zero position (see below).

- Home Standoff is the distance moved in the positive X direction after the second (slow speed)
  negative X direction activation of the inner upper limit switch.
  This move is required to remove any backlash in the tensioning (positive X) direction (as well as to clear the switch of course!).
  Probably should be 5 mm and can be hard coded or set in the machine profile.

 - Home Offset is the distance from the upper jaw grip lip to the lower jaw grip lip AFTER all homing motion is completed.
 This value is MANUALLY MEASURED and ENTERED by the user via a NEW input field to be labeled "Home Offset" on the Settings?? page
 and is part of the Machine Profile??.
 After homing, the position in the Machine Coordinate is the Home Offset which is the sum of  the distance
 between the upper jaw grip lip and the lower jaw grip lip when the inner upper limit switch is activated
 during the second homing negative X move AND the Home Standoff distance.

- Test Coordinate is relative to the Test Zero position.
  It is possible for a position in the Test Coordinate to be negative.
  Test Profile moves are defined and test position values are recorded and displayed in Test Coordinates.

- Test Zero is an arbitrary user-established position set when the user clicks the
  "Test Zero" button (currently labeled "Zero Length") on the Status Page or by the homing sequence.
  Test Profile moves are defined relative to the "Test Zero" position.

- Test Position is the distance (in the positive direction) of the lower jaw grip lip relative to the Test Zero position
  and is currently displayed on the Status page in the "Position" field.
  Other than being renamed as "Test Position" on the display and in the file header, It remains as displayed and recorded.
  The Test Position may be positive or negative.

- Gauge Length is an arbitrary value equivalent to the Machine Position value when the user clicks
  the "Test Zero" (currently labeled "Zero Length") button.
  Currently, the user manually measures and enters the Gauge Length via the "Gauge Length" field and it is passed to the file header.
  Gauge Length may be equal to, less than or greater than the Sample Length.
  While manual measurement and entry of the Gauge Length is workable for now,
  there would be a useful gain in efficiency as well as elimination of the potential for user measurement and entry error
  if the value could be captured from the the Machine Position via a user clickable "Set" button
  beside the current "Gauge Length" value field similar to the Update buttons besides the Test Number and Date fields.

- Sample Length (NEW) is the measured distance between the grip lips of the upper and lower jaws
  when the machine is positioned so that there is zero tension but no slack in the sample
  (see procedure below to establish Sample Length).
  Once established this value could be manually entered by the user or auto captured
  in a NEW "Sample Length" field on the Status page.
  This value will be displayed in the Test Setup section on the Status page and will be passed to the file header.
  When measured/captured, the value of the Sample Length is the same value as the Machine Position.
  While manual measurement and entry of the Sample Length would be workable for now,
  there would be a useful gain in efficiency as well as elimination of the potential for user measurement
  and entry error if the value could be captured from the the Machine Position via a user clickable
  "Set" button beside the NEW "Sample Length" value field similar to the Update buttons
  beside the Test Number and Date fields.
  */
/**********************************************************************
 * Includes
 **********************************************************************/
#include <propeller2.h>
#include <string.h>
#include "app_monitor.h"
#include "app_motion.h"

#include "dev_nvram.h"
#include "dev_logger.h"

#include "StateMachine.h"
#include "IO_digitalPin.h"

#include "ForceGauge.h"
#include "Encoder.h"
#include "Debug.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

#define APP_MONITOR_LOCK_REQ() _locktry(app_monitor_data.lock)
#define APP_MONITOR_LOCK_REQ_BLOCK()        \
    while (APP_MONITOR_LOCK_REQ() == false) \
        ;
#define APP_MONITOR_LOCK_REL() _lockrel(app_monitor_data.lock)

/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef struct
{
    int32_t forceRaw;
    int32_t encoderRaw;
    int32_t setpoint; // um
    uint32_t forceGaugeLog;
    uint32_t time; // us
    bool testRunning;
} app_monitor_inputData_t;

typedef struct
{
    bool setGaugeLength;
    bool setGaugeForce;
} app_monitor_requestData_t;

typedef struct
{
    app_monitor_sample_t sample;
    app_monitor_sample_t sampleOutput;
} app_monitor_outputData_t;

typedef struct
{
    app_monitor_requestData_t stagedRequest; // thread safe
    app_monitor_requestData_t request;
    app_monitor_inputData_t input;

    int lock;
    uint32_t gaugeLength;
    ForceGauge forceGauge;
    Encoder encoder;
    MachineProfile machineProfile;
    app_monitor_loggingState_E loggingState;

    app_monitor_sample_t sample;
    app_monitor_sample_t sampleOutput;
} app_monitor_data_t;

/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/

static app_monitor_data_t app_monitor_data;

/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

void app_monitor_private_processInputs()
{
    MachineState machineState;
    get_machine_state(&machineState);
    app_monitor_data.input.forceRaw = app_monitor_data.forceGauge.forceRaw;
    if (app_monitor_data.forceGauge.responding)
    {
        state_machine_set(PARAM_MACHINE_FORCE_GAUGE_COM, 1);
    }
    app_monitor_data.input.encoderRaw = encoder_value(&app_monitor_data.encoder);
    app_monitor_data.input.setpoint = app_motion_getSetpoint();
    app_monitor_data.input.forceGaugeLog = app_monitor_data.forceGauge.counter;
    app_monitor_data.input.time = _getus();
    app_monitor_data.input.testRunning = machineState.motionParameters.mode == MODE_TEST_RUNNING;
}

void app_monitor_private_stageRequests()
{
    if (memcmp(&app_monitor_data.request, &app_monitor_data.stagedRequest, sizeof(app_monitor_requestData_t)) != 0)
    {
        APP_MONITOR_LOCK_REQ_BLOCK();
        memcpy(&app_monitor_data.request, &app_monitor_data.stagedRequest, sizeof(app_monitor_requestData_t));
        APP_MONITOR_LOCK_REL();
    }
}

void app_monitor_private_processSample()
{
    const int32_t forceOffset = app_monitor_data.machineProfile.configuration.forceGaugeOffset;
    const int32_t encoderStepsPermm = app_monitor_data.machineProfile.configuration.encoderStepsPermm;
    const int32_t forceGain = app_monitor_data.machineProfile.configuration.forceGaugeGain;

    app_monitor_data.sample.force = (app_monitor_data.input.forceRaw - forceOffset) / forceGain;
    int32_t absolutePosition = (app_monitor_data.input.encoderRaw * 1000.0) / encoderStepsPermm;
    app_monitor_data.sample.position = app_monitor_data.gaugeLength - absolutePosition;
    app_monitor_data.sample.time = app_monitor_data.input.time;
    app_monitor_data.sample.index = app_monitor_data.input.forceGaugeLog;
    app_monitor_data.sample.setpoint = app_monitor_data.input.setpoint;
}

void app_monitor_private_stageSample()
{
    if (memcmp(&app_monitor_data.sample, &app_monitor_data.sampleOutput, sizeof(app_monitor_sample_t)) != 0)
    {
        APP_MONITOR_LOCK_REQ_BLOCK();
        memcpy(&app_monitor_data.sampleOutput, &app_monitor_data.sample, sizeof(app_monitor_sample_t));
        APP_MONITOR_LOCK_REL();
    }
}

void app_monitor_private_processLogging()
{
    app_monitor_loggingState_E currentState = app_monitor_data.loggingState;
    switch (currentState)
    {
    case APP_MONITOR_LOGGING_STATE_IDLE:
        if (app_monitor_data.input.testRunning)
        {
            if (dev_logger_append(DEV_LOGGER_CHANNEL_SAMPLE_DATA, DEV_LOGGER_CHANNEL_SAMPLE_DATA_HEADER))
            {
                app_monitor_data.loggingState = APP_MONITOR_LOGGING_STATE_RUNNING;
            }
            else
            {
                DEBUG_INFO("%s", "Failed to start logging due to sample data header not excisting\n");
            }
        }
        break;
    case APP_MONITOR_LOGGING_STATE_RUNNING:
        if (app_monitor_data.input.testRunning == false)
        {
            app_monitor_data.loggingState = APP_MONITOR_LOGGING_STATE_STOPPING;
        }
        else
        {
            dev_logger_push(DEV_LOGGER_CHANNEL_SAMPLE_DATA, &app_monitor_data.sample, sizeof(app_monitor_sample_t));
        }
        break;
    case APP_MONITOR_LOGGING_STATE_STOPPING:
        app_monitor_data.loggingState = APP_MONITOR_LOGGING_STATE_IDLE;
        dev_logger_stop(DEV_LOGGER_CHANNEL_SAMPLE_DATA);
        break;
    default:
        break;
    }
}

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void app_monitor_init(int lock)
{
    app_monitor_data.lock = lock;
    force_gauge_begin(&app_monitor_data.forceGauge, FORCE_GAUGE_RX, FORCE_GAUGE_TX);
    encoder_start(&app_monitor_data.encoder, SERVO_ENCODER_A, SERVO_ENCODER_B, -1, false, 0, -100000, 100000);
    dev_nvram_getChannelData(DEV_NVRAM_CHANNEL_MACHINE_PROFILE, &app_monitor_data.machineProfile, sizeof(MachineProfile));
    app_monitor_data.gaugeLength = 0;
    app_monitor_data.loggingState = APP_MONITOR_LOGGING_STATE_IDLE;
}

void app_monitor_run()
{
    app_monitor_private_stageRequests();
    app_monitor_private_processInputs();
    app_monitor_private_processSample();
    app_monitor_private_processLogging();

    if (app_monitor_data.request.setGaugeLength)
    {
        app_monitor_data.gaugeLength = app_monitor_data.sample.position;
        app_monitor_data.request.setGaugeLength = false;
    }

    app_monitor_private_stageSample();
#ifdef __EMULATION__
    _waitms(100);
#endif
}

void app_monitor_getSample(app_monitor_sample_t *sample)
{
    APP_MONITOR_LOCK_REQ_BLOCK();
    memcpy(sample, &app_monitor_data.sampleOutput, sizeof(app_monitor_sample_t));
    APP_MONITOR_LOCK_REL();
}

void app_monitor_setGaugeLength()
{
    APP_MONITOR_LOCK_REQ_BLOCK();
    app_monitor_data.stagedRequest.setGaugeLength = true;
    APP_MONITOR_LOCK_REL();
}

void app_monitor_setGaugeForce()
{
    APP_MONITOR_LOCK_REQ_BLOCK();
    app_monitor_data.stagedRequest.setGaugeForce = true;
    APP_MONITOR_LOCK_REL();
}

/**********************************************************************
 * End of File
 **********************************************************************/
