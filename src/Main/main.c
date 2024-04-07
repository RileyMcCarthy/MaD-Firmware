#include "Main/MaD.h"
#include <stdint.h>
#include <propeller2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum
{
    HEAPSIZE = 32400 * 4
};

#ifdef __EMULATION__
int main() {
    setbuf(stdout, NULL);
    init_simulator();
    mad_begin();
    return 0;
}
#else
/**
 * @brief Main method that is called on program startup.
 * Begins MaD Board instance
 *
 * @return int
 */
// all stdio must be done in main thread (or same cog)
int main()
{
    mad_begin();
    while (1)
        ;
    return 0;
}
#endif
