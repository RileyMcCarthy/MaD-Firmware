#ifndef IOBoard_H
#define IOBoard_H

#include "MCP23017.h"

#define SERVO_SRDY 14 //Servo ready status (Input)
#define SERVO_DIR 12  //Step direction (Output)
#define SERVO_PUL 13  //Step pulse (Output)
#define SERVO_ENA 11  //Servo enable (Output)

#define SD_CD 4   //Data SD card CD
#define SD_CMD 5  //Data SD card CMD
#define SD_CLK 6  //Data SD card clock
#define SD_DAT0 7 //Data SD card data 0

#define RTC_SCL 0  //clock for real time clock and IO expander I2C (Output)
#define RTC_SDA 26 //data for real time clock and IO expander I2C (Input/Output)

#define I2C_SCL 28 //clock for general I2C (Output)
#define I2C_SDA 29 //data for general I2C (Input/Output)

#define DST_A 19 //A for distance differential encoder (Input)
#define DST_B 20 //B for distance differential encoder (Input)
#define DST_Z 21 //Z for distance differential encoder (Input)
#define DST_ZI 8 //ZI for distance differential encoder (Input)

#define FORCE_OVLD 17 //overload status of force gauge (Input)
#define FORCE_RX 15   //force gauge data (Input)
#define FORCE_TX 16   //force gauge data (Output)

#define GPIO_SDA 26               //MCP23017 (GPIO Expander) SDA pin
#define GPIO_SCL 0                //MCP23017 (GPIO Expander) SCL pin
#define GPIO_ADDR 0               //MCP23017 (GPIO Expander) I2C Address
#define GPIO_DIR_MASKA 0b11110000 //MCP23017 (GPIO Expander) direction mask A
#define GPIO_DIR_MASKB 0b11111111 //MCP23017 (GPIO Expander) direction mask B

#define CHARGE_PUMP_OUT 1
#define DISTANCE_LIMIT_MAX 1
#define DISTANCE_LIMIT_MIN 2

/**
 * @brief Configures MCP23017 (GPIO Expander)
 * 
 * @param gpio Pointer to MCP23017 (GPIO Expander) instance
 */
static void gpioConfig(MCP23017 *gpio)
{
    gpio->begin(GPIO_ADDR, GPIO_SDA, GPIO_SCL); // use default address 0
    gpio->dira(GPIO_DIR_MASKA);
    gpio->dirb(GPIO_DIR_MASKB);
}

/**
 * @brief Gets input from specified input
 * 
 * @param gpio Pointer to MCP23017 instance
 * @param pin Pin to get input from
 * @return uint8_t Pin status: 0 = LOW, 1 = HIGH
 */
static uint8_t gpi(MCP23017 *gpio, int pin)
{
    if (pin >= 9 && pin <= 12) //reg a
    {
        return (gpio->outa() & (1 << (pin - 9))) >> (pin - 9);
    }
    else if (pin >= 1 && pin <= 8) //reg b
    {
        return (gpio->outb() & (1 << (pin - 1))) >> (pin - 1);
    }
    return NULL;
}

#endif