#include "ForceGauge.h"
#include <math.h>
#include <propeller2.h>
#include <smartpins.h>
#include "Utility/Debug.h"

#define CONFIG_0 0x00 // Configuration register 0
#define CONFIG_1 0x01 // Configuration register 1
#define CONFIG_2 0x02 // Configuration register 2
#define CONFIG_3 0x03 // Configuration register 3
#define CONFIG_4 0x04 // Configuration register 4

// 0bxxx first three determines speed
#define CONFIG_DATA1 0b11001000
#define CONFIG_DATA2 0b00000000
#define CONFIG_DATA3 0b00000001
#define CONFIG_DATA4 0b01110111

#define BAUD 115200

union Data_v
{
    int32_t val;
    uint8_t bval[4];
};

#define FORCE_MEMORY_SIZE 1024
static long force_stack[FORCE_MEMORY_SIZE];

// Private Functions

static void write_register(ForceGauge *forceGauge, uint8_t reg, uint8_t data)
{
    // Write has format(rrr = reg to write): 0x55, 0001 rrrx {data}
    fds_tx(&(forceGauge->serial), 0x55);
    fds_tx(&(forceGauge->serial), 0x40 + (reg << 1));
    fds_tx(&(forceGauge->serial), data);
}

static uint8_t read_register(ForceGauge *forceGauge, uint8_t reg)
{
    // read has format(rrr = reg to read): 0x55, 0010 rrrx, {returned data}
    fds_tx(&(forceGauge->serial), 0x55);
    fds_tx(&(forceGauge->serial), 0x20 + (reg << 1));
    uint8_t temp = fds_rxtime(&(forceGauge->serial), 100);
    return temp;
}

static bool force_gauge_reset(ForceGauge *forceGauge)
{
    //DEBUG_ERROR("%s","Force Gauge Reset\n");
    fds_start(&(forceGauge->serial), forceGauge->rx, forceGauge->tx, 0, BAUD);
    fds_tx(&(forceGauge->serial), 0x55); // Synchronization word
    fds_tx(&(forceGauge->serial), 0x01); // RESET
    _waitms(100);
    fds_tx(&(forceGauge->serial), 0x55); // Synchronization word
    fds_tx(&(forceGauge->serial), 0x06); // RESET
    _waitms(100);
    write_register(forceGauge, CONFIG_1, CONFIG_DATA1); // Setting data mode to continuous
    write_register(forceGauge, CONFIG_2, CONFIG_DATA2); // Setting data counter on
    write_register(forceGauge, CONFIG_3, CONFIG_DATA3); // Setting data counter on
    //write_register(forceGauge, CONFIG_4, CONFIG_DATA4); // Setting data counter on CHANGED

    fds_flush(&(forceGauge->serial));

    int temp;
    if ((temp = read_register(forceGauge, CONFIG_1)) != CONFIG_DATA1)
    {
        _waitms(100);
        fds_stop(&(forceGauge->serial));
        DEBUG_ERROR("Force Gauge not responding, expected %d, got %d\n", CONFIG_DATA1, temp);
        return false;
    }

    fds_tx(&(forceGauge->serial), 0x55); // Synchronization word
    fds_tx(&(forceGauge->serial), 0x08);

    _waitms(100);
    fds_stop(&(forceGauge->serial));
    DEBUG_ERROR("%s","Force Gauge Reset Complete\n");
    return true;
}

// Can be updated to use simpleSerial.spin2? inside spin folder of flexprop
static void continuous_data(void *arg)
{
    ForceGauge *forceGauge = (ForceGauge *)arg;
    int rx = forceGauge->rx;
    int tx = forceGauge->tx;
    int data = 0;
    int index = 0;
    int delay = (_clockfreq() / BAUD) / 2;
    int spmode = P_ASYNC_RX;
    int baudcfg = 7 + ((_clockfreq() / BAUD) << 16);
    long transmittx = delay * 35 * 2;
    long disconnecttx = _clockfreq(); // 1000ms before considered disconnected
    _pinclear(rx);
    _pinstart(rx, spmode, baudcfg, 0);
    uint32_t lastData = _cnt();
    while (1)
    {
        // need to ifdef under emulator
        if (_cogrunning(forceGauge->cogid) == false)
        {
            DEBUG_ERROR("%s","Force gauge cog stopped\n");
            break;
        }
        while (!_pinr(rx))
        {
            if ((_cnt() - lastData) > disconnecttx)
            {
                forceGauge->responding = false;
                DEBUG_INFO("force gauge reconnecting after timeout %d/%ld\n", (_cnt() - lastData), disconnecttx);
                _pinclear(rx);
                while (force_gauge_reset(forceGauge) == false)
                {
                    // trying again
                    _waitms(100);
                }
                _pinstart(rx, spmode, baudcfg, 0);
                forceGauge->responding = true;
                lastData = _cnt();
                continue;
            }
            else if ((_cnt() - lastData) > transmittx && index > 0)
            {
                index = 0;
                data = 0;
            }
        }
        lastData = _cnt();
        uint8_t byte = (_rdpin(rx) >> 24) & 0xFF;
        data |= (byte << index * 8); 
        index++;
        if (index >= 3)
        {
            forceGauge->responding = true;
            forceGauge->forceRaw = data;
            forceGauge->counter++;
            data = 0;
            index = 0;
            //DEBUG_INFO("Force: %d\n", forceGauge->forceRaw);
        }
    }
}

/**
 * @brief Begin the force gauge communication.
 * @note UART is LSB first
 * @todo Add data integrity check...
 * @param forceGauge The force gauge structure to start
 * @param rx serial rx pin
 * @param tx serial tx pin
 */
bool force_gauge_begin(ForceGauge *forceGauge, int rx, int tx)
{
    force_gauge_stop(forceGauge);
    forceGauge->rx = rx;
    forceGauge->tx = tx;
    
    forceGauge->responding = force_gauge_reset(forceGauge);

    if (forceGauge->responding)
    {
        forceGauge->cogid = _cogstart_C(continuous_data, forceGauge, &force_stack[0], sizeof(long) * FORCE_MEMORY_SIZE);
        forceGauge->responding &= forceGauge->cogid > 0;
    }
    return forceGauge->responding;
}

void force_gauge_stop(ForceGauge *forceGauge)
{
    if (forceGauge->cogid > 0)
    {
        _cogstop(forceGauge->cogid);
    }
}

// returns force in milliNewtons
int raw_to_force(int raw, int zero, int slope)
{
    return round((raw - zero) / slope);
}
