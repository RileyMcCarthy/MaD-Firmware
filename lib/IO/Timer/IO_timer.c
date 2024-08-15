//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "IO_timer.h"
#include "propeller2.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/
#define IO_TIMER_MAX_MS 4294967295U
/**********************************************************************
 * External Variables
 **********************************************************************/

/**********************************************************************
 * Private Variable Definitions
 **********************************************************************/

/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

/**********************************************************************
 * Private Function Definitions
 **********************************************************************/
uint32_t IO_timer_private_getms()
{
    return _getms();
}
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

void IO_timer_init(IO_timer_S *timer, uint32_t periodms)
{
    timer->periodms = periodms;
    timer->state = IO_TIMER_STATE_OFF;
    timer->startms = 0U;
}

void IO_timer_start(IO_timer_S *timer)
{
    timer->startms = IO_timer_private_getms();
    timer->state = IO_TIMER_STATE_RUNNING;
}

void IO_timer_stop(IO_timer_S *timer)
{
    timer->startms = 0U;
    timer->state = IO_TIMER_STATE_OFF;
}

IO_timer_state_E IO_timer_state(IO_timer_S *timer)
{
    const uint32_t currentms = IO_timer_private_getms();
    switch (timer->state)
    {
    case IO_TIMER_STATE_OFF:
        break;
    case IO_TIMER_STATE_RUNNING:
        // If timer is uint32_t rollover is 49 days...
        if ((currentms - timer->startms) > timer->periodms)
        {
            timer->state = IO_TIMER_STATE_EXPIRED;
        }
        break;
    case IO_TIMER_STATE_EXPIRED:
    case IO_TIMER_STATE_COUNT:
    default:
        break;
    }
    return timer->state;
}

bool IO_timer_expired(IO_timer_S *timer)
{
    return IO_timer_state(timer) == IO_TIMER_STATE_EXPIRED;
}

/**********************************************************************
 * End of File
 **********************************************************************/
