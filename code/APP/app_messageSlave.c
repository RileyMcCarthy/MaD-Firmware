//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <string.h>
#include "app_messageSlave.h"

#include "StateMachine.h"
#include "app_monitor.h"
#include "app_motion.h"
#include "IO_protocol.h"
#include "JsonDecoder.h"
#include "JsonEncoder.h"
#include "dev_nvram.h"
#include "dev_logger.h"
#include "Debug.h"
/**********************************************************************
 * Constants
 **********************************************************************/
static const char *const app_messageSlave_awk_to_string[] = {
    "NONE",
    "OK",
    "BUSY",
    "FAIL",
};
/*********************************************************************
 * Macros
 **********************************************************************/
#define APP_MESSAGESLAVE_LOCK_REQ() _locktry(app_message_slave_data.lock)
#define APP_MESSAGESLAVE_LOCK_REQ_BLOCK()        \
    while (APP_MESSAGESLAVE_LOCK_REQ() == false) \
        ;
#define APP_MESSAGESLAVE_LOCK_REL() _lockrel(app_message_slave_data.lock)
/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef enum
{
    APP_MESSAGE_SLAVE_AWK_NONE,
    APP_MESSAGE_SLAVE_AWK_OK,
    APP_MESSAGE_SLAVE_AWK_BUSY,
    APP_MESSAGE_SLAVE_AWK_ERROR,
    APP_MESSAGE_SLAVE_AWK_COUNT,
} app_message_slave_awk_E;

typedef struct
{
    uint32_t lastPing;
    bool requestTestMode;
    bool requestMotionEnabled;
} app_message_slave_output_S;

typedef struct
{
    IO_protocol_command_E command;
    bool isWrite;
    bool commandRecieved;
    bool recievedSync;
    uint16_t dataLength;
    uint16_t dataIndex;
    char dataRX[APP_MESSAGE_SLAVE_RX_BUFFER_SIZE];
    char dataTX[APP_MESSAGE_SLAVE_TX_BUFFER_SIZE];
    app_message_slave_awk_E awk;
    MachineProfile machineProfile;

    app_message_slave_state_E state;
    int lock;

    app_message_slave_output_S stagedOutput;
    app_message_slave_output_S output;
} app_message_slave_data_S;
/**********************************************************************
 * External Variables
 **********************************************************************/

/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
app_message_slave_data_S app_message_slave_data;
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/
static void app_messageSlave_private_encodeCommand();
static app_message_slave_awk_E app_message_slave_private_decodeCommand();
/**********************************************************************
 * Private Function Definitions
 **********************************************************************/

