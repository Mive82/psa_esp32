#ifndef PSA_VAN_RPM_STRUCT_H
#define PSA_VAN_RPM_STRUCT_H

/**
 * @brief VAN rpm and speed data struct. Iden 0x824
 *
 */
struct psa_van_rpm_speed_struct
{

    uint16_t rpm;           // RPM in big endian. Divide by 8 to get actual rpm;
    uint16_t speed;         // Speed in big endian. Divide by 100 to get actual speed;
    uint16_t distance;      // Packet sequence in big endian. Increments with each passed decimeter
    uint8_t packet_changed; // Increments each time information significantly changes
} __attribute__((packed));

#endif