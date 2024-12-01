#ifndef IO_CRC_H
#define IO_CRC_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include <stdint.h>
/**********************************************************************
 * Constants
 **********************************************************************/
#define LIB_UTILITY_UM_PER_MM 1000
#define LIB_UTILITY_MN_PER_N 1000

#define LIB_UTILITY_BIT_MASK_1 0x01
#define LIB_UTILITY_BIT_MASK_2 0x03
#define LIB_UTILITY_BIT_MASK_3 0x07
#define LIB_UTILITY_BIT_MASK_4 0x0F
#define LIB_UTILITY_BIT_MASK_5 0x1F
#define LIB_UTILITY_BIT_MASK_6 0x3F
#define LIB_UTILITY_BIT_MASK_7 0x7F
#define LIB_UTILITY_BIT_MASK_8 0xFF

#define LIB_UTILITY_CREATE_MASK(bits, shift, value) ((value & LIB_UTILITY_BIT_MASK_##bits) << shift)

#define LIB_UTILITY_SET_BIT(value, bit) ((value) |= (1 << (bit)))
#define LIB_UTILITY_SET_BITS(value, bits, shift, bitValue) ((value) = ((value) & ~(LIB_UTILITY_BIT_MASK_##bits << shift)) | ((bitValue & LIB_UTILITY_BIT_MASK_##bits) << shift))
#define LIB_UTILITY_CLEAR_BIT(value, bit) ((value) &= ~(1 << (bit)))
#define LIB_UTILITY_TOGGLE_BIT(value, bit) ((value) ^= (1 << (bit)))
#define LIB_UTILITY_GET_BIT(value, bit) (((value) >> (bit)) & 0x01)

/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/

/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
uint8_t lib_utility_CRC8(uint8_t *addr, uint16_t len);
/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* IO_CRC_H */
