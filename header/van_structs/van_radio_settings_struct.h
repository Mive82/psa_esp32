#ifndef PSA_VAN_RADIO_INFO_STRUCT_H
#define PSA_VAN_RADIO_INFO_STRUCT_H

#include <stdint.h>

// Radio audio settings 0x4D4 PSA_VAN_IDEN_AUDIO_SETTINGS

const uint8_t PSA_RADIO_INFO_SOURCE_RADIO = 0x51;
const uint8_t PSA_RADIO_INFO_SOURCE_CD = 0x52;

struct psa_radio_settings_byte_1
{
    uint8_t stalk_mute : 1;
    uint8_t external_mute : 1;
    uint8_t auto_volume : 1;
    uint8_t : 1;
    uint8_t loudness_on : 1;
    uint8_t audio_properties_menu_open : 1;
    uint8_t : 2;
} __attribute__((packed));

struct psa_radio_settings_byte_2
{
    uint8_t power_on : 1;
    uint8_t : 7;
} __attribute__((packed));

struct psa_radio_settings_byte_4
{
    uint8_t source : 4;
    uint8_t : 1;
    uint8_t tape_present : 1;
    uint8_t cd_present : 1;
    uint8_t : 1;
} __attribute__((packed));

struct psa_radio_settings_values
{
    uint8_t value : 7; // Add 0x3f to get actual value
    uint8_t updating : 1;
} __attribute__((packed));

/**
 * @brief Radio info struct. Ident 0x4D4
 *
 */
struct psa_van_radio_settings
{
    uint8_t header;
    struct psa_radio_settings_byte_1 audio_properties;
    struct psa_radio_settings_byte_2 power;
    uint8_t field3;
    struct psa_radio_settings_byte_4 source;
    struct psa_radio_settings_values volume;
    struct psa_radio_settings_values balance;
    struct psa_radio_settings_values fader;
    struct psa_radio_settings_values bass;
    struct psa_radio_settings_values treble;
    uint8_t footer;
} __attribute__((packed));

int a = sizeof(struct psa_van_radio_settings);

#endif