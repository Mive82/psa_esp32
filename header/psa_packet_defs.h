#ifndef PSA_PACKET_DEFS_H
#define PSA_PACKET_DEFS_H

#include <stdint.h>

enum psa_idents
{
    PSA_IDENT_VERSIONS = 0x4000,      // Send API and firmware version numbers
    PSA_IDENT_VIN = 0x4001,           // Send the VIN packet
    PSA_IDENT_ENGINE = 0x4002,        // Send the Engine packet
    PSA_IDENT_RADIO = 0x4003,         // Send the Radio packet
    PSA_IDENT_RADIO_PRESETS = 0x4004, // Sent the Presets packet
    PSA_IDENT_DOORS = 0x4005,         // Send the Doors packet
    PSA_IDENT_DASHBOARD = 0x4006,     // Send the Dashboard packet
    PSA_IDENT_TRIP = 0x4007,          // Send the Trip packet
    PSA_IDENT_HEADUNIT = 0x4008,      // Send the Headunit packet
    PSA_IDENT_CAR_STATUS = 0x4009,    // Send the Car status packet
    PSA_IDENT_CD_PLAYER = 0x4010,     // Send the CD player packet

    PSA_IDENT_SET_CD_CHANGER_DATA = 0x4501, // Send CD changer player data
    PSA_IDENT_SET_TRIP_RESET = 0x4502,      // Send trip reset request

    PSA_IDENT_TIME = 0x5000,       // Send the ESP RTC time
    PSA_IDENT_ESP32_TEMP = 0x5100, // Send the ESP temperature, borked

    PSA_IDENT_MAIN_APP = 0x6000, // Special ident used for my app, sends all packets as one packet. UNUSED
};

enum psa_msp_response
{
    PSA_MSP_OK = 0x0E,                // Incoming packet parsed successfully
    PSA_MSP_CRC_ERROR = 0xAA,         // Incoming packet has invalid CRC
    PSA_MSP_UNKNOWN_IDENT = 0xBB,     // Incoming packet has unknown ident
    PSA_MSP_INVALID_DATA_SIZE = 0xCC, // Received data size is not valid for sent ident
    PSA_MSP_INVALID_DATA = 0xDD,      // Received data is invalid, possibly out of bounds
    PSA_MSP_UNKNOWN_ERROR = 0xFF,     // Worst case scenario error, shouldn't appear normally
};

enum psa_radio_band
{
    PSA_NONE = 0,
    PSA_AM_1 = 1, // AM 1 band
    PSA_AM_2,     // AM 2 band
    PSA_AM_3,     // AM 3 band
    PSA_FM_1,     // FM 1 band
    PSA_FM_2,     // FM 2 band
    PSA_FM_3,     // FM 3 band
    PSA_FM_AST    // FMast band
};

enum psa_radio_source
{
    PSA_RADIO_NONE = (uint8_t)0,
    PSA_RADIO_TUNER = (uint8_t)1,      // Radio tuner
    PSA_RADIO_INTERNAL = (uint8_t)2,   // Internal CD or tape player
    PSA_RADIO_EXTERNAL = (uint8_t)3,   // CD changer
    PSA_RADIO_NAVIGATION = (uint8_t)4, // Navigation audio
};

/**
 * @brief Check the Documentation for an explanation
 *
 */
enum psa_car_state
{
    PSA_STATE_CAR_SLEEP = 0, // Car in sleep, no power to VAN+
    PSA_STATE_ECONOMY_MODE,  // Car is in Battery-saving mode
    PSA_STATE_CAR_LOCKED,    // Car off and doors locked
    PSA_STATE_CAR_OFF,       // Car off, radio off
    PSA_STATE_RADIO_ON,      // Car off, radio on
    PSA_STATE_ACCESSORY,     // Key in accessory position
    PSA_STATE_IGNITION,      // Key in ignition position
    PSA_STATE_CRANKING,      // Engine cranking
    PSA_STATE_ENGINE_ON,     // Engine ON.
    PSA_STATE_UNKNOWN,       // Some logic is wrong, this should not appear
};

enum psa_trip_meter
{
    PSA_TRIP_A = 1,
    PSA_TRIP_B = 2,
};

/**
 * @brief Doesnt work at the moment
 *
 */
enum psa_cd_player_status
{
    PSA_CD_PLAYER_PLAYING = (uint8_t)(1 << 0),
    PSA_CD_PLAYER_PAUSED = (uint8_t)(1 << 1),
    PSA_CD_PLAYER_FAST_FORWARDING = (uint8_t)(1 << 2),
    PSA_CD_PLAYER_REWINDING = (uint8_t)(1 << 3),
    PSA_CD_PLAYER_LOADING = (uint8_t)(1 << 4),
    PSA_CD_PLAYER_EJECTING = (uint8_t)(1 << 5),
    PSA_CD_PLAYER_ERROR = (uint8_t)(1 << 6),
};

