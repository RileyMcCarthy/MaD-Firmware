//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "dev_forceGauge.h"
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

typedef struct
{
    uint32_t rawADC; // should really be voltage
    uint32_t responding;
} dev_forceGauge_channelInput_S;

typedef struct
{
    int32_t force;
    uint32_t index;
    bool ready;
} dev_forceGauge_channelOutput_S;

typedef struct
{
    dev_forceGauge_channelInput_S input;
    dev_forceGauge_channelNVRAM_S nvram;
    dev_forceGauge_channelOutput_S output;

    uint32_t retryCount;

    dev_forceGauge_state_E state;
    int32_t lock;
} dev_forceGauge_channelData_S;
/**********************************************************************
 * External Variables
 **********************************************************************/
extern const dev_forceGauge_channelConfig_S dev_forceGauge_channelConfig[DEV_FORCEGAUGE_CHANNEL_COUNT];
/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/
static dev_forceGauge_channelData_S dev_forceGauge_data[DEV_FORCEGAUGE_CHANNEL_COUNT];
/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/
static void dev_forceGauge_private_processInputs(dev_forceGauge_channel_E channel)
{
    dev_forceGauge_data[channel].input.responding = IO_ADS122U04_getConversion(dev_forceGauge_channelConfig[channel].adcChannel, &dev_forceGauge_data[channel].input.rawADC, 1000);
}

static dev_forceGauge_state_E dev_forceGauge_private_getState(dev_forceGauge_channel_E channel)
{
    dev_forceGauge_state_E desiredState = dev_forceGauge_data[channel].state;
    switch (dev_forceGauge_data[channel].state)
    {
    case DEV_FORCEGAUGE_STATE_INIT:
        if (dev_forceGauge_data[channel].input.responding)
        {
            desiredState = DEV_FORCEGAUGE_STATE_RUNNING;
        }
        break;
    case DEV_FORCEGAUGE_STATE_RUNNING:
        if (dev_forceGauge_data[channel].input.responding == false)
        {
            desiredState = DEV_FORCEGAUGE_STATE_ERROR;
        }
        break;
    case DEV_FORCEGAUGE_STATE_ERROR:
        dev_forceGauge_data[channel].retryCount++;
        if (dev_forceGauge_data[channel].input.responding)
        {
            desiredState = DEV_FORCEGAUGE_STATE_RUNNING;
        }
        else if (dev_forceGauge_data[channel].retryCount > 3U)
        {
            desiredState = DEV_FORCEGAUGE_STATE_INIT;
        }
        break;
    default:
        break;
    }
    return desiredState;
}

static void dev_forceGauge_private_runAction(dev_forceGauge_channel_E channel)
{
    switch (dev_forceGauge_data[channel].state)
    {
    case DEV_FORCEGAUGE_STATE_INIT:
        dev_forceGauge_data[channel].output.ready = false;
        break;
    case DEV_FORCEGAUGE_STATE_RUNNING:
        dev_forceGauge_data[channel].output.force = (dev_forceGauge_data[channel].input.rawADC - dev_forceGauge_data[channel].nvram.zeroForceCount) / dev_forceGauge_data[channel].nvram.countPerForce;
        dev_forceGauge_data[channel].output.index++;
        dev_forceGauge_data[channel].output.ready = true;
        break;
    case DEV_FORCEGAUGE_STATE_ERROR:
        dev_forceGauge_data[channel].output.ready = false;
        break;
    default:
        break;
    }
}

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void dev_forceGauge_init(int lock)
{
    // load nvram
    // @TODO, we should really have 1 channel per nvram paraemeters per module
    // for example: 1 file for force gauge that updated the nvram struct directly
    // for now we will just load the machineCOnfig and parse data
    // in future we will have a separate nvram file for each module (servo etc)
    // then the machine configuration that is added to header can be aggregated
    // but the UI will be able to edit each module separately and wont reciever 1 massive struct
    // this will also allow for easier versioning of the nvram files
    // we could have dev_cogManager pass the nvram into the init and connect the files
    // then we can juse pass the data nvram struct as parameter

    MachineProfile machineProfile;
    dev_nvram_getChannelData(DEV_NVRAM_CHANNEL_MACHINE_PROFILE, &machineProfile, sizeof(MachineProfile));

    for (dev_forceGauge_channel_E channel = (dev_forceGauge_channel_E)0U; channel < DEV_FORCEGAUGE_CHANNEL_COUNT; channel++)
    {
        dev_forceGauge_data[channel].lock = lock;
        dev_forceGauge_data[channel].state = DEV_FORCEGAUGE_STATE_INIT;
        dev_forceGauge_data[channel].nvram.zeroForceCount = machineProfile.configuration.forceGaugeOffset; // @todo make this voltage :)
        dev_forceGauge_data[channel].nvram.countPerForce = machineProfile.configuration.forceGaugeGain;
    }
}

void dev_forceGauge_run()
{
    for (dev_forceGauge_channel_E channel = (dev_forceGauge_channel_E)0U; channel < DEV_FORCEGAUGE_CHANNEL_COUNT; channel++)
    {
        dev_forceGauge_private_processInputs(channel);
        dev_forceGauge_data[channel].state = dev_forceGauge_private_getState(channel);
        dev_forceGauge_private_runAction(channel);
    }
}

int32_t dev_forceGauge_getForce(dev_forceGauge_channel_E channel)
{
    return dev_forceGauge_data[channel].output.force;
}

uint32_t dev_forceGauge_getIndex(dev_forceGauge_channel_E channel)
{
    return dev_forceGauge_data[channel].output.index;
}

bool dev_forceGauge_isReady(dev_forceGauge_channel_E channel)
{
    return dev_forceGauge_data[channel].output.ready;
}

/**********************************************************************
 * End of File
 **********************************************************************/
