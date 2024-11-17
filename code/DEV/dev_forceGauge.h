#ifndef DEV_FORCEGAUGE_H
#define DEV_FORCEGAUGE_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "dev_forceGauge_config.h"
#include "IO_ADS122U04.h"

#include <stdint.h>
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/

// @todo: ADS122U04 should abstract to IO_ADC and use voltage instead of counts
// This will allow for different ADCs to be used in the future, fine for now :)

typedef enum
{
    DEV_FORCEGAUGE_STATE_INIT,
    DEV_FORCEGAUGE_STATE_RUNNING,
    DEV_FORCEGAUGE_STATE_ERROR,
    DEV_FORCEGAUGE_STATE_COUNT,
} dev_forceGauge_state_E;

typedef struct
{
    IO_ADS122U04_channel_E adcChannel;
} dev_forceGauge_channelConfig_S;

typedef struct
{
    int32_t zeroForceCount; // adc at zero force
    int32_t countPerForce;  // adc count per force
} dev_forceGauge_channelNVRAM_S;
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void dev_forceGauge_init(int lock);
void dev_forceGauge_run();

int32_t dev_forceGauge_getForce(dev_forceGauge_channel_E channel);
uint32_t dev_forceGauge_getIndex(dev_forceGauge_channel_E channel);
bool dev_forceGauge_isReady(dev_forceGauge_channel_E channel);

/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* DEV_FORCEGAUGE_H */