enum psa_cd_changer_status
{
    PSA_CD_CHANGER_OFF = (uint8_t)0x41,
    PSA_CD_CHANGER_ON = (uint8_t)0xC1,
    PSA_CD_CHANGER_BUSY = (uint8_t)0xD3,
    PSA_CD_CHANGER_PLAYING = (uint8_t)0xC3,
    PSA_CD_CHANGER_FAST_FORWARD = (uint8_t)0xC4,
    PSA_CD_CHANGER_FAST_REWIND = (uint8_t)0xC5,
};

enum psa_cd_changer_commands
{
    PSA_CD_CHANGER_COMM_NONE = 0x00,       // No new command
    PSA_CD_CHANGER_COMM_PLAY = 0x10,       // Command to start playback
    PSA_CD_CHANGER_COMM_PAUSE = 0x11,      // Command to pause playback
    PSA_CD_CHANGER_COMM_PREV_TRACK = 0x20, // Command to go to previous track
    PSA_CD_CHANGER_COMM_NEXT_TRACK = 0x21, // Command to go to next track
};

struct psa_header
{
    /* Packet start, 0x69 */
    uint8_t start;
    /* < when sending to the esp, > when receiving from the esp */
    uint8_t direction;
    /* Packet ident. Taken into CRC calc */
    uint16_t ident;
    /* Payload size. Taken into CRC calc */
    uint8_t size;
} __attribute__((packed));

struct psa_version_data
{
    uint8_t firmware_version_major;
    uint8_t firmware_version_minor;
    uint8_t api_version;
} __attribute__((packed));

struct psa_version_packet
{
    struct psa_header header;
    struct psa_version_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_engine_data
{
    uint16_t rpm;               // RPM * 8
    uint16_t speed;             // km/h * 100
    uint8_t fuel_level;         // Fuel level in %. Does not appear on all cars
    uint8_t engine_temperature; // Engine water temperature + 40 in Celcius. To display, subtract 40. 0xFF when invalid
    uint8_t reverse;            // Is the car in reverse
    uint8_t oil_temperature;    // Oil temperature + 40 in Celcius. 0xFF when invalid
    uint8_t reserved;           // reserved
} __attribute__((packed));

struct psa_engine_packet
{
    struct psa_header header;
    struct psa_engine_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_radio_data
{
    uint16_t freq;           // freq * 100
    uint8_t band;            // Radio band, enum psa_radio_band
    uint8_t preset;          // Preset number 1 - 6
    uint8_t signal_strength; // Signal Strength 0 - 15, probably. The values are random
    char station[9];         // ASCII station name. Do not treat as NULL-terminated.
} __attribute__((packed));

struct psa_radio_packet
{
    struct psa_header header;
    struct psa_radio_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_preset_info
{
    uint8_t preset_num;   // Preset number 1 - 6
    char preset_name[10]; // Preset station name
} __attribute__((packed));

struct psa_preset_data
{
    struct psa_preset_info presets[6];
} __attribute__((packed));

struct psa_preset_packet
{
    struct psa_header header;
    struct psa_preset_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_cd_player_data
{
    uint8_t status; // enum psa_cd_player_status

    uint8_t current_minute; // Current minute in decimal
    uint8_t total_minutes;  // Total minuts in decimal
    uint8_t current_second; // Current second in decimal
    uint8_t total_seconds;  // Not total_minutes*60, but ss in mm:ss. In decimal
    uint8_t current_track;  // Current track number in decimal
    uint8_t total_tracks;   // Total number of tracks in decimal
} __attribute__((packed));

struct psa_cd_player_packet
{
    struct psa_header header;
    struct psa_cd_player_data data;
    uint8_t crc;
} __attribute__((packed));

/**
 * @brief What settings are being changed (selected).
 * Used for displaying audio settings display
 *
 */
struct psa_headunit_settings
{
    uint8_t volume_changing : 1;      // The volume is changing. Not used
    uint8_t bass_changing : 1;        // The bass value is changing / selected
    uint8_t treble_changing : 1;      // The treble value is changing / selected
    uint8_t fader_changing : 1;       // The fader value is changing / selected
    uint8_t balance_changing : 1;     // The balance value is changing / selected
    uint8_t auto_volume_changing : 1; // The auto-volume value is changing / selected
    uint8_t loudness_changing : 1;    // The LOUDNESS value is changing / selected
    uint8_t : 1;                      // Unused
} __attribute__((packed));

struct psa_headunit_data
{
    uint8_t unit_powered_on;    // 1 if powered on
    uint8_t source;             // enum psa_radio_source
    uint8_t muted;              // is muted
    uint8_t cd_present;         // is CD present
    uint8_t tape_present;       // is TAPE present
    uint8_t volume;             // Current volume
    int8_t bass;                // Current bass
    int8_t treble;              // Current treble
    int8_t fader;               // Current fader
    int8_t balance;             // Current balance
    uint8_t auto_volume;        // Current auto volume setting
    uint8_t loudness_on;        // Current loudness setting
    uint8_t settings_menu_open; // Is settings menu open. Not always accurate.

