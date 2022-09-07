#ifndef VAN_PACKET_PARSER
#define VAN_PACKET_PARSER

#include "psa_common.h"
#include "van_structs/van_dash_struct.h"
#include "van_structs/van_radio_settings_struct.h"
#include "van_structs/van_rpm_struct.h"
#include "van_structs/van_trip_computer_struct.h"
#include "van_structs/van_radio_struct.h"
#include "van_structs/van_car_status_structs.h"
#include "van_structs/van_cd_changer_commands_struct.h"

/**
 * @brief Parse a VAN packet by the given IDEN.
 * Returns `PSA_OK` if packet was succesfully parsed.
 * Stores the results in the `data_buffers` buffers.
 *
 * @param iden Iden of the packet
 * @param size Size of the data portion
 * @param data Data of the packet
 * @param data_buffers The msp buffers to store the parsed data into
 * @return int
 */
extern int psa_parse_van_packet_esp32(
    uint16_t iden,
    uint8_t size,
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers);

/**
 * @brief Legacy function used with the ESP8266 library. Does the same as `psa_parse_van_packet_esp32`,
 * but works with the VanBus ESP8266 library packet class.
 * Returns `PSA_OK` if packet was succesfully parsed.
 *
 * @param pkt TVanPacketRxDesc packet
 * @param data_buffers The msp buffers to store the parsed data into
 * @return int
 */
extern int psa_parse_van_packet_esp8266(void *pkt, void *data_buffers);

extern void psa_handle_button_logic_task(void *params);

#endif