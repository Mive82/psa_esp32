//#define PSA_SIMULATE

#ifndef PSA_SIMULATE

#include "header/van_packet_iden.h"
#include "header/van_packet_parser.h"
#include "header/psa_common.h"
#include "header/psa_packet_defs.h"
#include "header/van_helper.h"

// #ifdef USE_VAN_BUS_LIB
#ifdef USE_SOFTWARE_VAN

#include <VanBus.h>
#include <VanBusRx.h>
#include <VanBusTx.h>

#else

#include <itss46x.h>
#include <tss46x_van.h>
#include <tss463.h>

#endif // USE SOFTWARE_VAN

static int audio_menu_open = 0;
static int audio_menu_setting = 0; // 0, 1, 2, 3, 4, 5, 6

static const int audio_menu_duration_ms = 4000;

volatile struct psa_headunit_data *audio_settings_output_buffer = NULL;

static unsigned long last_button_millis = 0;

union dash_mileage
{
    uint32_t number : 24;
    uint8_t buffer[3];
} __attribute__((packed));

static uint16_t swap_endian_uint16(uint16_t const x)
{
    return (x << 8) | (x >> 8);
}

static uint32_t swap_endian_uint32(uint32_t const x)
{
    uint32_t val = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

static uint32_t mileage_swap_bytes(union dash_mileage const mileage)
{
    uint32_t temp_mileage = (uint32_t)mileage.buffer[0] << 16 | (uint32_t)mileage.buffer[1] << 8 | mileage.buffer[2];
    return temp_mileage;
}

void psa_handle_button_logic_task(void *params)
{
    while (1)
    {

        if (millis() - last_button_millis > audio_menu_duration_ms)
        {
            audio_menu_open = 0;
            audio_menu_setting = 0;
        }

        if (audio_settings_output_buffer != NULL)
        {
            audio_settings_output_buffer->settings_menu_open = audio_menu_open;
            switch (audio_menu_setting)
            {
            case 0:

                audio_settings_output_buffer->settings_changing.auto_volume_changing = 0;
                audio_settings_output_buffer->settings_changing.balance_changing = 0;
                audio_settings_output_buffer->settings_changing.bass_changing = 0;
                audio_settings_output_buffer->settings_changing.fader_changing = 0;
                audio_settings_output_buffer->settings_changing.loudness_changing = 0;
                audio_settings_output_buffer->settings_changing.treble_changing = 0;
                audio_settings_output_buffer->settings_changing.volume_changing = 0;

                break;
            case 1: // Bass
                audio_settings_output_buffer->settings_changing.auto_volume_changing = 0;
                audio_settings_output_buffer->settings_changing.balance_changing = 0;
                audio_settings_output_buffer->settings_changing.bass_changing = 1;
                audio_settings_output_buffer->settings_changing.fader_changing = 0;
                audio_settings_output_buffer->settings_changing.loudness_changing = 0;
                audio_settings_output_buffer->settings_changing.treble_changing = 0;
                audio_settings_output_buffer->settings_changing.volume_changing = 0;
                break;

            case 2: // Treble
                audio_settings_output_buffer->settings_changing.auto_volume_changing = 0;
                audio_settings_output_buffer->settings_changing.balance_changing = 0;
                audio_settings_output_buffer->settings_changing.bass_changing = 0;
                audio_settings_output_buffer->settings_changing.fader_changing = 0;
                audio_settings_output_buffer->settings_changing.loudness_changing = 0;
                audio_settings_output_buffer->settings_changing.treble_changing = 1;
                audio_settings_output_buffer->settings_changing.volume_changing = 1;
                break;

            case 3: // Loudness
                audio_settings_output_buffer->settings_changing.auto_volume_changing = 0;
                audio_settings_output_buffer->settings_changing.balance_changing = 0;
                audio_settings_output_buffer->settings_changing.bass_changing = 0;
                audio_settings_output_buffer->settings_changing.fader_changing = 0;
                audio_settings_output_buffer->settings_changing.loudness_changing = 1;
                audio_settings_output_buffer->settings_changing.treble_changing = 0;
                audio_settings_output_buffer->settings_changing.volume_changing = 0;
                break;

            case 4: // Fader
                audio_settings_output_buffer->settings_changing.auto_volume_changing = 0;
                audio_settings_output_buffer->settings_changing.balance_changing = 0;
                audio_settings_output_buffer->settings_changing.bass_changing = 0;
                audio_settings_output_buffer->settings_changing.fader_changing = 1;
                audio_settings_output_buffer->settings_changing.loudness_changing = 0;
                audio_settings_output_buffer->settings_changing.treble_changing = 0;
                audio_settings_output_buffer->settings_changing.volume_changing = 0;
                break;
            case 5: // Balance

                audio_settings_output_buffer->settings_changing.auto_volume_changing = 0;
                audio_settings_output_buffer->settings_changing.balance_changing = 1;
                audio_settings_output_buffer->settings_changing.bass_changing = 0;
                audio_settings_output_buffer->settings_changing.fader_changing = 0;
                audio_settings_output_buffer->settings_changing.loudness_changing = 0;
                audio_settings_output_buffer->settings_changing.treble_changing = 0;
                audio_settings_output_buffer->settings_changing.volume_changing = 0;
                break;
            case 6: // Auto-volume
                audio_settings_output_buffer->settings_changing.auto_volume_changing = 1;
                audio_settings_output_buffer->settings_changing.balance_changing = 0;
                audio_settings_output_buffer->settings_changing.bass_changing = 0;
                audio_settings_output_buffer->settings_changing.fader_changing = 0;
                audio_settings_output_buffer->settings_changing.loudness_changing = 0;
                audio_settings_output_buffer->settings_changing.treble_changing = 0;
                audio_settings_output_buffer->settings_changing.volume_changing = 0;
                break;
            default:
                break;
            }
        }

        vTaskDelay(5 / portTICK_PERIOD_MS); // A delay, so it doesn't take up all processing time
    }
}

static int psa_parse_buttons_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }
    if (data_buffers->headunit_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (audio_settings_output_buffer == NULL)
    {
        audio_settings_output_buffer = (struct psa_headunit_data *)data_buffers->headunit_data;
    }

    if ((data[1] & 0x0F) == 0x02)
    {
        // Refresh the timer when audio up or down buttons are pressed
        if (((data[2] & 0x1F) == 0x11 || (data[2] & 0x1F) == 0x12) && audio_menu_open)
        {
            last_button_millis = millis();
        }
        // Change the menu when the audio button is let go. Will probably go tits up when the button is held down for more than 2 secs
        if (data[2] == 0x56)
        {
            last_button_millis = millis();
            if (audio_menu_setting >= 6)
            {
                audio_menu_open = 0;
                audio_menu_setting = 0;
            }
            else
            {
                audio_menu_open = 1;
                audio_menu_setting++;
            }
        }
    }

    return PSA_OK;
}

