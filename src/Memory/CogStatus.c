#include "Memory/CogStatus.h"
#include "Utility/Debug.h"
#include <propeller2.h>

#define COG_STATUS_TIMOUT 100

static int _cog_motion_status = -1;
static int _cog_control_status = -1;
static int _cog_communication_status = -1;
static int _cog_monitor_status = -1;

//each cog should write currentms to memory and we can check if it is still running
// blink led for the cog that dies if it dies


void set_motion_status(int statusms)
{
    _cog_motion_status = statusms;
}

void set_control_status(int statusms)
{
    _cog_control_status = statusms;
}

void set_communication_status(int statusms)
{
    _cog_communication_status = statusms;
}

void set_monitor_status(int statusms)
{
    _cog_monitor_status = statusms;
}

static uint32_t _last_debug_sent = 0;

bool get_machine_status()
{
    const uint32_t currentms = _getms();
    const uint32_t motion_statusms = currentms - _cog_motion_status;
    const uint32_t control_statusms = currentms - _cog_control_status;
    const uint32_t communication_statusms = currentms - _cog_communication_status;
    const uint32_t monitor_statusms = currentms - _cog_monitor_status;


    const bool motion_status = motion_statusms < COG_STATUS_TIMOUT;
    const bool control_status = control_statusms < COG_STATUS_TIMOUT;
    const bool communication_status = communication_statusms < COG_STATUS_TIMOUT;
    const bool monitor_status = monitor_statusms < COG_STATUS_TIMOUT;
    
    const bool machine_status = motion_status && control_status && communication_status && monitor_status;
    if (!machine_status && ((_getms() - _last_debug_sent) > 5000))
    {
        _last_debug_sent = _getms();
        DEBUG_ERROR("Machine Status Error: Motion: %u, Control: %u, Communication: %u, Monitor: %u\n", motion_statusms, control_statusms, communication_statusms, monitor_statusms);
    }
    return machine_status;
}



