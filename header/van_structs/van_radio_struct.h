#ifndef VAN_RADIO_STRUCTS_H
#define VAN_RADIO_STRUCTS_H

// Packet iden 0x554

enum psa_van_radio_band
{
    PSA_VAN_BAND_NONE = 0,
    PSA_VAN_BAND_FM1,
    PSA_VAN_BAND_FM2,
    PSA_VAN_BAND_FM3,
    PSA_VAN_BAND_FMAST,
    PSA_VAN_BAND_AM,
    PSA_VAN_BAND_PTY_SELECT,
};

struct psa_van_radio_freq_info_scan_info
{
    uint8_t : 3;
    uint8_t manual_scan_in_progress : 1;
    uint8_t freq_scan_is_running : 1;
    uint8_t : 2;
    uint8_t freq_scan_direction : 1;
} __attribute__((packed));

struct psa_van_radio_freq_info_ta_rds_flags
{
    uint8_t ta_active : 1;
    uint8_t rds_active : 1;
    uint8_t : 3;
    uint8_t rds_data_present : 1;
    uint8_t ta_data_present : 1;
    uint8_t : 1;
} __attribute__((packed));

// Len 22
struct psa_van_radio_freq_info
{
    uint8_t header;                                           // 0
    uint8_t data_type;                                        // Should be 0xD1 for frequency info // 1
    uint8_t band : 3;                                         // 2
    uint8_t memory_position : 5;                              // 2
    struct psa_van_radio_freq_info_scan_info scan_info;       // 3
    uint8_t frequency[2];                                     // 4,5
    uint8_t signal_info;                                      // 6 AND with 0x0F to get signal strength, not sure if accurate
    struct psa_van_radio_freq_info_ta_rds_flags rds_ta_flags; // 7
    uint8_t unknown2[4];                                      // 8,9,10,11
    uint8_t station[9];                                       // 12,13,14,15,16,17,18,19,20
    uint8_t footer;
} __attribute__((packed));

// Len 10, didn't appear in my car
struct psa_van_radio_cd_info_len_10
{
    uint8_t header;
    uint8_t data_type; // Should be 0xD6 for CD track info
    uint8_t shuffle;
    uint8_t status; // loading, error, etc... to research
    uint8_t unknown1;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t current_track;
    uint8_t total_tracks;
    uint8_t footer;
} __attribute__((packed));

// Len 19
struct psa_van_radio_cd_info_len_19
{
    uint8_t header;
    uint8_t data_type; // Should be 0xD6 for CD track info
    uint8_t shuffle;
    uint8_t status; // loading, error, etc... to research
    uint8_t unknown1;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t current_track;
    uint8_t total_tracks;
    uint8_t total_minutes;
    uint8_t total_seconds;
    uint8_t unknown2[7];
    uint8_t footer;
} __attribute__((packed));

// Len 12, didn't appear in my car
struct psa_van_radio_preset_info
{
    uint8_t header;
    uint8_t data_type;       // Should be D3 for preset info
    uint8_t position : 4;    // Preset position
    uint8_t : 4;             // Unknown
    uint8_t station_name[8]; // ASCII, not NULL-terminated
    uint8_t footer;
} __attribute__((packed));

// int a = sizeof(struct psa_van_radio_preset_info);
#endif