static int psa_parse_rpm_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->engine_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_engine_data *engine_data = (struct psa_engine_data *)(data_buffers->engine_data);

    struct psa_van_rpm_speed_struct const *const rpm_data = (struct psa_van_rpm_speed_struct *)data;

    engine_data->rpm = swap_endian_uint16(rpm_data->rpm);
    engine_data->speed = swap_endian_uint16(rpm_data->speed);

    return PSA_OK;
}

static int psa_parse_dash_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->dash_data == NULL || data_buffers->door_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_dash_data *dash_data = (struct psa_dash_data *)(data_buffers->dash_data);
    struct psa_engine_data *engine_data = (struct psa_engine_data *)(data_buffers->engine_data);
    // struct psa_door_data *door_data = (struct psa_door_data *)(data_buffers->door_data);

    struct psa_van_dashboard *van_data = (struct psa_van_dashboard *)data;

    dash_data->accesories_on = van_data->accesories_on;
    dash_data->backlight_off = van_data->is_backlight_off;
    dash_data->brightness = van_data->brightness;
    dash_data->door_open = van_data->door_open;
    dash_data->economy_mode = van_data->economy_mode;
    dash_data->engine_running = van_data->engine_running;
    dash_data->external_temp = van_data->external_temp;
    dash_data->ignition_on = van_data->ignition_on;

    dash_data->backlight_off = van_data->is_backlight_off;
    dash_data->reverse_gear = van_data->reverse_gear;
    dash_data->water_temp = van_data->water_temp;
    engine_data->engine_temperature = van_data->water_temp;
    engine_data->reverse = van_data->reverse_gear;
    uint32_t mileage;
    mileage = van_data->mileage;

    union dash_mileage temp_mileage;
    temp_mileage.number = van_data->mileage;

    dash_data->mileage = mileage_swap_bytes(temp_mileage);

    // door_data->door_open = van_data->door_open;

    return PSA_OK;
}

