#ifndef PSA_VAN_CAR_STATUS_STRUCT_H
#define PSA_VAN_CAR_STATUS_STRUCT_H

// Iden 0x524

struct psa_van_car_status_2_short
{
    uint8_t bytes[8];
    uint8_t byte_08;
    uint8_t bytes2[5];
} __attribute__((packed));

struct psa_van_car_status_2_long
{
    uint8_t bytes[8];
    uint8_t byte_08;
    uint8_t bytes2[7];
} __attribute__((packed));

#endif // PSA_VAN_CAR_STATUS_STRUCT_H