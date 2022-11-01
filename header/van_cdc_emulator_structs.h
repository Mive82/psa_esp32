#ifndef PSA_SIMULATE

#ifndef VAN_CDC_EMU_H
#define VAN_CDC_EMU_H

#include <stdint.h>
#include "psa_common.h"

enum van_cd_changer_status
{
    VAN_CDC_OFF = 0x41,
    VAN_CDC_ON_PAUSED = 0xC1,
    VAN_CDC_ON_BUSY = 0xD3,
    VAN_CDC_ON_PLAYING = 0xC3,
    VAN_CDC_ON_FAST_FORWARD = 0xC4,
    VAN_CDC_ON_FAST_REWIND = 0xC5,
};

enum van_cd_changer_cd_presence
{
    VAN_CDC_CD_PRESENT = 0x16,
    VAN_CDC_CD_NOT_PRESENT = 0x06,
};

struct van_cd_changer_packet
{
    uint8_t header;       // 0x80 to 0x87. Same as footer
    uint8_t shuffle;      // 1 if tracks are shuffled
    uint8_t status;       // enum van_cd_changer_status
    uint8_t cd_present;   // enum van_cd_changer_cd_presence
    uint8_t minutes;      // Track minute progress in bcd
    uint8_t seconds;      // Track seconds progress in the current minute in bcd
    uint8_t track_num;    // Number of the current track in bcd
    uint8_t cd_num;       // Number of the current CD in bcd
    uint8_t total_tracks; // Total number of tracks in bcd
    uint8_t unknown;
    uint8_t cd_flag; // bitwise representation of present CDs
    uint8_t footer;
} __attribute__((packed));

union van_cd_changer_union
{
    struct van_cd_changer_packet packet;
    uint8_t buffer[sizeof(packet)];
};

extern enum psa_status init_cdc_packet(
    struct van_cd_changer_packet *packet);

#endif // VAN_CDC_EMU_H

#endif // PSA_SIMULATE