static int psa_parse_trip_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->door_data == NULL || data_buffers->trip_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_trip_data *trip_data = (struct psa_trip_data *)(data_buffers->trip_data);
    struct psa_door_data *door_data = (struct psa_door_data *)(data_buffers->door_data);

    struct psa_van_trip_computer *van_data = (struct psa_van_trip_computer *)data;

    door_data->open_doors = (van_data->door_status.packed);

    if (door_data->open_doors)
    {
        door_data->door_open = 1;
    }
    else
    {
        door_data->door_open = 0;
    }

    trip_data->current_fuel_consumption = swap_endian_uint16(van_data->current_fuel_consumption);
    trip_data->distance_to_empty = swap_endian_uint16(van_data->distance_to_empty);
    trip_data->trip_1_distance = swap_endian_uint16(van_data->trip_1_distance);
    trip_data->trip_1_fuel_consumption = swap_endian_uint16(van_data->trip_1_fuel_consumption);
    trip_data->trip_1_speed = van_data->trip_1_speed;

    trip_data->trip_2_distance = swap_endian_uint16(van_data->trip_2_distance);
    trip_data->trip_2_fuel_consumption = swap_endian_uint16(van_data->trip_2_fuel_consumption);
    trip_data->trip_2_speed = van_data->trip_2_speed;
    trip_data->trip_button_pressed = van_data->trip_button.trip_button;

    return PSA_OK;
}

// 0x554 len 22
static int psa_parse_radio_tuner_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->radio_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data[1] != 0xD1) // D1 is frequency info
    {
        return PSA_UNKNOWN_IDEN;
    }

    struct psa_van_radio_freq_info *van_data = (struct psa_van_radio_freq_info *)data;
    struct psa_radio_data *radio_data = (struct psa_radio_data *)data_buffers->radio_data;

    struct psa_preset_data *preset_data = (struct psa_preset_data *)data_buffers->presets_data;

    memset(radio_data->station, 0, 9);
    strncpy(radio_data->station, (const char *)van_data->station, 8);

    uint16_t freq = uint16_t(van_data->frequency[1]) << 8 | van_data->frequency[0];

    radio_data->freq = freq * 5 + 5000;

    radio_data->signal_strength = van_data->signal_info & 0x0F;
    // radio_data->freq = uint16_t(van_data->frequency[1]) << 8 | van_data->frequency[0]; // swap_endian_uint16(van_data->frequency);

    radio_data->preset = van_data->memory_position;

    if (radio_data->preset > 0 && radio_data->preset <= 6)
    {
        strncpy(preset_data->presets[radio_data->preset - 1].preset_name, (const char *)van_data->station, 8);
        preset_data->presets->preset_num = radio_data->preset;
    }

    switch (van_data->band)
    {
    case PSA_VAN_BAND_FM1:
        radio_data->band = PSA_FM_1;
        break;
    case PSA_VAN_BAND_FM2:
        radio_data->band = PSA_FM_2;
        break;
    case PSA_VAN_BAND_FM3:
        radio_data->band = PSA_FM_3;
        break;
    case PSA_VAN_BAND_FMAST:
        radio_data->band = PSA_FM_AST;
        break;
    case PSA_VAN_BAND_AM:
        radio_data->band = PSA_AM_1;
        break;
    case PSA_VAN_BAND_NONE:
    case PSA_VAN_BAND_PTY_SELECT:
    default:
        radio_data->band = PSA_NONE;
        break;
    }

    return PSA_OK;
}

