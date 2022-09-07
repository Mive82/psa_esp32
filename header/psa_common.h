#ifndef PSA_COMMON
#define PSA_COMMON

// #include "psa_packet_defs.h"

enum psa_status
{
    PSA_OK = 0,
    PSA_NULL_PTR,
    PSA_UNKNOWN_IDEN,
    PSA_INVALID_VAN_PACKET,
    PSA_INVALID_VAN_PACKET_SIZE,
    PSA_INVALID_MSP_PACKET_SIZE,
    PSA_NO_PACKET,
    PSA_ERROR,
};

/**
 * @brief Struct containing pointers to all data buffers for packets.
 *
 */
struct psa_output_data_buffers
{
    void *engine_data;
    void *radio_data;
    void *presets_data;
    void *cd_player_data;
    void *headunit_data;
    void *door_data;
    void *vin_data;
    void *dash_data;
    void *trip_data;
    void *status_data;
};

#endif