    struct psa_headunit_settings settings_changing; // What settings are being updated

} __attribute__((packed));

struct psa_headunit_packet
{
    struct psa_header header;
    struct psa_headunit_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_door_data
{
    uint8_t door_open; // Is a door open?

    uint8_t open_doors; // What doors are open. Check below comment for clarification

    /*
        Open doors bitfields:

        uint8_t fuel_flap : 1;   // bit 0
        uint8_t sunroof : 1;     // bit 1
        uint8_t hood : 1;        // bit 2
        uint8_t boot_lid : 1;    // bit 3
        uint8_t rear_left : 1;   // bit 4
        uint8_t rear_right : 1;  // bit 5
        uint8_t front_left : 1;  // bit 6
        uint8_t front_right : 1; // bit 7
    */
} __attribute__((packed));

struct psa_door_packet
{
    struct psa_header header;
    struct psa_door_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_dash_data
{
    uint8_t brightness;        // 0 to 15. Dashboard brightness
    uint8_t backlight_off : 1; // 1 when interior backlight is off

    uint8_t accesories_on : 1;  // Key in ACC
    uint8_t ignition_on : 1;    // Key in IGN
    uint8_t engine_running : 1; // Engine on
    uint8_t door_open : 1;      // A door is open
    uint8_t economy_mode : 1;   // ECO MODE for the radio.
    uint8_t reverse_gear : 1;   // Gearbox in reverse

    uint8_t water_temp;    // Subtract by 40
    uint32_t mileage;      // Divide by 10
    uint8_t external_temp; // 0xFF if not present, otherwise divide by 2 and subtract 40.

} __attribute__((packed));

struct psa_dash_packet
{
    struct psa_header header;
    struct psa_dash_data data;
    uint8_t crc;

} __attribute__((packed));

/**
 * @brief Data from the trip computer.
 *
 *
 */
struct psa_trip_data
{
    uint16_t trip_1_distance;
    uint16_t trip_2_distance;
    uint16_t trip_1_fuel_consumption;
    uint16_t trip_2_fuel_consumption;
    uint16_t current_fuel_consumption;
    uint16_t distance_to_empty;

    uint8_t trip_1_speed;
    uint8_t trip_2_speed;
    uint8_t trip_button_pressed;

} __attribute__((packed));

struct psa_trip_packet
{
    struct psa_header header;
    struct psa_trip_data data;
    uint8_t crc;

} __attribute__((packed));

/**
 * @brief Random assortment of data that doesnt fit anywhere else.
 * Also send the car state, and cd changer command.
 *
 */
struct psa_status_data
{
    uint8_t doors_locked;
    uint8_t deadlocking_active;
    uint8_t car_state;
    uint8_t cd_changer_command;
    uint8_t reserved[14];
} __attribute__((packed));

struct psa_status_packet
{
    struct psa_header header;
    struct psa_status_data data;
    uint8_t crc;
} __attribute__((packed));

/**
 * @brief The current time as reported by the ESP
 *
 */
struct psa_time_data
{
    uint8_t valid_time; // 1 if time is set
    uint8_t day;        // 1 - 31
    uint8_t month;      // 0 - 11
    uint8_t year;       // years since 1900
    uint8_t hour;       // 0 - 24
    uint8_t minutes;    // 0 - 59
    uint8_t seconds;    // 0 - 61
} __attribute__((packed));

struct psa_time_packet
{
    struct psa_header header;
    struct psa_time_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_esp32_temp_data
{
    uint16_t temperature;
    uint16_t cpu_freq;
} __attribute__((packed));

struct psa_esp32_temp_packet
{
    struct psa_header header;
    struct psa_esp32_temp_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_vin_packet
{
    struct psa_header header;
    uint8_t vin[17]; // VIN in ASCII. NOT null-terminated
    uint8_t crc;

} __attribute__((packed));

// SETTER STRUCTS

struct psa_cd_changer_data
{
    uint8_t status;             // enum psa_cd_changer_status
    uint8_t track_time_seconds; // Track seconds in decimal
    uint8_t track_time_minutes; // Track minutes in decimal
    uint8_t track_number;       // Track number in decimal
    uint8_t cd_number;          // CD number in decimal
    uint8_t total_tracks;       // Total tracks in decimal
    uint8_t cds_present;        // Bitwise presentation of present cds
} __attribute__((packed));

struct psa_cd_changer_packet
{
    struct psa_header header;
    struct psa_cd_changer_data data;
    uint8_t crc;
} __attribute__((packed));

struct psa_trip_reset_data
{
    uint8_t trip_meter; // enum psa_trip_meter, trip meter to reset
} __attribute__((packed));

struct psa_trip_reset_packet
{
    struct psa_header header;
    struct psa_trip_reset_data data;
    uint8_t crc;
} __attribute__((packed));

#endif