static app_message_slave_state_E app_message_slave_private_getDesiredState()
{
    app_message_slave_state_E desiredState = app_message_slave_data.state;
    switch (app_message_slave_data.state)
    {
    case APP_MESSAGE_SLAVE_STATE_INIT:
        desiredState = APP_MESSAGE_SLAVE_STATE_WAITING;
        break;
    case APP_MESSAGE_SLAVE_STATE_WAITING:
        if (IO_protocol_recieveSync())
        {
            desiredState = APP_MESSAGE_SLAVE_STATE_READING_COMMAND;
        }
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_COMMAND:
        if (app_message_slave_data.commandRecieved)
        {
            if (app_message_slave_data.command == IO_PROTOCOL_COMMAND_SNA)
            {
                desiredState = APP_MESSAGE_SLAVE_STATE_WAITING;
            }
            else if (app_message_slave_data.isWrite)
            {
                desiredState = APP_MESSAGE_SLAVE_STATE_READING_HEADER;
            }
            else
            {
                desiredState = APP_MESSAGE_SLAVE_STATE_RESPOND;
            }
        }
        else if (app_message_slave_data.command == IO_PROTOCOL_COMMAND_ERROR)
        {
            desiredState = APP_MESSAGE_SLAVE_STATE_WAITING;
        }
        else
        {
            // Continue waiting for command
        }
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_HEADER:
        if (app_message_slave_data.dataLength >= APP_MESSAGE_SLAVE_RX_BUFFER_SIZE)
        {
            desiredState = APP_MESSAGE_SLAVE_STATE_WAITING;
            DEBUG_ERROR("Data length too long: %hu\n", app_message_slave_data.dataLength);
        }
        else if (app_message_slave_data.dataLength > 0U)
        {
            desiredState = APP_MESSAGE_SLAVE_STATE_READING_DATA;
        }
        else
        {
        }
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_DATA:
        if (app_message_slave_data.dataIndex >= app_message_slave_data.dataLength)
        {
            desiredState = APP_MESSAGE_SLAVE_STATE_READING_PROCESS;
        }
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_PROCESS:
        if ((app_message_slave_data.awk > APP_MESSAGE_SLAVE_AWK_NONE) && (app_message_slave_data.awk < APP_MESSAGE_SLAVE_AWK_COUNT))
        {
            desiredState = APP_MESSAGE_SLAVE_STATE_AWK;
        }
        else
        {
            desiredState = APP_MESSAGE_SLAVE_STATE_WAITING;
        }
        break;
    case APP_MESSAGE_SLAVE_STATE_RESPOND:
        return APP_MESSAGE_SLAVE_STATE_WAITING;
    case APP_MESSAGE_SLAVE_STATE_AWK:
        return APP_MESSAGE_SLAVE_STATE_WAITING;
    default:
        desiredState = APP_MESSAGE_SLAVE_STATE_INIT;
        break;
    }
    return desiredState;
}

static void app_message_slave_private_entryAction()
{
    switch (app_message_slave_data.state)
    {
    case APP_MESSAGE_SLAVE_STATE_INIT:
        break;
    case APP_MESSAGE_SLAVE_STATE_WAITING:
        app_message_slave_data.recievedSync = false;
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_COMMAND:
        app_message_slave_data.command = IO_PROTOCOL_COMMAND_SNA;
        app_message_slave_data.isWrite = false;
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_HEADER:
        app_message_slave_data.dataLength = 0;
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_DATA:
        strncpy(app_message_slave_data.dataRX, "", APP_MESSAGE_SLAVE_RX_BUFFER_SIZE);
        app_message_slave_data.dataIndex = 0;
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_PROCESS:
        app_message_slave_data.awk = app_message_slave_private_decodeCommand();
        break;
    case APP_MESSAGE_SLAVE_STATE_RESPOND:
        strncpy(app_message_slave_data.dataTX, "", APP_MESSAGE_SLAVE_TX_BUFFER_SIZE);
        app_messageSlave_private_encodeCommand();
        break;
    case APP_MESSAGE_SLAVE_STATE_AWK:
        strncpy(app_message_slave_data.dataTX, "", APP_MESSAGE_SLAVE_TX_BUFFER_SIZE);
        snprintf(app_message_slave_data.dataTX,
                 APP_MESSAGE_SLAVE_TX_BUFFER_SIZE,
                 "{\"cmd\":\"%d\",\"awk\":\"%s\"}",
                 app_message_slave_data.command, app_messageSlave_awk_to_string[app_message_slave_data.awk]);
        break;
    default:
        break;
    }
}

