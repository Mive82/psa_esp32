#ifndef PSA_CONFIG_H
#define PSA_CONFIG_H

// NTP stuff. Wifi is only used to set the time once,
// after that it is never turned on again.

#define PSA_SERIAL_PORT Serial // Serial port used, Serial - PC, Serial2 - RPi
                               // I haven't been able to run both together reliably

const char *ssid = "SSID";                     // WiFi SSID
const char *password = "PASS";                 // WiFi Password
const char *ntp_server1 = "1.hr.pool.ntp.org"; // NTP server
const char *ntp_server2 = NULL;
const char *ntp_server3 = NULL;

const uint8_t rpi_power_pin = 13; // Pin connected to Raspberry's RUN header

const int utc_offset = 3600; // Your timezone offset from UTC+0 in seconds
const int dst_offset = 3600; // Your DST offset is seconds

#endif // PSA_CONFIG_H