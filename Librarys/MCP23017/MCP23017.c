#include "MCP23017.h"

// registers
#define REG_IODIRA 0x00 // Direction Register A
#define REG_IPOLA 0x02  // Polarity Register A
#define REG_GPPUA 0x0C  // Pull-up Register A
#define REG_GPIOA 0x12  // I/O Register A
#define REG_OLATA 0x14  // Output latch register A

#define REG_IODIRB 0x01 // Direction Register B
#define REG_IPOLB 0x03  // Polarity Register B
#define REG_GPPUB 0x0D  // Pull-up Register B
#define REG_GPIOB 0x13  // I/O Register B
#define REG_OLATB 0x15  // Output latch register B

/*Private functions*/
static uint8_t read_register(MCP23017 *mcp23017, uint8_t addr)
{
    uint8_t rdata = 0xFF;

    mcp23017->i2cBus.start();
    mcp23017->i2cBus.write(mcp23017->writeAddr);
    mcp23017->i2cBus.write(addr);
    mcp23017->i2cBus.start();
    mcp23017->i2cBus.write(mcp23017->readAddr);
    rdata = mcp23017->i2cBus.read(1);
    mcp23017->i2cBus.stop();
    return rdata;
}

static bool write_register(MCP23017 *mcp23017, uint8_t addr, uint8_t value)
{
    int ack = 0;

    mcp23017->i2cBus.start();
    ack = mcp23017->i2cBus.write(mcp23017->writeAddr);
    mcp23017->i2cBus.write(addr);
    mcp23017->i2cBus.write(value);
    mcp23017->i2cBus.stop();
    return ack == 0;
}

MCP23017 *mcp23017_create()
{
    return (MCP23017 *)malloc(sizeof(MCP23017));
}

void mcp23017_destroy(MCP23017 *mcp23017)
{
    mcp23017->i2cBus.stop();
    free(mcp23017);
}

/**
 * @brief 
 * 
 * @param addr 
 * @param theSDA 
 * @param theSCL 
 */
void mcp23017_begin(MCP23017 *mcp23017, uint8_t addr, int sda, int scl)
{
    mcp23017->i2cBus.setup(scl, sda, 100, 1); //1.5k pullup
    mcp23017->writeAddr = ((0x20 | addr) << 1) & 0b11111110;
    mcp23017->readAddr = ((0x20 | addr) << 1) | 0b00000001;
}

void mcp_set_direction(MCP23017 *mcp23017, uint16_t pin, uint8_t direction)
{
    int reg = REG_IODIRA;
    if (pin > 7)
    { //register B
        reg = REG_IODIRB;
        pin -= 8;
    }

    int value = read_register(mcp23017, reg);
    bitWrite(value, pin, direction);
    bool ack = write_register(mcp23017, reg, value);
}

uint8_t mcp_get_direction(MCP23017 *mcp23017, uint16_t pin)
{
    int reg = REG_IODIRA;
    if (pin > 7)
    { //register B
        reg = REG_IODIRB;
        pin -= 8;
    }
    return bitRead(read_register(mcp23017, reg), pin);
}

void mcp_set_pin(MCP23017 *mcp23017, uint16_t pin, uint8_t state)
{
    int reg = REG_GPIOA;
    if (pin > 7)
    { //register B
        reg = REG_GPIOB;
        pin -= 8;
    }

    int value = read_register(mcp23017, reg);
    bitWrite(value, pin, state);
    write_register(mcp23017, reg, value);
}
uint8_t mcp_get_pin(MCP23017 *mcp23017, uint16_t pin)
{
    int reg = REG_GPIOA;
    if (pin > 7)
    { //register B
        reg = REG_GPIOB;
        pin -= 8;
    }
    return bitRead(read_register(mcp23017, reg), pin);
}

void mcp_set_pullup(MCP23017 *mcp23017, uint16_t pin, uint8_t state)
{
    int reg = REG_IPOLA;
    if (pin > 7)
    { //register B
        reg = REG_IPOLB;
        pin -= 8;
    }

    int value = read_register(mcp23017, reg);
    bitWrite(value, pin, state);
    write_register(mcp23017, reg, value);
}
uint8_t mcp_get_pullup(MCP23017 *mcp23017, uint16_t pin)
{
    int reg = REG_IPOLA;
    if (pin > 7)
    { //register B
        reg = REG_IPOLB;
        pin -= 8;
    }

    return bitRead(read_register(mcp23017, reg), pin);
}
