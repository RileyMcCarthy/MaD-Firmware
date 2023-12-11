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
#include "SocketIO.h"
int main() {
    setbuf(stdout, NULL);
    
    int port = 1234;
    char address[64];
    strncpy(address, "127.0.0.1", 64);
    
    printf("Connecting to %s:%d\n", address, port);

    socketio_begin();
    socketio_set_address(address, port);

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
