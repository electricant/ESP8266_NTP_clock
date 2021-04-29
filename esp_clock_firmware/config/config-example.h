/*
 *  Example configuration file.
 *  Copy it as config.h and change the following values as needed
 */

/*
 * WiFi related settings: SSID, password and hostname
 */
#define WIFI_SSID "SSID"
#define WIFI_PASS "Password"
#define WIFI_HOSTNAME "hostname"

/*
 * MQTT settings.
 * 
 * The server the clock will connect to is MQTT_BROKER:MQTT_PORT.
 * To obtain temperature and humidity it will subscribe to MQTT_TOPIC using MQTT_CLIENT_ID as ID.
 * By default the ID is the same as the hostname.
 * MQTT_BUF_SIZE controls the size of the buffers used to parse the MQTT response data.
 * Increase it if you encounter dropped characters, otherwhise leave it to the default value.
 */
#define MQTT_BROKER "iot.example.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC "path/to/topic"
#define MQTT_CLIENT_ID WIFI_HOSTNAME

/*
 * NTP and clock related settings.
 * 
 * The ESP8266 will connect to NTP_SERVER on boot and once every NTP_SYNC_INTERVAL minutes, asking for time.
 * NTP uses an epoch of January 1, 1900 referenced to the GMT.
 * OFFSET_FROM_GMT_H and OFFSET_FROM_GMT_M control the offset to be added to GMT to obtain the local time,
 * as hours and minutes respectively.
 */
#define NTP_SERVER "ntp.example.com"
#define NTP_SYNC_INTERVAL_M (4*60)    // 4 hours

#define OFFSET_FROM_GMT_H 0
#define OFFSET_FROM_GMT_M 0
// if defined correct the time information taking DST into account
#define ENABLE_DST

/*
 * Task-related configuration macros.
 */
// Update the screen contents once every this time (in milliseconds)
#define SCREEN_UPDATE_MS 1000

// Update the backlight state once every this ms
#define BACKLIGHT_UPDATE_MS 200

// Backlight duration ticks (the effective backlight duration is BACKLIGHT_DURATION_TICKS*BACKLIGHT_UPDATE_MS)
#define BACKLIGHT_DURATION_TICKS 10

// ADC reading value above which the backlight is turned on
#define BACKLIGHT_ON_THRESHOLD 845

// Turn off the backlight when the ADC reading is below THRESHOLD - DEADBAND
#define BACKLIGHT_DEADBAND 10

// Backlight brightness value when on (must be between 0 and 1023)
#define BACKLIGHT_BRIGHTNESS 256

// MQTT update rate is defined by as half MQTT_KEEPALIVE value to be on the safe side
#define MQTT_UPDATE_MS ((MQTT_KEEPALIVE / 2)*1000)
