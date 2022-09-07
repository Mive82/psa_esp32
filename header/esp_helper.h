#ifndef ESP_HELPER_H
#define ESP_HELPER_H

#include "psa_common.h"

enum psa_voltmeter_resolution
{
    PSA_VOLTMETER_INVALID = -1,
    PSA_VOLTMETER_11_BIT = 0,
    PSA_VOLTMETER_12_BIT = 1,
};

/**
 * @brief Attempts to read the internal temperature of the ESP32.
 *
 * @return float Temp in Celcius
 */
extern float read_esp32_temperature();

/**
 * @brief Configures the NTP client.
 *
 * @param server1 First NTP server
 * @param server2 Second NTP server (optional)
 * @param server3 Third NTP server (optional)
 */
extern void ntp_config(const char *server1, const char *server2 = NULL, const char *server3 = NULL);

/**
 * @brief Set the time zone
 * Time zone offset is negative, so GMT +1 would be -3600, not 3600.
 *
 * @param offset Time zone offset in seconds (negative)
 * @param dst_offset Daylight savings time offset (positive).
 */
extern void set_time_zone(long offset, int dst_offset);

/**
 * @brief Start the ADC used as a voltmeter with the specified resolution.
 *
 * @param pin GPIO pin to use
 * @param resolution ADC resolution (enum psa_voltmeter_resolution)
 * @return enum psa_status
 */
// extern enum psa_status init_voltmeter(uint8_t pin, enum psa_voltmeter_resolution resolution);

/**
 * @brief Read the analog input voltage with voltage divider resistiors R1 and R2.
 * Returns voltage in milivolts.
 *
 *
 * @param pin Input GPIO pin
 * @param R1 R1 value in ohms
 * @param R2 R2 value in ohms
 * @return uint16_t voltage in milivolts
 */
// extern uint16_t read_voltmeter(uint8_t pin, int R1, int R2);

#endif