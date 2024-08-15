#ifndef IO_TIMER_H
#define IO_TIMER_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdint.h>
#include <stdbool.h>
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef enum
{
    IO_TIMER_STATE_OFF,
    IO_TIMER_STATE_RUNNING,
    IO_TIMER_STATE_EXPIRED,
    IO_TIMER_STATE_COUNT,
} IO_timer_state_E;

typedef struct
{
    uint32_t startms;
    uint32_t periodms;
    IO_timer_state_E state;
} IO_timer_S;
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
void IO_timer_init(IO_timer_S *timer, uint32_t periodms);
void IO_timer_start(IO_timer_S *timer);
void IO_timer_stop(IO_timer_S *timer);
IO_timer_state_E IO_timer_state(IO_timer_S *timer);
bool IO_timer_expired(IO_timer_S *timer);
/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* IO_TIMER_H */