static void app_message_slave_private_runAction()
{
    switch (app_message_slave_data.state)
    {
    case APP_MESSAGE_SLAVE_STATE_INIT:
        break;
    case APP_MESSAGE_SLAVE_STATE_WAITING:
        app_message_slave_data.recievedSync = IO_protocol_recieveSync();
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_COMMAND:
        app_message_slave_data.commandRecieved = IO_protocol_recieveCommand(&app_message_slave_data.isWrite, &app_message_slave_data.command);
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_HEADER:
        app_message_slave_data.dataLength = IO_protocol_recieveLength();
        DEBUG_INFO("Command: %d\n", app_message_slave_data.commandRecieved);
        DEBUG_INFO("Data length: %d\n", app_message_slave_data.dataLength);
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_DATA:
        if (app_message_slave_data.dataLength > app_message_slave_data.dataIndex)
        {
            uint16_t remaining = app_message_slave_data.dataLength - app_message_slave_data.dataIndex;
            app_message_slave_data.dataIndex += IO_protocol_recieveString(app_message_slave_data.dataRX, remaining);
        }
        break;
    case APP_MESSAGE_SLAVE_STATE_READING_PROCESS:
        break;
    case APP_MESSAGE_SLAVE_STATE_RESPOND:
        (void)IO_protocol_send(app_message_slave_data.command, (uint8_t *)app_message_slave_data.dataTX, strlen(app_message_slave_data.dataTX));
        break;
    case APP_MESSAGE_SLAVE_STATE_AWK:
        DEBUG_INFO("Sending awk: %d\n", app_message_slave_data.awk);
        DEBUG_INFO("Data: %s\n", app_message_slave_data.dataTX);
        (void)IO_protocol_send(IO_PROTOCOL_COMMAND_AWK, (uint8_t *)app_message_slave_data.dataTX, strlen(app_message_slave_data.dataTX));
        break;
    default:
        break;
    }
}

