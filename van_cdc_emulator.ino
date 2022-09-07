#include "header/van_cdc_emulator_structs.h"

enum psa_status init_cdc_packet(
    struct van_cd_changer_packet *packet)
{
    if (packet == NULL)
    {
        return PSA_NULL_PTR;
    }

    packet->header = 0x80;
    packet->footer = 0x80;
    packet->cd_flag = 0b000001;
    packet->cd_num = 0x01;
    packet->cd_present = VAN_CDC_CD_PRESENT;
    packet->minutes = 1;
    packet->seconds = 0;
    packet->shuffle = 0;
    packet->status = VAN_CDC_ON_PLAYING;
    packet->track_num = 1;
    packet->total_tracks = 1;
    packet->unknown = 0x00;

    return PSA_OK;
}