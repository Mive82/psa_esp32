#ifndef PSA_VAN_DASHBOARD_STRUCT_H
#define PSA_VAN_DASHBOARD_STRUCT_H

#include <stdint.h>

/**
 * @brief Dashboard struct. Ident 0x8A4. PSA_VAN_IDEN_ENGINE
 *
 */
struct psa_van_dashboard
{
    uint8_t brightness : 4; // 0 - 15
    uint8_t wiper : 1;
    uint8_t : 2;
    uint8_t is_backlight_off : 1; // 1 when light position is off

    uint8_t accesories_on : 1;   // Key in ACC
    uint8_t ignition_on : 1;     // Key in IGN
    uint8_t engine_running : 1;  // Engine on
    uint8_t door_open : 1;       // A door is open
    uint8_t economy_mode : 1;    // Engine in economy mode
    uint8_t reverse_gear : 1;    // Gearbox in reverse
    uint8_t trailer_present : 1; // A trailer is hooked up
    uint8_t : 1;

    uint8_t water_temp;    // Subtract by 40
    uint32_t mileage : 24; // Should be 3 bytes in big endian format, divide by 10 to get value
    uint8_t external_temp; // 0xff if sensor not present (Subtract 0x50 and divide by 2 to get celcius value)

} __attribute__((packed));

/**
 * @brief Instrument cluster struct. Ident 0x4FC. Len 11. PSA_VAN_IDEN_LIGHTS_STATUS
 *
 */
struct psa_van_instrument_cluster_short
{
    uint8_t cluster_lights_1;
    uint8_t door_led;
    uint16_t km_to_service;
    uint8_t auto_gearbox_gear;
    uint8_t light_indicators;
    uint8_t oil_temp;
    uint8_t fuel_level;
    uint8_t oil_level; // 0 - 254
    uint8_t unknown_1;
    uint8_t lpg_level;
} __attribute__((packed));

/**
 * @brief Instrument cluster struct. Ident 0x4FC. Len 14. PSA_VAN_IDEN_LIGHTS_STATUS
 *
 */
struct psa_van_instrument_cluster_long
{
    uint8_t cluster_lights_1;
    uint8_t door_led;
    uint16_t km_to_service;
    uint8_t auto_gearbox_gear;
    uint8_t light_indicators;
    uint8_t oil_temp;
    uint8_t fuel_level;
    uint8_t oil_level; // 0 - 254
    uint8_t unknown_1;
    uint8_t lpg_level;
    uint8_t cruise_control;
    uint8_t cruise_control_speed;
    uint8_t unknown_2;
} __attribute__((packed));

#endif