app_message_slave_awk_E app_message_slave_private_decodeCommand()
{
    app_message_slave_awk_E awk = APP_MESSAGE_SLAVE_AWK_NONE;
    switch (app_message_slave_data.command)
    {
    case IO_PROTOCOL_COMMAND_PING:
        app_message_slave_data.stagedOutput.lastPing = _getms();
        break;
    case IO_PROTOCOL_COMMAND_DATA:
        // read only
        break;
    case IO_PROTOCOL_COMMAND_STATE:
        // read only
        break;
    case IO_PROTOCOL_COMMAND_MPROFILE:
        DEBUG_INFO("Saving new machine profile: %s\n", app_message_slave_data.dataRX);
        MachineProfile newProfile;
        if (json_to_machine_profile(&newProfile, app_message_slave_data.dataRX) == false)
        {
            DEBUG_WARNING("%s", "failed to parse machine profile\n");
            break;
        }

        dev_nvram_updateChannelData(DEV_NVRAM_CHANNEL_MACHINE_PROFILE, &newProfile, sizeof(MachineProfile));
        // should have this notify once it successfully writes to NVRAM
        app_notification_send(APP_NOTIFICATION_TYPE_SUCCESS, "Machine profile saved to SD Card, please reboot\n");
#ifndef __EMULATION__
        _waitms(1000);
        _reboot(); //-- need to reenable, how can I simulate reboot...
#endif
        break;
    case IO_PROTOCOL_COMMAND_MOTIONPROFILE:
        // @todo delete???
        break;
    case IO_PROTOCOL_COMMAND_MOTIONMODE:
    {
        // @todo make this generic, like reqTestMode, runTest, etc
        DEBUG_INFO("%s", "Getting motion mode\n");

        MotionMode mode;
        if (json_to_motion_mode(&mode, app_message_slave_data.dataRX) == false)
        {
            DEBUG_WARNING("%s", "failed to parse motion mode\n");
            break;
        }

        DEBUG_INFO("Setting motion mode: %d\n", mode);
        app_message_slave_data.stagedOutput.requestTestMode = mode == MODE_TEST;
        state_machine_set(PARAM_MOTION_MODE, mode);
        break;
    }
    case IO_PROTOCOL_COMMAND_MOTIONSTATUS:
    {
        DEBUG_INFO("%s", "Getting motion status\n");
        // @todo make simple like requestEnableMotion
        MotionStatus status;
        DEBUG_INFO("Motion status: %s\n", app_message_slave_data.dataRX);
        if (json_to_motion_status(&status, app_message_slave_data.dataRX) == false)
        {
            DEBUG_WARNING("%s", "failed to parse motion status\n");
            break;
        }

        app_message_slave_data.stagedOutput.requestMotionEnabled = (status == MOTIONSTATUS_ENABLED);
        state_machine_set(PARAM_MOTION_STATUS, status);

        break;
    }
    case IO_PROTOCOL_COMMAND_MOVE:
    {
        DEBUG_INFO("%s", "Getting test command\n");
        DEBUG_INFO("Data: %s\n", app_message_slave_data.dataRX);

        lib_json_move_S moveJSON;
        if (json_to_move(&moveJSON, app_message_slave_data.dataRX))
        {
            app_motion_move_t move;
            move.x = moveJSON.x * 1000; // mm -> um
            move.f = moveJSON.f * 1000; // mm/s -> um/s
            move.g = moveJSON.g;
            move.p = moveJSON.p;
            if (app_motion_addTestMove(&move))
            {
                DEBUG_INFO("%s", "test move added\n");
                awk = APP_MESSAGE_SLAVE_AWK_OK;
            }
            else
            {
                DEBUG_INFO("%s", "test move busy\n");
                awk = APP_MESSAGE_SLAVE_AWK_BUSY;
            }
        }
        else
        {
            awk = APP_MESSAGE_SLAVE_AWK_ERROR;
            DEBUG_ERROR("%s", "failed to parse test command\n");
        }
        break;
    }
    case IO_PROTOCOL_COMMAND_MANUAL_MOVE:
    {
        DEBUG_INFO("%s", "Getting manual command\n");

        lib_json_move_S moveJSON;
        if (json_to_move(&moveJSON, app_message_slave_data.dataRX))
        {
            app_motion_move_t move;
            move.x = moveJSON.x * 1000; // mm -> um
            move.f = moveJSON.f * 1000; // mm/s -> um/s
            move.g = moveJSON.g;
            move.p = moveJSON.p;
            if (app_motion_addManualMove(&move))
            {
                DEBUG_INFO("manual move added: %dmm at %dmm/s\n", move.x, move.f);
                // send_awk(CMD_MOVE, "OK");
                awk = APP_MESSAGE_SLAVE_AWK_OK;
            }
        }
        break;
    }
    case IO_PROTOCOL_COMMAND_TEST_HEADER:
    {
        DEBUG_INFO("%s", "Recieving test header\n");
        dev_logger_testSampleHeader_S testSampleHeader;
        strncpy(testSampleHeader.header, app_message_slave_data.dataRX, sizeof(testSampleHeader.header) - 1);
        const char *name;
        if (json_to_test_header_name(&name, app_message_slave_data.dataRX))
        {
            DEBUG_INFO("Test header name: %s\n", name);
            DEBUG_INFO("Test Header content: %s\n", testSampleHeader.header);
            app_motion_clearMoveQueue(); // clear any remaining motion test moves before new profile
            dev_logger_start(DEV_LOGGER_CHANNEL_SAMPLE_DATA_HEADER, name);
            if (dev_logger_push(DEV_LOGGER_CHANNEL_SAMPLE_DATA_HEADER, &testSampleHeader, sizeof(dev_logger_testSampleHeader_S)))
            {
                DEBUG_INFO("%s", "Test header pushed\n");
                awk = APP_MESSAGE_SLAVE_AWK_OK;
            }
            else
            {
                DEBUG_ERROR("%s", "Failed to push test header\n");
            }
        }
        else
        {
            DEBUG_ERROR("%s", "Failed to get name for test header\n");
        }
        break;
    }
    break;
    case IO_PROTOCOL_COMMAND_AWK:
        break;
    default:
        break;
    }
    return awk;
}

