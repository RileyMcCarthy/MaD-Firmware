#ifndef WATCHDOG_CONFIG_H
#define WATCHDOG_CONFIG_H
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    WATCHDOG_CHANNEL_CONTROL,
    WATCHDOG_CHANNEL_COMMUNICATION,
    WATCHDOG_CHANNEL_MONITOR,
    WATCHDOG_CHANNEL_MOTOR,
    WATCHDOG_CHANNEL_LOGGER,
    WATCHDOG_CHANNEL_FORCEGAUGE,
    WATCHDOG_CHANNEL_SERIAL,
    WATCHDOG_CHANNEL_COUNT,
} watchdog_channel_t;

#endif // WATCHDOG_CONFIG_H
