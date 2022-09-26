#include "header/esp_helper.h"
#include "rom/ets_sys.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "esp_sntp.h"

#include "esp32-hal.h"
#include "lwip/apps/sntp.h"
#include "esp_netif.h"

#include "header/adc_lut.h"
#include <esp_adc_cal.h>

float read_esp32_temperature()
{
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    ets_delay_us(100);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    ets_delay_us(5);
    float temp_f = (float)GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
    float temp_c = (temp_f - 32) / 1.8;
    return temp_c;
}

static enum psa_voltmeter_resolution adc_resolution = PSA_VOLTMETER_INVALID;

static int R1_val, R2_val;
static float divide_value;

void ntp_config(
    const char *server1,
    const char *server2,
    const char *server3)
{
    esp_netif_init();
    if (sntp_enabled())
    {
        sntp_stop();
    }
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char *)server1);
    sntp_setservername(1, (char *)server2);
    sntp_setservername(2, (char *)server3);
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    Serial.println("Starting sntp");
    sntp_init();
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    //  sntp_sync_time(tm);
    //  sntp_sync_time
}

void set_time_zone(long offset, int dst_offset)
{
    char cst[17] = {0};
    char cdt[17] = "DST";
    char tz[33] = {0};

    if (offset % 3600)
    {
        sprintf(cst, "UTC%ld:%02u:%02u", offset / 3600, abs((offset % 3600) / 60), abs(offset % 60));
    }
    else
    {
        sprintf(cst, "UTC%ld", offset / 3600);
    }
    if (dst_offset != 3600)
    {
        long tz_dst = offset - dst_offset;
        if (tz_dst % 3600)
        {
            sprintf(cdt, "DST%ld:%02u:%02u", tz_dst / 3600, abs((tz_dst % 3600) / 60), abs(tz_dst % 60));
        }
        else
        {
            sprintf(cdt, "DST%ld", tz_dst / 3600);
        }
    }
    sprintf(tz, "%s%s", cst, cdt);
    setenv("TZ", tz, 1);
    tzset();
}

/**
 * @brief Read the analog input voltage with voltage divider resistiors R1 and R2.
 * Returns voltage in milivolts.
 *
 * Call this if the ADC is configured with 11 bit resolution.
 *
 * @param pin Input GPIO pin
 * @param R1 R1 value in ohms
 * @param R2 R2 value in ohms
 * @return uint16_t voltage in milivolts
 */
static uint16_t read_voltmeter_11_bit(uint8_t pin, int R1, int R2)
{
    uint16_t raw_value = analogRead(pin) * 2;

    if (raw_value > 4095)
    {
        raw_value = 4095;
    }

    uint16_t value = ADC_LUT[raw_value];
    uint32_t vout = (value * 3333) / 4096;
    uint16_t vin = (float)vout / (R2 / (R1 + R2));

    return vin;
}

/**
 * @brief Read the analog input voltage with voltage divider resistiors R1 and R2.
 * Returns voltage in milivolts.
 *
 * Call this if the ADC is configured with 12 bit resolution.
 *
 * @param pin Input GPIO pin
 * @param R1 R1 value in ohms
 * @param R2 R2 value in ohms
 * @return uint16_t voltage in milivolts
 */
static uint16_t read_voltmeter_12_bit(uint8_t pin, int R1, int R2)
{
    uint16_t raw_value = analogRead(pin);

    if (raw_value > 4095)
    {
        raw_value = 4095;
    }

    uint16_t value = ADC_LUT[raw_value];
    uint32_t vout = (value * 3333) / 4096;
    uint16_t vin = (float)vout / (R2 / (R1 + R2));

    return vin;
}

enum psa_status init_voltmeter(uint8_t pin, enum psa_voltmeter_resolution resolution)
{
    adc1_channel_t channelNum;

    switch (pin)
    {
    case (36):
        channelNum = ADC1_CHANNEL_0;
        break;

    case (39):
        channelNum = ADC1_CHANNEL_3;
        break;

    case (34):
        channelNum = ADC1_CHANNEL_6;
        break;

    case (35):
        channelNum = ADC1_CHANNEL_7;
        break;

    case (32):
        channelNum = ADC1_CHANNEL_4;
        break;

    case (33):
        channelNum = ADC1_CHANNEL_5;
        break;

    default:
        return PSA_ERROR;
    }

    switch (resolution)
    {
    case PSA_VOLTMETER_11_BIT:
        analogReadResolution(11);
        adc1_config_channel_atten(channelNum, ADC_ATTEN_DB_11);
        adc_resolution = PSA_VOLTMETER_11_BIT;
        break;

    case PSA_VOLTMETER_12_BIT:
        analogReadResolution(12);
        adc1_config_channel_atten(channelNum, ADC_ATTEN_DB_11);
        adc_resolution = PSA_VOLTMETER_12_BIT;
        break;

    default:
        return PSA_ERROR;
        break;
    }

    return PSA_OK;
}