// 0x554 len 10
static int psa_parse_radio_cd_short_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->cd_player_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data[1] != 0xD6) // D6 is CD info
    {
        return PSA_UNKNOWN_IDEN;
    }

    struct psa_van_radio_cd_info_len_10 *van_data = (struct psa_van_radio_cd_info_len_10 *)data;
    struct psa_cd_player_data *cd_data = (struct psa_cd_player_data *)data_buffers->cd_player_data;

    cd_data->current_minute = bcd_to_dec(van_data->minutes);
    cd_data->current_second = bcd_to_dec(van_data->seconds);
    cd_data->current_track = bcd_to_dec(van_data->current_track);
    cd_data->total_tracks = bcd_to_dec(van_data->total_tracks);

    cd_data->status = 0;

    switch (van_data->status)
    {
    case 0x10:
        cd_data->status |= PSA_CD_PLAYER_ERROR;
        break;
    case 0x11:
        cd_data->status |= PSA_CD_PLAYER_LOADING;
        break;
    case 0x12:
        cd_data->status |= PSA_CD_PLAYER_LOADING | PSA_CD_PLAYER_PAUSED;
        break;
    case 0x13:
        cd_data->status |= PSA_CD_PLAYER_LOADING | PSA_CD_PLAYER_PLAYING;
        break;
    case 0x02:
        cd_data->status |= PSA_CD_PLAYER_PAUSED;
        break;
    case 0x03:
        cd_data->status |= PSA_CD_PLAYER_PLAYING;
        break;
    case 0x04:
        cd_data->status |= PSA_CD_PLAYER_FAST_FORWARDING;
        break;
    case 0x05:
        cd_data->status |= PSA_CD_PLAYER_REWINDING;
        break;
    default:
        cd_data->status = 0;
        break;
    }

    cd_data->total_minutes = 0xff;
    cd_data->total_seconds = 0xff;

    return PSA_OK;
}

// 0x554 len 19
static int psa_parse_radio_cd_long_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->cd_player_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data[1] != 0xD6) // D6 is CD info
    {
        return PSA_UNKNOWN_IDEN;
    }

    struct psa_van_radio_cd_info_len_19 *van_data = (struct psa_van_radio_cd_info_len_19 *)data;
    struct psa_cd_player_data *cd_data = (struct psa_cd_player_data *)data_buffers->cd_player_data;

    cd_data->current_minute = bcd_to_dec(van_data->minutes);
    cd_data->current_second = bcd_to_dec(van_data->seconds);
    cd_data->current_track = bcd_to_dec(van_data->current_track);
    cd_data->total_tracks = bcd_to_dec(van_data->total_tracks);
    cd_data->total_minutes = bcd_to_dec(van_data->total_minutes);
    cd_data->total_seconds = bcd_to_dec(van_data->total_seconds);

    cd_data->status = 0;

    switch (van_data->status)
    {
    case 0x10:
        cd_data->status |= PSA_CD_PLAYER_ERROR;
        break;
    case 0x11:
        cd_data->status |= PSA_CD_PLAYER_LOADING;
        break;
    case 0x12:
        cd_data->status |= PSA_CD_PLAYER_LOADING | PSA_CD_PLAYER_PAUSED;
        break;
    case 0x13:
        cd_data->status |= PSA_CD_PLAYER_LOADING | PSA_CD_PLAYER_PLAYING;
        break;
    case 0x02:
        cd_data->status |= PSA_CD_PLAYER_PAUSED;
        break;
    case 0x03:
        cd_data->status |= PSA_CD_PLAYER_PLAYING;
        break;
    case 0x04:
        cd_data->status |= PSA_CD_PLAYER_FAST_FORWARDING;
        break;
    case 0x05:
        cd_data->status |= PSA_CD_PLAYER_REWINDING;
        break;
    default:
        cd_data->status = 0;
        break;
    }

    return PSA_OK;
}

static int psa_parse_radio_preset_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->presets_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data[1] != 0xD3) // D3 is preset info
    {
        return PSA_UNKNOWN_IDEN;
    }

    struct psa_van_radio_preset_info *van_data = (struct psa_van_radio_preset_info *)data;

    struct psa_preset_data *preset_data = (struct psa_preset_data *)data_buffers->presets_data;

    if (van_data->position > 0 && van_data->position < 7)
    {
        memset(preset_data->presets[van_data->position].preset_name, 0, 10);
        strncpy(preset_data->presets[van_data->position].preset_name, (const char *)van_data->station_name, 8);

        preset_data->presets[van_data->position].preset_num = van_data->position;
    }
    else
    {
        return PSA_INVALID_VAN_PACKET;
    }

    return PSA_OK;
}

