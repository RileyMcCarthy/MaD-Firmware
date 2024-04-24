#include "Encoder.h"
#include <stdlib.h>
#include <propeller2.h>
#include "SocketIO.h"
#include "Utility/Debug.h"

void encoder_start(Encoder *self, uint8_t pinA, uint8_t pinB, uint8_t btn, bool d4x, int32_t preset, int32_t lo, int32_t hi)
{
    if (self != NULL)
    {
        self->socket_a = get_pin_socketid(pinA);
        self->socket_b = get_pin_socketid(pinB);
    }
}
int32_t encoder_value(Encoder *self)
{
    int32_t value = 0;
    if (self != NULL)
    {
        char recv_a[1];
        char recv_b[1];
        int bytes_received_a;
        int bytes_received_b;
        do {
            bytes_received_a = socketio_receive(self->socket_a, recv_a, 1);
            bytes_received_b = socketio_receive(self->socket_b, recv_b, 1);
            if ((bytes_received_a > 0) && (bytes_received_b > 0)) {
                // Process the data
                if (recv_a[0] == 0x01 && recv_b[0] == 0x00) {
                    value++;
                }
                else if (recv_a[0] == 0x00 && recv_b[0] == 0x01) {

                    value--;
                }
                else
                {
                    // Invalid data
                    DEBUG_WARNING("Invalid quad encoder data %d:%d\n", recv_a[0], recv_b[0]);
                }
            }
        } while ((bytes_received_a > 0) && (bytes_received_b > 0));
    }
    self->value += value;
    return self->value;
}
void encoder_set(Encoder *self, int32_t value)
{
    if (self == NULL)
    {
        return;
    }
}

