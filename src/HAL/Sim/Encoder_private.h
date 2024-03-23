#include <stdint.h>
#include <stdbool.h>

typedef struct Encoder_e {
    uint8_t pinA;
    uint8_t pinB;
    uint8_t btn;
    bool d4x;
    int32_t preset;
    int32_t lo;
    int32_t hi;
    int32_t value;
} Encoder;