void app_messageSlave_private_encodeCommand()
{
    switch (app_message_slave_data.command)
    {
    case IO_PROTOCOL_COMMAND_PING:
        DEBUG_INFO("%s", "Sending ping with firmware version\n");
        strncpy(app_message_slave_data.dataTX, "{\"version\": \"1.0.0\"}", APP_MESSAGE_SLAVE_TX_BUFFER_SIZE);
        break;
    case IO_PROTOCOL_COMMAND_DATA:
    {
        app_monitor_sample_t monitor_data;
        app_monitor_getSample(&monitor_data);
        // DEBUG_INFO("Sending Data (%d)\n", monitor_data.index);
        snprintf(app_message_slave_data.dataTX, APP_MESSAGE_SLAVE_TX_BUFFER_SIZE,
                 "{\"Force\":%d,\"Position\":%d,\"Setpoint\":%d,\"Time\":%d,\"Log\":%d, \"Raw\":%d}",
                 monitor_data.force, monitor_data.position, monitor_data.setpoint, monitor_data.time, monitor_data.index, monitor_data.force);
        // DEBUG_INFO("Data: %s\n", app_message_slave_data.dataTX);
        break;
    }
    case IO_PROTOCOL_COMMAND_STATE:
    {
        DEBUG_INFO("%s", "Sending machine state\n");
        MachineState machine_state;
        get_machine_state(&machine_state);
        char *buf = machine_state_to_json(&machine_state);
        if (buf == NULL)
        {
            DEBUG_ERROR("%s", "Failed to convert machine state to json\n");
            return;
        }
        strncpy(app_message_slave_data.dataTX, buf, APP_MESSAGE_SLAVE_TX_BUFFER_SIZE);
        break;
    }
    case IO_PROTOCOL_COMMAND_MPROFILE:
    {
        DEBUG_INFO("%s", "Sending machine profile\n");

        // Convert to json
        char *buf = machine_profile_to_json(&app_message_slave_data.machineProfile);
        if (buf == NULL)
        {
            DEBUG_ERROR("%s", "Failed to convert machine profile to json\n");
            return;
        }
        DEBUG_INFO("Sending machine profile: %s\n", buf);
        strncpy(app_message_slave_data.dataTX, buf, APP_MESSAGE_SLAVE_TX_BUFFER_SIZE);
        break;
    }
    case IO_PROTOCOL_COMMAND_MOTIONPROFILE:
        break;
    case IO_PROTOCOL_COMMAND_MOTIONMODE:
        break;
    case IO_PROTOCOL_COMMAND_MOTIONSTATUS:
        break;
    case IO_PROTOCOL_COMMAND_MOVE:
    {
        break;
    }
    case IO_PROTOCOL_COMMAND_START_TEST:
    {
        // This should not be a read command but idk lol
        // this also should either be consumed or set something in control.c
        if (state_machine_set(PARAM_MOTION_MODE, MODE_TEST_RUNNING))
        {
            app_notification_send(APP_NOTIFICATION_TYPE_SUCCESS, "Starting Test!");
        }
        break;
    }
    default:
        break;
    }
}

static void app_messageSlave_private_stageOutputs()
{
    APP_MESSAGESLAVE_LOCK_REQ_BLOCK();
    memcpy(&app_message_slave_data.output, &app_message_slave_data.stagedOutput, sizeof(app_message_slave_output_S));
    APP_MESSAGESLAVE_LOCK_REL();
}

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void app_messageSlave_init(int lock)
{
    app_message_slave_data.lock = lock;
    dev_nvram_getChannelData(DEV_NVRAM_CHANNEL_MACHINE_PROFILE, &app_message_slave_data.machineProfile, sizeof(MachineProfile));
}

void app_messageSlave_run()
{
    app_message_slave_state_E desiredState = app_message_slave_private_getDesiredState();
    if (desiredState != app_message_slave_data.state)
    {
        app_message_slave_data.state = desiredState;
        app_message_slave_private_entryAction();
    }
    app_message_slave_private_runAction();
    app_messageSlave_private_stageOutputs();
}

bool app_messageSlave_requestMotionEnabled()
{
    APP_MESSAGESLAVE_LOCK_REQ_BLOCK();
    bool motionEnabled = app_message_slave_data.output.requestMotionEnabled;
    APP_MESSAGESLAVE_LOCK_REL();
    return motionEnabled;
}

bool app_messageSlave_requestTestMode()
{
    APP_MESSAGESLAVE_LOCK_REQ_BLOCK();
    bool testMode = app_message_slave_data.output.requestTestMode;
    APP_MESSAGESLAVE_LOCK_REL();
    return testMode;
}

/**********************************************************************
 * End of File
 **********************************************************************/
