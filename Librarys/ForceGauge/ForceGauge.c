#include "ForceGauge.h"
#include "simpletools.h"

#define CONFIG_0 0x00 // Configuration register 0
#define CONFIG_1 0x01 // Configuration register 1
#define CONFIG_2 0x02 // Configuration register 2
#define CONFIG_3 0x03 // Configuration register 3
#define CONFIG_4 0x04 // Configuration register 4

union Data_v
{
    float fval;
    int32_t val;
    uint8_t bval[4];
};

// Private Functions

static void write_register(ForceGauge *forceGauge, uint8_t reg, uint8_t data)
{
    // Write has format(rrr = reg to write): 0x55, 0b0001 rrrx {data}
    // uart_write(&(forceGauge->serial), 0x55); @todo implement serial
    // uart_write(&(forceGauge->serial), 0x40 + (reg << 1));
    // uart_write(&(forceGauge->serial), data);
    forceGauge->serial.tx(0x55);
    forceGauge->serial.tx(0x40 + (reg << 1));
    forceGauge->serial.tx(data);
}

static uint8_t read_register(ForceGauge *forceGauge, uint8_t reg)
{
    uint8_t temp;
    // read has format(rrr = reg to read): 0x55, 0b0010 rrrx, {returned data}
    // uart_write(&(forceGauge->serial), 0x55);@todo implement serial
    // uart_write(&(forceGauge->serial), 0x20 + (reg << 1));
    // uart_read(&(forceGauge->serial), 1, &temp);
    forceGauge->serial.tx(0x55);
    forceGauge->serial.tx(0x20 + (reg << 1));
    temp = forceGauge->serial.rxtime(10);
    return temp;
}
ForceGauge *force_gauge_create()
{
    ForceGauge *forceGauge = (ForceGauge *)malloc(sizeof(ForceGauge));
    return forceGauge;
}

void force_gauge_destroy(ForceGauge *forceGauge)
{
    free(forceGauge);
}

/**
 * @brief Begin the force gauge communication.
 * @note UART is LSB first
 * @todo Add data integrity check...
 * @param forceGauge The force gauge structure to start
 * @param rx serial rx pin
 * @param tx serial tx pin
 * @return Error: FORCEGAUGE_NOT_RESPONDING if communications fails, FORCEEGAUGE_COG_FAIL if cog fails to start, SUCCESS otherwise.
 */
Error force_gauge_begin(ForceGauge *forceGauge, int rx, int tx, int slope, int zero)
{
    forceGauge->interpolationSlope = slope;
    forceGauge->interpolationZero = zero;
    // uart_start(&(forceGauge->serial), rx, tx, 2, 57600); @todo implement serial
    // uart_write(&(forceGauge->serial), 0x55); //Synchronization word
    // uart_write(&(forceGauge->serial), 0x06); //Reset
    forceGauge->serial.start(rx, tx, 2, 57600);
    forceGauge->serial.tx(0x55); // Synchronization word
    forceGauge->serial.tx(0x06); // Reset
    _waitms(10);

    write_register(forceGauge, CONFIG_1, 0b00001000); // Setting data mode to continuous
    write_register(forceGauge, CONFIG_3, 0b00000001); // Setting data aquisition to automatic

    // uart_write(&(forceGauge->serial), 0x55); @todo implement serial
    // uart_write(&(forceGauge->serial), 0x08);
    forceGauge->serial.tx(0x55);
    forceGauge->serial.tx(0x08);
    return SUCCESS;
}

int force_gauge_get_force(ForceGauge *forceGauge)
{
    int forceRaw = forceGauge->serial.rxtime(1000);
    if (forceRaw == -1)
    {
        printf("Force read timeout\n");
        return forceRaw;
    }
    int force = (forceRaw - forceGauge->interpolationZero) / forceGauge->interpolationSlope;
    return forceRaw;
}
