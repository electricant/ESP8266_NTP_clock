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
#define MQTT_BUF_SIZE 50

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
 * Scheduler and task-related configuration macros.
 * The scheduler is very basic: it decides which task to run every SCHED_RATE_MS.
 * It keeps track of the next run time of each task and if current millis() is greater than the task run time,
 * then the selected task is run. If no task has to be run the it sleeps until the next iteration.
 * So basically each task function is executed in a periodic fashion.
 * 
 * SCHED_NUM_TASKS controls the maximum number of tasks that can be allocated. In this case it should be leaved to the default,
 * since only three tasks are currently running.
 */
#define SCHED_RATE_MS 100
#define SCHED_NUM_TASKS 3

// Update the screen contents once every this time (in milliseconds)
#define SCREEN_UPDATE_MS 1000

// Update the backlight state once every this ms
#define BACKLIGHT_UPDATE_MS 200

// Backlight duration ticks (the effective backlight duration is BACKLIGHT_DURATION_TICKS*BACKLIGHT_UPDATE_MS)
#define BACKLIGHT_DURATION_TICKS 10

// MQTT update rate is defined by as half MQTT_KEEPALIVE value to be on the safe side
#define MQTT_UPDATE_MS ((MQTT_KEEPALIVE / 2)*1000)
