#ifndef VAN_HELPER_H
#define VAN_HELPER_H

#include <stdint.h>

/**
 * @brief Extract the 12 bit VAN iden from a 16 bit iden+command value
 *
 * @param byte1
 * @param byte2
 * @return uint16_t iden value
 */
extern uint16_t get_iden_from_bytes(uint8_t const byte1, uint8_t const byte2);

/**
 * @brief Convert binary-coded decimal to normal decimal number.
 * Returns converted value if conversion is valid, returns the original value otherwise.
 *
 * @param value Decimal value
 * @return uint8_t BCD value
 */
extern uint8_t bcd_to_dec(uint8_t value);

/**
 * @brief Convert a decimal number to a binary-coded decimal
 * Returns converted value if conversion is valid, returnes the original value otherwise.
 * Valid values are from 0 to 99. `0xFF` just returns `0xFF`.
 * If input is bigger than 99, it will be clamped to 99.
 * @param value BCD value
 * @return uint8_t Decimal value
 */
extern uint8_t dec_to_bcd(uint8_t value);

// extern void handle_cd_changer_data();

#endif