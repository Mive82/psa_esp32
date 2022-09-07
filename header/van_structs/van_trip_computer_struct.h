#ifndef PSA_VAN_TRIP_COMPUTER_STRUCT_H
#define PSA_VAN_TRIP_COMPUTER_STRUCT_H

// IDEN 0x564 or PSA_VAN_IDEN_CAR_STATUS1

struct psa_van_trip_computer_door_struct
{
    uint8_t fuel_flap : 1;   // bit 0
    uint8_t sunroof : 1;     // bit 1
    uint8_t hood : 1;        // bit 2
    uint8_t boot_lid : 1;    // bit 3
    uint8_t rear_left : 1;   // bit 4
    uint8_t rear_right : 1;  // bit 5
    uint8_t front_left : 1;  // bit 6
    uint8_t front_right : 1; // bit 7
} __attribute__((packed));

union psa_van_trip_computer_door_union
{
    struct psa_van_trip_computer_door_struct unpacked;
    uint8_t packed;
};

struct psa_van_trip_computer_button_struct_maybe
{
    uint8_t trip_button : 1;
    uint8_t : 7;
} __attribute__((packed));

/**
 * @brief Trip computer info. Also includes door info. Iden 0x564
 *
 */
struct psa_van_trip_computer
{

    uint8_t header;
    uint8_t unknown1[6];

    union psa_van_trip_computer_door_union door_status;

    uint8_t unknown2[2];

    struct psa_van_trip_computer_button_struct_maybe trip_button;

    uint8_t trip_1_speed;
    uint8_t trip_2_speed;
    uint8_t unknown3;

    uint16_t trip_1_distance;          // I'm guessing in KM
    uint16_t trip_1_fuel_consumption;  // Says divide by 10 (probably in big endian)
    uint16_t trip_2_distance;          // I'm guessing in KM
    uint16_t trip_2_fuel_consumption;  // Says divide by 10 (probably in big endian)
    uint16_t current_fuel_consumption; // Says divide by 10 (probably in big endian)
    uint16_t distance_to_empty;        // I'm guessing in KM
    uint8_t footer;

} __attribute__((packed));

#endif