static int psa_parse_headunit_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->headunit_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_van_radio_settings *van_data = (struct psa_van_radio_settings *)data;

    struct psa_headunit_data *headunit_data = (struct psa_headunit_data *)data_buffers->headunit_data;

    headunit_data->unit_powered_on = van_data->power.power_on;
    headunit_data->auto_volume = van_data->audio_properties.auto_volume;
    headunit_data->balance = ((int8_t)0x3f - van_data->balance.value);
    headunit_data->fader = ((int8_t)0x3f - van_data->fader.value);
    headunit_data->bass = ((int8_t)van_data->bass.value - 0x3f);
    headunit_data->treble = ((int8_t)van_data->treble.value - 0x3f);
    headunit_data->loudness_on = van_data->audio_properties.loudness_on;
    headunit_data->volume = van_data->volume.value;

    headunit_data->muted = van_data->audio_properties.external_mute || van_data->audio_properties.stalk_mute;

    switch (van_data->source.source)
    {
    case 0x01: // Tuner
        headunit_data->source = PSA_RADIO_TUNER;
        break;
    case 0x02: // Tape or CD
        headunit_data->source = PSA_RADIO_INTERNAL;
        break;
    case 0x03: // Cd changer
        headunit_data->source = PSA_RADIO_EXTERNAL;
        break;
    case 0x05: // Navigation
        headunit_data->source = PSA_RADIO_NAVIGATION;
        break;
    default:
        headunit_data->source = PSA_RADIO_NONE;
        break;
    }

    headunit_data->cd_present = van_data->source.cd_present;
    headunit_data->tape_present = van_data->source.tape_present;
    // headunit_data->settings_menu_open = van_data->audio_properties.audio_properties_menu_open;

    // headunit_data->settings_changing.balance_changing = van_data->balance.updating;
    // headunit_data->settings_changing.bass_changing = van_data->bass.updating;
    // headunit_data->settings_changing.fader_changing = van_data->fader.updating;
    // headunit_data->settings_changing.treble_changing = van_data->treble.updating;
    // headunit_data->settings_changing.volume_changing = van_data->volume.updating;

    return PSA_OK;
}

static int psa_parse_vin_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    if (data_buffers->vin_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    // char *vin = (char *)data_buffers->vin_data;
    int const vin_size = 17;

    memcpy(data_buffers->vin_data, data, vin_size);

    return PSA_OK;
}

// 0x4FC len 11
static int psa_parse_instruments_short_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }
    if (data_buffers->engine_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_engine_data *engine_data = (struct psa_engine_data *)data_buffers->engine_data;
    struct psa_van_instrument_cluster_short *van_data = (struct psa_van_instrument_cluster_short *)data;

    engine_data->fuel_level = van_data->fuel_level;
    engine_data->oil_temperature = van_data->oil_temp;

    return PSA_OK;
}

// 0x4FC len 14
static int psa_parse_instruments_long_iden(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }
    if (data_buffers->engine_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_engine_data *engine_data = (struct psa_engine_data *)data_buffers->engine_data;
    struct psa_van_instrument_cluster_long *van_data = (struct psa_van_instrument_cluster_long *)data;

    engine_data->fuel_level = van_data->fuel_level;
    engine_data->oil_temperature = van_data->oil_temp;

    return PSA_OK;
}

// 0x524 len 14
static int psa_parse_car_status_2_short(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }
    if (data_buffers->engine_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_status_data *status_data = (struct psa_status_data *)data_buffers->status_data;
    struct psa_van_car_status_2_short *van_data = (struct psa_van_car_status_2_short *)data;

    status_data->doors_locked = van_data->byte_08 & 0x01;
    status_data->deadlocking_active = van_data->byte_08 & 0x08;

    return PSA_OK;
}

// 0x524 len 16
static int psa_parse_car_status_2_long(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }
    if (data_buffers->engine_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_status_data *status_data = (struct psa_status_data *)data_buffers->status_data;
    struct psa_van_car_status_2_long *van_data = (struct psa_van_car_status_2_long *)data;

    status_data->doors_locked = van_data->byte_08 & 0x01;
    status_data->deadlocking_active = van_data->byte_08 & 0x08;

    return PSA_OK;
}

static int psa_parse_cdc_command(
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }
    if (data_buffers->status_data == NULL)
    {
        return PSA_NULL_PTR;
    }

    struct psa_status_data *status_data = (struct psa_status_data *)data_buffers->status_data;
    struct psa_van_cd_changer_command_struct *van_data = (struct psa_van_cd_changer_command_struct *)data;

    uint16_t command = swap_endian_uint16(van_data->command);

    switch (command)
    {
    case PSA_VAN_CD_CHANGER_PLAY:
        status_data->cd_changer_command = PSA_CD_CHANGER_COMM_PLAY;
        break;
    case PSA_VAN_CD_CHANGER_PAUSE:
        status_data->cd_changer_command = PSA_CD_CHANGER_COMM_PAUSE;
        break;
    case PSA_VAN_CD_CHANGER_NEXT_TRACK:
        status_data->cd_changer_command = PSA_CD_CHANGER_COMM_NEXT_TRACK;
        break;
    case PSA_VAN_CD_CHANGER_PREVIOUS_TRACK:
        status_data->cd_changer_command = PSA_CD_CHANGER_COMM_PREV_TRACK;
        break;
    default:
        status_data->cd_changer_command = PSA_CD_CHANGER_COMM_NONE;
        break;
    }

    return PSA_OK;
}

