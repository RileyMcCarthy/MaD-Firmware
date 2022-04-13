#define P2_TARGET_MHZ 180
#include <sys/p2es_clock.h>
#include "MaD.h"
enum
{
    HEAPSIZE = 32768 * 5
};

/**
 * @brief Main method that is called on program startup.
 * Begins MaD Board instance
 *
 * @return int
 */

int main()
{
    _clkset(_SETFREQ, _CLOCKFREQ);
    mad_begin();
    while (1)
        ;
    return 0;
}
