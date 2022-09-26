//#define PSA_SIMULATE

//#define PSA_SPIFFS_LOG

#define RASPBERRY_POWER_CONTROL // Define to control the Raspberry power on status

#define ESP_USE_WIFI // Define to enable the use of WiFi on the ESP

#define ESP_POWER_CONTROL // Define to enable deep sleep

#include "config.h"

#ifndef PSA_SIMULATE

#ifdef USE_SOFTWARE_VAN

#include <VanBus.h>
#include <VanBusRx.h>
#include <VanBusTx.h>

#else

// Everything TSS463C Related -> Used for sending CD changer packets, and maybe receving VAN packets
#include <itss46x.h>
#include <tss46x_van.h>
#include <tss463.h>

// Everything related to receiving VAN packets
#include <esp32_arduino_rmt_van_rx.h>
#include <esp32_rmt_van_rx.h>

#define VAN_824_CHANNEL 0
#define VAN_564_CHANNEL 1
#define VAN_8A4_CHANNEL 2
#define VAN_4D4_CHANNEL 3
#define VAN_554_CHANNEL 4
#define VAN_E24_CHANNEL 5

#endif // USE SOFTWARE_VAN

#include "header/van_packet_parser.h" // Parsing received packets

#endif // PSA_SIMULATE

#ifdef PSA_SPIFFS_LOG

#include <FS.h>
#include <SPIFFS.h>

#endif

#include "header/psa_packet_defs.h"          // MSP packet definitons, for RPi
#include "header/psa_common.h"               // psa_status and msp buffers passed to the VAN parser
#include "header/msp_common.h"               // MSP defines
#include "header/van_helper.h"               // get_iden_from_bytes
#include "header/esp_helper.h"               // read_esp32_temperature(), ntp_config(), set_time_zone()
#include "header/van_cdc_emulator_structs.h" // CD changer MSP packet definiton, and initializer function

#include "header/version.h"

#include <WiFi.h>

RTC_DATA_ATTR uint8_t rtc_time_valid = 0; // Is the time set in the RTC

RTC_DATA_ATTR uint8_t car_state = PSA_STATE_CAR_SLEEP; // The car state, used for RPi power management

uint8_t raspberry_power_status = 0; // Target RPi power status, 0 - Off, 1 - On

// HardwareSerial serial_port = Serial; // Serial port used, Serial - PC, Serial2 - RPi
// I haven't been able to run both together reliably

unsigned long last_cdc_message = 0;    // last cd changer packet timestamp, used to send it every second
unsigned long current_cdc_message = 0; // Current cd changer packet timestamp, used for overflow safety... I hope

esp_timer_handle_t timer_handle;

/*
    The following unions are for MSP packets.
    They are unions so I can easily manipulate the buffers underneath
*/