int psa_parse_van_packet_esp32(
    uint16_t iden,
    uint8_t size,
    uint8_t const *const data,
    struct psa_output_data_buffers *data_buffers)
{
    if (data == NULL || data_buffers == NULL)
    {
        return PSA_NULL_PTR;
    }

    switch (iden)
    {
    case PSA_VAN_IDEN_RPM:
        if (size != sizeof(struct psa_van_rpm_speed_struct))
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

        return psa_parse_rpm_iden(data, (struct psa_output_data_buffers *)data_buffers);
        break;

    case PSA_VAN_IDEN_CAR_STATUS1:
        if (size != sizeof(struct psa_van_trip_computer))
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

        return psa_parse_trip_iden(data, (struct psa_output_data_buffers *)data_buffers);
        break;

    case PSA_VAN_IDEN_ENGINE:
        if (size != sizeof(struct psa_van_dashboard))
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

        return psa_parse_dash_iden(data, (struct psa_output_data_buffers *)data_buffers);
        break;
    case PSA_VAN_IDEN_VIN:
        if (size != 17)
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

        return psa_parse_vin_iden(data, (struct psa_output_data_buffers *)data_buffers);

        break;

    case PSA_VAN_IDEN_HEAD_UNIT:
        if (size == 22)
        {
            return psa_parse_radio_tuner_iden(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else if (size == 10)
        {
            return psa_parse_radio_cd_short_iden(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else if (size == 19)
        {
            return psa_parse_radio_cd_long_iden(data, (struct psa_output_data_buffers *)data_buffers);
        }
        // else if (size == 12)
        // {
        //     return psa_parse_radio_preset_iden(data, (struct psa_output_data_buffers *)data_buffers);
        // }
        else
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

        break;
    case PSA_VAN_IDEN_AUDIO_SETTINGS:
        if (size != 11)
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

        return psa_parse_headunit_iden(data, (struct psa_output_data_buffers *)data_buffers);
        break;

    case PSA_VAN_IDEN_LIGHTS_STATUS:
        if (size == 11)
        {
            return psa_parse_instruments_short_iden(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else if (size == 14)
        {
            return psa_parse_instruments_long_iden(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }
        break;

    case PSA_VAN_IDEN_CAR_STATUS2:
        if (size == 14)
        {
            return psa_parse_car_status_2_short(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else if (size == 16)
        {
            return psa_parse_car_status_2_long(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

    case PSA_VAN_IDEN_CDCHANGER_COMMAND:
        if (size == 2)
        {
            return psa_parse_cdc_command(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

    case PSA_VAN_IDEN_DEVICE_REPORT:
        if (size == 3)
        {
            return psa_parse_buttons_iden(data, (struct psa_output_data_buffers *)data_buffers);
        }
        else
        {
            return PSA_INVALID_VAN_PACKET_SIZE;
        }

    default:
        return PSA_UNKNOWN_IDEN;
        break;
    }

    return PSA_OK;
}

#ifdef USE_SOFTWARE_VAN

int psa_parse_van_packet_esp8266(void *pkt, void *data_buffers)
{
    if (pkt == NULL || data_buffers == NULL)
    {
        return -1;
    }

    int status = PSA_NO_PACKET;

    TVanPacketRxDesc *rx_pkt = (TVanPacketRxDesc *)pkt;
    // struct psa_output_data_buffers *van_data_buffers = (struct psa_output_data_buffers *)data_buffers;

    if (VanBus.Receive(*rx_pkt))
    {
        status = psa_parse_van_packet_esp32(rx_pkt->Iden(), rx_pkt->DataLen(), rx_pkt->Data(), data_buffers);
    }

    return status;
}

#endif // USE_SOFTWARE_VAN

#endif // PSA_SIMULATE