union engine_packet_union
{
    struct psa_engine_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union radio_packet_union
{
    struct psa_radio_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union radio_presets_union
{
    struct psa_preset_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union headunit_packet_union
{
    struct psa_headunit_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union cd_player_packet_union
{
    struct psa_cd_player_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union vin_packet_union
{
    struct psa_vin_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union dash_packet_union
{
    struct psa_dash_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union door_packet_union
{
    struct psa_door_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union trip_packet_union
{
    struct psa_trip_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union status_packet_union
{
    struct psa_status_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union time_packet_union
{
    struct psa_time_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union esp32_temp_packet_union
{
    struct psa_esp32_temp_packet packet;
    uint8_t buffer[sizeof(packet)];
};

union version_packet_union
{
    struct psa_version_packet packet;
    uint8_t buffer[sizeof(packet)];
};

uint8_t msp_input_buffer[MSP_MAX_SIZE]; // Buffer for incoming MSP messages

uint8_t msp_response_buffer[MSP_MAX_SIZE]; // Buffer for MSP responses for SET request messages

#ifdef PSA_SPIFFS_LOG

uint16_t log_file_no = 0;
File log_file;
uint8_t do_log = 0;

char log_buffer[255];

uint8_t log_error = 0;

#endif

#ifdef PSA_SIMULATE
const char vin[] = "VF32AKFWF4XXXXXXX";

const char *radio_station_names[] = {
    "HRT-HR 1",
    "HRT-HR 2",
    "Laganini",
    "Otvoreni",
    "Narodni",
    "Sl.Radio"};

const uint16_t radio_station_freqs[] = {
    9180,
    10170,
    9910,
    9120,
    9530,
    10620};

#endif

// const int TX_PIN = D1;
// const int RX_PIN = D2;

#ifdef USE_SOFTWARE_VAN
const int TX_PIN = 22;
const int RX_PIN = 23;
#endif // USE_SOFTWARE_VAN

#ifndef PSA_SIMULATE

SemaphoreHandle_t mutex = xSemaphoreCreateMutex(); // A mutex just in case, probably not even doing anything

#endif

RTC_DATA_ATTR uint8_t presets_data_valid = 0;
RTC_DATA_ATTR union radio_presets_union store_presets_packet[4];

union engine_packet_union engine_packet;
union radio_packet_union radio_packet;
union radio_presets_union presets_packet[4];
union headunit_packet_union headunit_packet;
union cd_player_packet_union cd_packet;
union vin_packet_union vin_packet;
union dash_packet_union dash_packet;
union door_packet_union door_packet;
union trip_packet_union trip_packet;
union status_packet_union status_packet;
union time_packet_union time_packet;
union esp32_temp_packet_union esp32_packet;
union version_packet_union version_packet;

union van_cd_changer_union cd_changer_emu_packet; // In van_cdc_emulator_structs.h

#ifdef PSA_SIMULATE
volatile uint16_t rpm = 700 * 8;
volatile uint16_t speed = 0;
volatile uint8_t fuel = 35;
volatile uint8_t temp = 90;

volatile uint8_t dash_data_ready = 0;

volatile uint32_t engine_time_passed = 0;

volatile uint32_t radio_time_passed = 0;

volatile uint32_t door_time_passed = 0;

volatile uint16_t engine_sequence = 0;

volatile uint8_t curr_band = 1;

volatile uint8_t curr_preset = 1;

volatile uint16_t curr_fuel = 0;

volatile uint16_t curr_range = 1000;

volatile unsigned long last_trip = 0;
#endif

// struct psa_incoming_msp_packet msp_message;
// struct psa_vin_packet vin_packet;

struct psa_output_data_buffers data_buffers; // Pointers to MSP buffers that will be filled out by the VAN parser

#ifndef PSA_SIMULATE

#ifdef USE_SOFTWARE_VAN

TVanPacketRxDesc van_rx_pkt;

#else

SPIClass *spi;            // Spi instance, used for comms with the TSS463C
ITss46x *vanSender;       // Creating a TSS instance
TSS46X_VAN *VANInterface; // Ditto

uint8_t van_message[34];     // Buffer for incoming VAN message, the entire VAN packet has a max size of 32
uint8_t van_message_len = 0; // Length of the message received

ESP32_RMT_VAN_RX VAN_RX; // ESP32 VAN receiver instance

// ESP32 Receiver pin setup
const uint8_t VAN_DATA_RX_RMT_CHANNEL = 0;
const uint8_t VAN_DATA_RX_PIN = 21;
const uint8_t VAN_DATA_RX_LED_INDICATOR_PIN = 2;

// Handles for tasks, they are all on core 0
TaskHandle_t van_task = NULL;
TaskHandle_t msp_task = NULL;
TaskHandle_t ntp_task = NULL;
TaskHandle_t car_state_task = NULL;

/**
 * @brief Initialize SPI communication with the TSS463 chip.
 *
 */
void InitTss463()
{
    // initialize SPI

    uint8_t VAN_PIN = 5;      // Set pin 5 as the CS pin
    spi = new SPIClass(VSPI); // Start spi communication on the VSPI SPI bus
    spi->begin();
    // instantiate the VAN message sender for a TSS463
    vanSender = new Tss463(VAN_PIN, spi);
}

/**
 * @brief A Task that periodically checks for new VAN packets
 *
 * @param params NULL
 */
void van_receive_task(void *params)
{

    // serial_port.print("Van_receive_task on core ");
    // serial_port.println(xPortGetCoreID());

    uint8_t *van_message_ptr = (uint8_t *)van_message; // Unneccesary cast, but to be safe

    // All tasks must have a loop like this
    while (1)
    {

        xSemaphoreTake(mutex, portMAX_DELAY);              // Mutex stuff, hopefully does something
        VAN_RX.Receive(&van_message_len, van_message_ptr); // Receives a new VAN frame
                                                           // The first byte contains the SOF byte, 0x0E
                                                           // The second and third bytes are the IDEN (12 bits) and COMMAND (4 bits)
                                                           // The rest is data
                                                           // The last two bytes are the CRC
        if (van_message_len > 0)
        {
            if (VAN_RX.IsCrcOk(van_message_ptr, van_message_len))
            {
                psa_parse_van_packet_esp32(get_iden_from_bytes(van_message[1], van_message[2]), van_message_len - 5, van_message_ptr + 3, &data_buffers);
                // uint8_t data_len = van_message_len - 5;
                // uint8_t *data = van_message_ptr + 3;
                // Currently looking for packet 8C4 with data type 8A
                // if ((get_iden_from_bytes(van_message[1], van_message[2]) == 0x8C4))
                // {
                //     // Only display the packet if it's different
                //     // if (memcmp(vanMessage, vanMessageOld, vanMessageLength) != 0)
                //     {
                //         // memcpy(vanMessageOld, vanMessage, vanMessageLength);
                //         for (size_t i = 0; i < data_len; i++)
                //         {
                //             if (i != data_len - 1)
                //             {
                //                 serial_port.printf("%02X ", data[i]);
                //             }
                //             else
                //             {
                //                 serial_port.printf("%02X", data[i]);
                //             }
                //         }
                //         serial_port.println();
                //     }
                // }
            }
        }
        xSemaphoreGive(mutex);
        vTaskDelay(5 / portTICK_PERIOD_MS); // A delay, so it doesn't take up all processing time
        // delay(7);
    }
}

#endif // USE_SOFTWARE_VAN

void update_rasbperry_power_state(int power)
{
#ifdef RASPBERRY_POWER_CONTROL
    if (power)
    {
        pinMode(rpi_power_pin, INPUT);
    }
    else
    {
        pinMode(rpi_power_pin, OUTPUT);
        digitalWrite(rpi_power_pin, LOW);
    }
#endif // RASPBERRY_POWER_CONTROL
}

void update_car_state(uint8_t new_car_state)
{
    if (new_car_state == car_state)
    {
        return;
    }
    else if (new_car_state > PSA_STATE_CAR_OFF)
    {
        // if (esp_timer_is_active(timer_handle))
        // {
        //     esp_timer_stop(timer_handle);
        // }
        setCpuFrequencyMhz(160);
    }
    else
    {
        // if (raspberry_power_status == 1 && !esp_timer_is_active(timer_handle))
        // {
        //     esp_timer_start_once(timer_handle, (60 * 1000 * 1000));
        // }
        // else if (raspberry_power_status == 0 && esp_timer_is_active(timer_handle))
        // {
        //     esp_timer_stop(timer_handle);
        // }
        setCpuFrequencyMhz(80);
    }

    car_state = new_car_state;
}

void rpi_off_timer_timeout(void *args)
{
    raspberry_power_status = 0;
    update_rasbperry_power_state(raspberry_power_status);
}

/**
 * @brief A task that calculates the car state as defined in the table.
 *
 * @param params
 */
void psa_calculate_state(void *params)
{
    while (1)
    {
        // PSA_STATE_ENGINE_ON
        if (dash_packet.packet.data.engine_running)
        {
            update_car_state(PSA_STATE_ENGINE_ON);
            raspberry_power_status = 1;
        }
        // PSA_STATE_ECONOMY_MODE
        // else if (dash_packet.packet.data.economy_mode)
        // {
        //     update_car_state(PSA_STATE_ECONOMY_MODE);
        //     raspberry_power_status = 0;
        // }
        // PSA_STATE_CRANKING
        else if (dash_packet.packet.data.accesories_on == 0 &&
                 dash_packet.packet.data.ignition_on == 1)
        {
            update_car_state(PSA_STATE_CRANKING);
            raspberry_power_status = 1;
        }
        // PSA_STATE_IGNITION
        else if (dash_packet.packet.data.ignition_on == 1 &&
                 dash_packet.packet.data.accesories_on == 1)
        {
            update_car_state(PSA_STATE_IGNITION);
            raspberry_power_status = 1;
        }
        // PSA_STATE_ACCESSORY
        else if (dash_packet.packet.data.accesories_on == 1 &&
                 dash_packet.packet.data.ignition_on == 0)
        {
            update_car_state(PSA_STATE_ACCESSORY);
            raspberry_power_status = 1;
        }
        // PSA_STATE_RADIO_ON
        // PSA_STATE_CAR_LOCKED
        // PSA_STATE_CAR_OFF
        else if (dash_packet.packet.data.accesories_on == 0 &&
                 dash_packet.packet.data.ignition_on == 0)
        {
            if (headunit_packet.packet.data.unit_powered_on)
            {
                update_car_state(PSA_STATE_RADIO_ON);
                raspberry_power_status = 1;
            }
            else if (status_packet.packet.data.doors_locked)
            {
                update_car_state(PSA_STATE_CAR_LOCKED);
                raspberry_power_status = 0;
            }
            else
            {
                update_car_state(PSA_STATE_CAR_OFF);
            }
        }

        update_rasbperry_power_state(raspberry_power_status);

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

#endif // PSA_SIMULATE

/**
 * @brief Calculate crc of an msp packet. Returns the crc.
 * Calculated by XOR-ing every byte except the first two (start magic and direction) and the last one (the crc itself)
 *
 * @param packet The packet buffer
 * @param size Packet size
 * @return uint8_t crc
 */
uint8_t calculate_crc(uint8_t const *const packet, const uint8_t size)
{
    if (packet == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (size <= sizeof(struct psa_header))
    {
        return PSA_INVALID_MSP_PACKET_SIZE;
    }

    uint8_t crc = 0;

    for (int i = 2; i < size - 1; ++i)
    {
        crc ^= packet[i];
    }

    return crc;
}

/*
    MSP Packet initialization functions.
    They populate the header with static information for each packet.
*/

void prepare_vin_packet()
{
    memset(vin_packet.buffer, 0, sizeof(vin_packet));

    vin_packet.packet.header.start = MSP_START_MAGIC;
    vin_packet.packet.header.ident = PSA_IDENT_VIN;
    vin_packet.packet.header.direction = '>';
    vin_packet.packet.header.size = sizeof(vin_packet.packet.vin);

#ifdef PSA_SIMULATE
    memcpy(vin_packet.packet.vin, vin, vin_packet.packet.header.size);
#endif
}

void init_version_packet()
{
    memset(version_packet.buffer, 0, sizeof(version_packet));

    version_packet.packet.header.start = MSP_START_MAGIC;
    version_packet.packet.header.ident = PSA_IDENT_VERSIONS;
    version_packet.packet.header.size = sizeof(version_packet.packet.data);
    version_packet.packet.header.direction = '>';

    version_packet.packet.data.api_version = PSA_ESP_API_VERSION;
    version_packet.packet.data.firmware_version_major = PSA_ESP_FIRMWARE_VERSION_MAJOR;
    version_packet.packet.data.firmware_version_minor = PSA_ESP_FIRMWARE_VERSION_MINOR;

    version_packet.packet.crc = calculate_crc(version_packet.buffer, sizeof(version_packet));
}

void init_engine_packet()
{
    // engine_data_size = sizeof(engine_packet);
    engine_packet.packet.header.start = MSP_START_MAGIC;
    engine_packet.packet.header.direction = '>';
    engine_packet.packet.header.ident = (uint16_t)PSA_IDENT_ENGINE;
    engine_packet.packet.header.size = sizeof(struct psa_engine_data);

    engine_packet.packet.crc = 0;
}
void init_radio_packet()
{
    radio_packet.packet.header.start = MSP_START_MAGIC;
    radio_packet.packet.header.direction = '>';
    radio_packet.packet.header.ident = (uint16_t)PSA_IDENT_RADIO;
    radio_packet.packet.header.size = sizeof(struct psa_radio_data);

    radio_packet.packet.crc = 0;
}
void init_presets_packet()
{
    for (int i = 0; i < PSA_PRESET_MAX; ++i)
    {

        presets_packet[i].packet.header.start = MSP_START_MAGIC;
        presets_packet[i].packet.header.direction = '>';
        presets_packet[i].packet.header.ident = PSA_IDENT_RADIO_PRESETS;
        presets_packet[i].packet.header.size = sizeof(struct psa_preset_data);

        presets_packet[i].packet.crc = 0;
    }
}

void init_headunit_packet()
{
    headunit_packet.packet.header.start = MSP_START_MAGIC;
    headunit_packet.packet.header.direction = '>';
    headunit_packet.packet.header.ident = (uint16_t)PSA_IDENT_HEADUNIT;
    headunit_packet.packet.header.size = sizeof(struct psa_headunit_data);

    headunit_packet.packet.crc = 0;
}
void init_cd_packet()
{
    cd_packet.packet.header.start = MSP_START_MAGIC;
    cd_packet.packet.header.direction = '>';
    cd_packet.packet.header.ident = (uint16_t)PSA_IDENT_CD_PLAYER;
    cd_packet.packet.header.size = sizeof(struct psa_cd_player_data);

    cd_packet.packet.crc = 0;
}

void init_dash_packet()
{
    dash_packet.packet.header.start = MSP_START_MAGIC;
    dash_packet.packet.header.direction = '>';
    dash_packet.packet.header.ident = (uint16_t)PSA_IDENT_DASHBOARD;
    dash_packet.packet.header.size = sizeof(struct psa_dash_data);

    dash_packet.packet.crc = 0;

#ifdef PSA_SIMULATE

    dash_packet.packet.data.engine_running = 1;

#endif // PSA_SIMULATE
}
void init_door_packet()
{
    door_packet.packet.header.start = MSP_START_MAGIC;
    door_packet.packet.header.direction = '>';
    door_packet.packet.header.ident = (uint16_t)PSA_IDENT_DOORS;
    door_packet.packet.header.size = sizeof(struct psa_door_data);

    door_packet.packet.crc = 0;
}
void init_trip_packet()
{
    trip_packet.packet.header.start = MSP_START_MAGIC;
    trip_packet.packet.header.direction = '>';
    trip_packet.packet.header.ident = (uint16_t)PSA_IDENT_TRIP;
    trip_packet.packet.header.size = sizeof(struct psa_trip_data);

    trip_packet.packet.data.current_fuel_consumption = 0xffff;

    trip_packet.packet.crc = 0;
}

void init_status_packet()
{
    status_packet.packet.header.start = MSP_START_MAGIC;
    status_packet.packet.header.direction = '>';
    status_packet.packet.header.ident = (uint16_t)PSA_IDENT_CAR_STATUS;
    status_packet.packet.header.size = sizeof(struct psa_status_data);

    status_packet.packet.crc = 0;
}

void init_time_packet()
{
    time_packet.packet.header.start = MSP_START_MAGIC;
    time_packet.packet.header.direction = '>';
    time_packet.packet.header.ident = (uint16_t)PSA_IDENT_TIME;
    time_packet.packet.header.size = sizeof(struct psa_time_data);

    time_packet.packet.crc = 0;
}

void init_esp32_temp_packet()
{
    memset(esp32_packet.buffer, 0, sizeof(esp32_packet));

    esp32_packet.packet.header.start = MSP_START_MAGIC;
    esp32_packet.packet.header.direction = '>';
    esp32_packet.packet.header.ident = (uint16_t)PSA_IDENT_ESP32_TEMP;
    esp32_packet.packet.header.size = sizeof(struct psa_esp32_temp_data);

    esp32_packet.packet.crc = 0;
}

/**
 * @brief Initializes all MSP packets.
 * Fills out header information.
 *
 */
void init_packets()
{
    init_version_packet();
    prepare_vin_packet();
    init_engine_packet();
    init_radio_packet();
    init_presets_packet();
    init_dash_packet();
    init_door_packet();
    init_trip_packet();
    init_headunit_packet();
    init_cd_packet();
    init_status_packet();
    init_time_packet();
    init_esp32_temp_packet();

    init_cdc_packet((struct van_cd_changer_packet *)&cd_changer_emu_packet.packet); // in van_cdc_emulator_structs.h
}

#ifdef PSA_SPIFFS_LOG

void psa_start_log()
{
    if (do_log == 0)
    {
        log_file = SPIFFS.open("/logno.txt", "r", true);

        char file_no[10];
        memset(file_no, 0, sizeof(file_no));

        log_file_no = log_file.parseInt();

        serial_port.printf("Read file number %d\n", log_file_no);
        log_file_no++;

        log_file.close();
        log_file = SPIFFS.open("/logno.txt", "w", true);

        log_file.println(log_file_no);
        log_file.close();
        char path[50];
        sprintf(path, "/log_%d.csv", log_file_no);
        log_file = SPIFFS.open(path, "w", true);
        serial_port.printf("Opened file %s\n", path);
        sprintf(log_buffer, "RPM,SPEED,TEMP,FUEL,FUEL_CONS");
        log_file.println(log_buffer);
        do_log = 1;
    }
}

void psa_log_data()
{
    if (do_log)
    {

        uint16_t fuel_cons = (trip_packet.packet.data.current_fuel_consumption == 0xffff ? 0 : trip_packet.packet.data.current_fuel_consumption);

        if (!log_error)
        {
            sprintf(log_buffer, "%d,%.2f,%d,%d,%.1f",
                    engine_packet.packet.data.rpm / 8,
                    (float)engine_packet.packet.data.speed / 100,
                    engine_packet.packet.data.engine_temperature - 39,
                    engine_packet.packet.data.fuel_level,
                    (float)fuel_cons / 10);

            log_file.println(log_buffer);
        }
    }
}

void psa_end_log()
{
    if (do_log)
    {
        if (!log_error)
        {
            log_file.flush();
            log_file.close();
            do_log = 0;
        }
    }
}

#endif

/**
 * @brief WiFi Event callback.
 * Executed when the ESP gets an IP address.
 * Sets the time using ntp, and shuts down the wifi.
 *
 * @param event
 * @param info
 */
void on_wifi_ip_callback(WiFiEvent_t event, WiFiEventInfo_t info)
{
// Serial.println("IP address: ");
// Serial.println(WiFi.localIP());
#ifdef ESP_USE_WIFI
    ntp_config(ntp_server1, ntp_server2, ntp_server3);
    struct tm tm_test;
    if (getLocalTime(&tm_test))
    {
        rtc_time_valid = 1;
    }

    delay(4000);

    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
#endif // ESP_USE_WIFI
}

void on_wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
#ifdef ESP_USE_WIFI

    if (rtc_time_valid == 1)
    {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
    }
    else
    {
        WiFi.reconnect();
    }

#endif // ESP_USE_WIFI
}

/**
 * @brief A seperate task for receiving MSP messages.
 * Currently unused.
 *
 * @param params
 */
// void msp_receive_task(void *params)
// {
//     while (1)
//     {
//         receive_msp_message();
//     }
// }

/**
 * @brief Periodic retry if clock is not set.
 *
 * @param params
 */
void wifi_retry_task(void *params)
{
    while (1)
    {
        vTaskDelay(30000 / portTICK_PERIOD_MS);
#ifdef ESP_USE_WIFI
        if (rtc_time_valid == 0 && WiFi.status() != WL_CONNECTED)
        {
            WiFi.reconnect();
        }
#endif // ESP_USE_WIFI
    }
}

// void serialFlush(int bytes, HardwareSerial serial_port)
// {
//     while (bytes > 0)
//     {
//         char t = serial_port.read();
//         bytes--;
//     }
// }

/**
 * @brief Send a response to MSP SET request
 *
 * @param ident Ident to respond to
 * @param response Byte to send as response. From enum psa_msp_response
 */
void send_set_response(uint16_t ident, uint8_t response, HardwareSerial *serial)
{
    int response_size = sizeof(struct psa_header) + 2;

    struct psa_header *header = (struct psa_header *)msp_response_buffer;
    header->start = MSP_START_MAGIC;
    header->size = 1;
    header->direction = '>';
    header->ident = ident;

    msp_response_buffer[sizeof(*header)] = response;

    msp_response_buffer[sizeof(*header) + 1] = calculate_crc((uint8_t *)msp_response_buffer, response_size);

    serial->write(msp_response_buffer, response_size);
}

void parse_msp_data(uint16_t ident, uint8_t *packet, uint8_t packet_size, HardwareSerial *serial)
{

    struct psa_header *header = (struct psa_header *)packet;

    uint8_t *data = (uint8_t *)(packet + sizeof(struct psa_header));

    uint8_t data_size = header->size;

    uint8_t packet_crc = data[data_size];

    uint8_t calculated_crc = calculate_crc(packet, packet_size);

    if (packet_crc != calculated_crc)
    {
        send_set_response(ident, PSA_MSP_CRC_ERROR, serial);
    }

    switch (ident)
    {
    case PSA_IDENT_SET_CD_CHANGER_DATA:
        if (data_size != sizeof(struct psa_cd_changer_data))
        {
            send_set_response(ident, PSA_MSP_INVALID_DATA_SIZE, serial);
        }
        else
        {
            struct psa_cd_changer_data *cdc_data = (struct psa_cd_changer_data *)data;

            if (cdc_data->cds_present > 0b111111 || cdc_data->cd_number > 6 || cdc_data->cd_number < 0 || cdc_data->cds_present < 0)
            {
                send_set_response(ident, PSA_MSP_INVALID_DATA, serial);
            }
            else
            {

                // cd_changer_emu_packet.packet.cd_flag = cdc_data->cds_present;

                if (cdc_data->track_time_minutes == 0xff ||
                    cdc_data->track_time_seconds == 0xff ||
                    cdc_data->total_tracks == 0xff ||
                    cdc_data->track_number == 0xff)
                {
                    cd_changer_emu_packet.packet.minutes = 0xff;
                    cd_changer_emu_packet.packet.seconds = 0xff;
                    cd_changer_emu_packet.packet.total_tracks = 0xff;
                    cd_changer_emu_packet.packet.track_num = 0xff;
                }
                else
                {

                    cd_changer_emu_packet.packet.minutes = dec_to_bcd(cdc_data->track_time_minutes);
                    cd_changer_emu_packet.packet.seconds = dec_to_bcd(cdc_data->track_time_seconds);
                    cd_changer_emu_packet.packet.total_tracks = dec_to_bcd(cdc_data->total_tracks);
                    cd_changer_emu_packet.packet.track_num = dec_to_bcd(cdc_data->track_number);
                }
                cd_changer_emu_packet.packet.cd_num = cdc_data->cd_number;
                cd_changer_emu_packet.packet.status = cdc_data->status;
            }

            send_set_response(ident, PSA_MSP_OK, serial);
            AnswerToCDC();
        }
        break;

    case PSA_IDENT_SET_TRIP_RESET:
        if (data_size != sizeof(struct psa_trip_reset_data))
        {
            send_set_response(ident, PSA_MSP_INVALID_DATA_SIZE, serial);
        }
        else
        {
            struct psa_trip_reset_data *trip_data = (struct psa_trip_reset_data *)data;

            if (send_VAN_trip_reset((enum psa_trip_meter)trip_data->trip_meter))
            {
                send_set_response(ident, PSA_MSP_INVALID_DATA, serial);
            }

            send_set_response(ident, PSA_MSP_OK, serial);
        }
        break;

    default:
        send_set_response(ident, PSA_MSP_UNKNOWN_IDENT, serial);
        break;
    }
}

/**
 * @brief Send an MSP packet back.
 *
 * @param ident Ident to send back
 */
void send_packet(uint16_t ident, HardwareSerial *serial)
{
    // Most packets are updated in van_packet_parser.ino
    // Here, they are just sent.

    // Serial1.printf("Getting ready to send ident %d\r\n", ident);
    switch (ident)
    {

    case PSA_IDENT_VERSIONS:
        serial->write(version_packet.buffer, sizeof(version_packet));
        break;

    case PSA_IDENT_ENGINE:

        engine_packet.packet.crc = calculate_crc(engine_packet.buffer, sizeof(engine_packet));
        // Serial1.printf("Sent ident %d\r\n", ident);
        serial->write(engine_packet.buffer, sizeof(engine_packet));

        break;

    case PSA_IDENT_RADIO:

        radio_packet.packet.crc = calculate_crc(radio_packet.buffer, sizeof(radio_packet));
        // Serial1.printf("Sent ident %d\r\n", ident);
        serial->write(radio_packet.buffer, sizeof(radio_packet));

        break;

    case PSA_IDENT_RADIO_PRESETS:
        switch (radio_packet.packet.data.band)
        {
        case PSA_AM_1:
            presets_packet[PSA_PRESET_AM].packet.crc = calculate_crc(presets_packet[PSA_PRESET_AM].buffer, sizeof(*presets_packet));
            serial->write(presets_packet[PSA_PRESET_AM].buffer, sizeof(*presets_packet));
            break;
        case PSA_FM_1:
            presets_packet[PSA_PRESET_FM_1].packet.crc = calculate_crc(presets_packet[PSA_PRESET_FM_1].buffer, sizeof(*presets_packet));
            serial->write(presets_packet[PSA_PRESET_FM_1].buffer, sizeof(*presets_packet));
            break;
        case PSA_FM_2:
            presets_packet[PSA_PRESET_FM_2].packet.crc = calculate_crc(presets_packet[PSA_PRESET_FM_2].buffer, sizeof(*presets_packet));
            serial->write(presets_packet[PSA_PRESET_FM_2].buffer, sizeof(*presets_packet));
            break;
        case PSA_FM_AST:
            presets_packet[PSA_PRESET_FMAST].packet.crc = calculate_crc(presets_packet[PSA_PRESET_FMAST].buffer, sizeof(*presets_packet));
            serial->write(presets_packet[PSA_PRESET_FMAST].buffer, sizeof(*presets_packet));
            break;
        default:
            break;
        }
        break;

    case PSA_IDENT_HEADUNIT:

        headunit_packet.packet.crc = calculate_crc(headunit_packet.buffer, sizeof(headunit_packet));
        serial->write(headunit_packet.buffer, sizeof(headunit_packet));
        break;

    case PSA_IDENT_CD_PLAYER:
        cd_packet.packet.crc = calculate_crc(cd_packet.buffer, sizeof(cd_packet));
        serial->write(cd_packet.buffer, sizeof(cd_packet));
        break;

    case PSA_IDENT_DASHBOARD:

        dash_packet.packet.crc = calculate_crc(dash_packet.buffer, sizeof(dash_packet));
        serial->write(dash_packet.buffer, sizeof(dash_packet));
        break;

    case PSA_IDENT_TRIP:

        trip_packet.packet.crc = calculate_crc(trip_packet.buffer, sizeof(trip_packet));
        serial->write(trip_packet.buffer, sizeof(trip_packet));
        break;

    case PSA_IDENT_DOORS:

        door_packet.packet.crc = calculate_crc(door_packet.buffer, sizeof(door_packet));
        serial->write(door_packet.buffer, sizeof(door_packet));
        break;

    case PSA_IDENT_VIN:
        vin_packet.packet.crc = calculate_crc(vin_packet.buffer, sizeof(vin_packet));
        serial->write(vin_packet.buffer, sizeof(vin_packet));
        break;

    case PSA_IDENT_TIME:
        if (rtc_time_valid)
        {
            struct tm time_now;
            getLocalTime(&time_now);
            time_packet.packet.data.valid_time = rtc_time_valid;
            time_packet.packet.data.day = (uint8_t)time_now.tm_mday;
            time_packet.packet.data.month = (uint8_t)time_now.tm_mon;
            time_packet.packet.data.year = (uint8_t)time_now.tm_year;
            time_packet.packet.data.hour = (uint8_t)time_now.tm_hour;
            time_packet.packet.data.minutes = (uint8_t)time_now.tm_min;
            time_packet.packet.data.seconds = (uint8_t)time_now.tm_sec;
        }
        else // If time is invalid, send all zeros
        {
            time_packet.packet.data.valid_time = rtc_time_valid;
            time_packet.packet.data.day = 0;
            time_packet.packet.data.month = 0;
            time_packet.packet.data.year = 0;
            time_packet.packet.data.hour = 0;
            time_packet.packet.data.minutes = 0;
            time_packet.packet.data.seconds = 0;
        }

        time_packet.packet.crc = calculate_crc(time_packet.buffer, sizeof(time_packet));
        serial->write(time_packet.buffer, sizeof(time_packet));
        break;

    case PSA_IDENT_CAR_STATUS:
        status_packet.packet.data.car_state = car_state;
        status_packet.packet.crc = calculate_crc(status_packet.buffer, sizeof(status_packet));
        serial->write(status_packet.buffer, sizeof(status_packet));
        status_packet.packet.data.cd_changer_command = PSA_CD_CHANGER_COMM_NONE;
        break;

    case PSA_IDENT_ESP32_TEMP:

        // Depends on the ESP board. The temperature sensor is borked on most of them
        esp32_packet.packet.data.temperature = uint16_t(read_esp32_temperature() * 100);
        esp32_packet.packet.data.cpu_freq = (uint16_t)getCpuFrequencyMhz();
        esp32_packet.packet.crc = calculate_crc(esp32_packet.buffer, sizeof(esp32_packet));
        serial->write(esp32_packet.buffer, sizeof(esp32_packet));
        break;
    default:
        break;
    }
}

#ifdef PSA_SIMULATE

void simulate_engine_data()
{

    // struct psa_header *header = &(packet->header);
    // struct psa_engine_data *data = &(packet->data);

    // uint8_t crc = 0;

    // engine_packet.packet.header

    // crc ^= header->size;
    // crc ^= header->ident;

    engine_packet.packet.data.rpm = rpm;
    engine_packet.packet.data.speed = speed;
    engine_packet.packet.data.fuel_level = fuel;
    engine_packet.packet.data.engine_temperature = temp;
    engine_packet.packet.data.sequence = engine_sequence;

    // for (int i = 2; i < engine_packet.packet.header.size + sizeof(struct psa_header); ++i)
    // {
    //     crc ^= engine_packet.buffer[i];
    // }

    // footer = (struct psa_footer *)output_engine_buffer + sizeof(*header) + header->size;

    // engine_packet.packet.crc = crc;

    if (millis() - engine_time_passed > 50)
    {

        rpm += 20;
        speed += 15;
        engine_time_passed = millis();
    }

    if (rpm > 44000)
    {
        rpm = 6000;
    }

    if (speed > 9000)
    {
        speed = 0;
    }
}

void simulate_radio_data()
{
    memset(radio_packet.packet.data.station, 0, sizeof(radio_packet.packet.data.station));

    radio_packet.packet.data.band = curr_band;
    radio_packet.packet.data.freq = radio_station_freqs[curr_preset - 1];
    radio_packet.packet.data.preset = curr_preset;
    strcpy(radio_packet.packet.data.station, radio_station_names[curr_preset - 1]);

    if (millis() - radio_time_passed > 2000)
    {
        curr_preset++;
        curr_band++;

        radio_time_passed = millis();
        if (curr_band > PSA_FM_AST)
        {
            curr_band = 1;
        }
        if (curr_preset > 6)
        {
            curr_preset = 1;
        }
    }
}

void simulate_door_data()
{
    if (millis() - door_time_passed > 5000)
    {
        door_packet.packet.data.open_doors = 0;
    }
    else
    {
        door_packet.packet.data.open_doors = (1 << 3);
    }
    door_packet.packet.data.door_open = door_packet.packet.data.open_doors;
}

void simulate_trip_data()
{
    if (millis() - last_trip > 600)
    {
        last_trip = millis();
        curr_fuel += 2;

        curr_range -= 20;

        if (curr_fuel > 200)
        {
            curr_fuel = 2;
        }

        if (curr_range < 60)
        {
            curr_range = 1000;
        }

        trip_packet.packet.data.current_fuel_consumption = curr_fuel;
        trip_packet.packet.data.distance_to_empty = curr_range;
    }
}

#endif

/**
 * @brief Check the serial port for new MSP requests, and respond appropriately.
 *
 */
void receive_msp_message(HardwareSerial *serial)
{
    while (serial->available() >= sizeof(struct psa_header))
    {
        // If the start byte is found, start reading the message
        if (serial->peek() == 0x69)
        {
            struct psa_header *header = (struct psa_header *)msp_input_buffer;
            uint8_t bytes_read = serial->readBytes(msp_input_buffer, MSP_MAX_SIZE);
            if (header->size > 0) // This is a SET request
            {
                // A SET request message has a crc.
                parse_msp_data(header->ident, (uint8_t *)msp_input_buffer, header->size + sizeof(*header) + 1, serial);
            }
            else // This is a GET request
            {
                // A GET request message doesn't have a crc.
                send_packet(header->ident, serial);
            }
        }
        // Remove bytes from queue until the start byte is found
        else
        {
            serial->read();
        }
    }
}

#ifndef PSA_SIMULATE

#ifndef USE_SOFTWARE_VAN

// uint8_t headerByte = 0x80;

/**
 * @brief Construct a CD changer packet.
 * Must be sent at least every second, with at least the header changing,
 * otherwise the radio ignores it.
 *
 */
void AnswerToCDC()
{

    uint8_t old_header_byte = cd_changer_emu_packet.packet.header;

    last_cdc_message = current_cdc_message;

    uint8_t new_header_byte = (old_header_byte == 0x87) ? 0x80 : old_header_byte + 1;

    cd_changer_emu_packet.packet.header = new_header_byte;
    cd_changer_emu_packet.packet.footer = new_header_byte;

    VANInterface->set_channel_for_immediate_reply_message(8, 0x4EC, cd_changer_emu_packet.buffer, 12);
}

int send_VAN_trip_reset(enum psa_trip_meter trip_meter)
{
    uint8_t trip_reset_data[2] = {0x00, 0xFF};

    switch (trip_meter)
    {
    case PSA_TRIP_A:
        VANInterface->disable_channel(7);
        trip_reset_data[0] = 0xA0;
        VANInterface->set_channel_for_transmit_message(7, 0x5E4, trip_reset_data, 2, 1);
        return 0;
        break;

    case PSA_TRIP_B:
        VANInterface->disable_channel(7);
        trip_reset_data[0] = 0x60;
        VANInterface->set_channel_for_transmit_message(7, 0x5E4, trip_reset_data, 2, 1);
        return 0;
        break;
    default:
        return 1;
        break;
    }
}

#endif // USE_SOFTWARE_VAN

#endif // PSA_SIMULATE

unsigned long last_log_message = 0;
unsigned long current_log_message = 0;
// uint8_t channel_cheking = 0;

void cdc_message_task(void *params)
{
    while (1)
    {
        current_cdc_message = millis();

        // Send a new cd changer message every second.
        // The contents don't matter, just that the header and footer change
        if (current_cdc_message - last_cdc_message >= 1000) // Cd changer message
        {
            // cdc_time_passed = 0;
            xSemaphoreTake(mutex, portMAX_DELAY); // Mutex stuff, hopefully does something
            last_cdc_message = current_cdc_message;
            AnswerToCDC(); // TSS463C transmitter
            xSemaphoreGive(mutex);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void esp_sleep_task(void *params)
{
    while (1)
    {
        if (digitalRead(4) == LOW)
        {
            // Serial.println("Going to sleep");
            esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, HIGH);
            esp_deep_sleep_start();
        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

#ifndef PSA_SERIAL_PORT
#define PSA_SERIAL_PORT Serial2
#endif // PSA_SERIAL_PORT

void setup()
{
    setCpuFrequencyMhz(80);
    // btStop();
    //#ifndef PSA_SIMULATE
    pinMode(13, INPUT); // Make it so the PI stays on
    // pinMode(13, OUTPUT);
    // digitalWrite(13, LOW);
    set_time_zone(-utc_offset, dst_offset); // Set the time zone according to set offset

    // serial_port.begin(500000);
    // serial_port.setTimeout(1);

    // Serial.begin(115200); // Start the UART port
    // Serial.setTimeout(1); // Make the timeout as low as possible

    PSA_SERIAL_PORT.begin(500000); // Start the UART port
    PSA_SERIAL_PORT.setTimeout(1); // Make the timeout as low as possible

    pinMode(4, INPUT_PULLDOWN); // Wakeup pin, If low, go to sleep

#ifdef ESP_USE_WIFI
    WiFi.onEvent(on_wifi_ip_callback, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);        // Event handler for when ESP gets an IP
    WiFi.onEvent(on_wifi_disconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED); // Event handlef for when the ESP fails to connect to WiFi

    WiFi.begin(ssid, password);
    // if (rtc_time_valid == 0)
    // {
    // }
    // else // If the time is already set, don't even connect. Could probably be done better, but for now it's fine.
    // {
    //     WiFi.mode(WIFI_OFF);
    // }
#else
    WiFi.mode(WIFI_OFF);

#endif

#ifdef PSA_SPIFFS_LOG
    SPIFFS.begin(true);
    // psa_start_log();
#endif // PSA_SPIFFS_LOG

#ifndef PSA_SIMULATE

#ifdef USE_SOFTWARE_VAN
    TVanBus::Setup(RX_PIN, TX_PIN);

#else
    assert(mutex); // Initialize the mutex

    esp_log_level_set("*", ESP_LOG_NONE);

    InitTss463(); // TSS463 VAN driver init

    // Software VAN reader init, VAN_NETWORK_TYPE_COMFORT is 125k
    VAN_RX.Init(VAN_DATA_RX_RMT_CHANNEL, VAN_DATA_RX_PIN, VAN_DATA_RX_LED_INDICATOR_PIN, VAN_LINE_LEVEL_HIGH, VAN_NETWORK_TYPE_COMFORT);

    VANInterface = new TSS46X_VAN(vanSender, VAN_125KBPS);
    VANInterface->begin();

#endif // USE_SOFTWARE_VAN
#endif // PSA_SIMULATE

    //#endif

    // Serial1.begin(9600);
    // Serial.swap();

    // Start initializing all MSP packets

    memset(msp_input_buffer, 0, sizeof(msp_input_buffer));
    memset(msp_response_buffer, 0, sizeof(msp_response_buffer));
    memset(engine_packet.buffer, 0, sizeof(engine_packet));
    memset(radio_packet.buffer, 0, sizeof(radio_packet));
    for (int i = 0; i < PSA_PRESET_MAX; ++i)
    {
        memset(presets_packet[i].buffer, 0, sizeof(*presets_packet));
    }
    memset(vin_packet.buffer, 0, sizeof(vin_packet));
    memset(dash_packet.buffer, 0, sizeof(dash_packet));
    memset(door_packet.buffer, 0, sizeof(door_packet));
    memset(trip_packet.buffer, 0, sizeof(trip_packet));
    memset(headunit_packet.buffer, 0, sizeof(headunit_packet));
    memset(cd_packet.buffer, 0, sizeof(cd_packet));
    memset(status_packet.buffer, 0, sizeof(status_packet));

    if (presets_data_valid == 0)
    {
        for (int i = 0; i < PSA_PRESET_MAX; ++i)
        {
            memset(store_presets_packet[i].buffer, 0, sizeof(*store_presets_packet));
        }
        presets_data_valid = 1;
    }
    else
    {
        for (int i = 0; i < PSA_PRESET_MAX; ++i)
        {
            memcpy(presets_packet[i].buffer, store_presets_packet[i].buffer, sizeof(*presets_packet));
        }
    }

    // Populate all buffer pointers for the VAN packet parser to fill out.

    data_buffers.dash_data = &(dash_packet.packet.data);
    data_buffers.door_data = &(door_packet.packet.data);
    data_buffers.engine_data = &(engine_packet.packet.data);
    data_buffers.radio_data = &(radio_packet.packet.data);
    data_buffers.presets_data_am = &(presets_packet[PSA_PRESET_AM].packet.data);
    data_buffers.presets_data_fm_1 = &(presets_packet[PSA_PRESET_FM_1].packet.data);
    data_buffers.presets_data_fm_2 = &(presets_packet[PSA_PRESET_FM_2].packet.data);
    data_buffers.presets_data_fm_ast = &(presets_packet[PSA_PRESET_FMAST].packet.data);
    data_buffers.vin_data = &(vin_packet.packet.vin);
    data_buffers.trip_data = &(trip_packet.packet.data);
    data_buffers.headunit_data = &(headunit_packet.packet.data);
    data_buffers.status_data = &(status_packet.packet.data);
    data_buffers.cd_player_data = &(cd_packet.packet.data);
    // dash_packet.packet.data.mileage = 1000000;

    init_packets(); // Populate MSP headers

#ifndef PSA_SIMULATE

    esp_timer_create_args_t esp_timer_args;

    esp_timer_args.callback = (esp_timer_cb_t)rpi_off_timer_timeout;
    esp_timer_args.arg = NULL;
    esp_timer_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_args.name = "PiPoweroff";
    esp_timer_args.skip_unhandled_events = 1;

    memset(&esp_timer_args, 0, sizeof(esp_timer_args));
    // esp_timer_create(&esp_timer_args, &timer_handle);

    // Task creation

    // Create a VAN receiver task on core 0
    xTaskCreatePinnedToCore(
        van_receive_task,
        "VanRecvTask",
        1000,
        NULL,
        2,
        &van_task,
        0);

    // Create a car state calculation task on core 0
    xTaskCreatePinnedToCore(
        psa_calculate_state,
        "StateCalcTask",
        1000,
        NULL,
        1,
        &car_state_task,
        0);

#ifdef ESP_USE_WIFI
    // // Create a wifi retry task on core 0
    // xTaskCreatePinnedToCore(
    //     wifi_retry_task,
    //     "WifiRetry",
    //     1000,
    //     NULL,
    //     1,
    //     NULL,
    //     0);
#endif // ESP_USE_WIFI
    xTaskCreatePinnedToCore(
        psa_handle_button_logic_task,
        "AudioSettings",
        1000,
        NULL,
        1,
        NULL,
        0);

    // xTaskCreatePinnedToCore(
    //     cdc_message_task,
    //     "CDCTask",
    //     1000,
    //     NULL,
    //     1,
    //     NULL,
    //     1);
#ifdef ESP_POWER_CONTROL
    // xTaskCreatePinnedToCore(
    //     esp_sleep_task,
    //     "SleepTask",
    //     1000,
    //     NULL,
    //     1,
    //     NULL,
    //     1);
#endif // ESP_POWER_CONTROL

    // rpi_off_timer_timeout

#endif // PSA_SIMULATE
}

void loop()
{

#ifdef ESP_POWER_CONTROL
    // If the wake pin is LOW, go to deep sleep.
    if (digitalRead(4) == LOW && car_state <= PSA_STATE_CAR_OFF)
    {
        // Serial.println("Going to sleep");
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, HIGH);
        for (int i = 0; i < PSA_PRESET_MAX; ++i)
        {
            memcpy(store_presets_packet[i].buffer, presets_packet[i].buffer, sizeof(*presets_packet));
        }
        presets_data_valid = 1;
        esp_deep_sleep_start();
    }
#endif // ESP_POWER_CONTROL
    receive_msp_message(&PSA_SERIAL_PORT);

#ifdef PSA_SPIFFS_LOG

    if (dash_packet.packet.data.engine_running)
    {
        if (!do_log)
        {
            psa_start_log();
        }
    }

    current_log_message = millis();
    if (current_log_message - last_log_message >= 250)
    {
        last_log_message = current_log_message;
        if (dash_packet.packet.data.engine_running)
        {
            psa_log_data();
        }
    }

    if (dash_packet.packet.data.engine_running == 0)
    {
        if (do_log)
        {
            psa_end_log();
        }
    }

#endif // PSA_SPIFFS_LOG

#ifdef PSA_SIMULATE

    simulate_engine_data();
    simulate_radio_data();
    simulate_door_data();
    simulate_trip_data();
#else

#ifdef USE_SOFTWARE_VAN

    psa_parse_van_packet_esp8266(&van_rx_pkt, &data_buffers);

#else

    current_cdc_message = millis();

    // Send a new cd changer message every second.
    // The contents don't matter, just that the header and footer change
    if (current_cdc_message - last_cdc_message >= 1000) // Cd changer message
    {
        // cdc_time_passed = 0;
        last_cdc_message = current_cdc_message;
        AnswerToCDC(); // TSS463C transmitter
    }

#endif // USE_SOFTWARE_VAN

#endif // PSA_SIMULATE

    // delay(1000);
}