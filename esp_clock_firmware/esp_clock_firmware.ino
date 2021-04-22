/*
    Digital clock with LCD display using a Wemos D1 / ESP8266

    Features:
     - Time is fetched using NTP
     - Also the weather is fetched from the network
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include "EasyNTPClient/src/EasyNTPClient.h"
#include "EasyNTPClient/src/EasyNTPClient.cpp"
#include <TimeLib.h>
#include <ArduinoJson.h>

#include "qd_sched.h"
#include "clock_display_d2x2.h"
#include "d1_gpio_utils.h"
#include "ash.h"
#include "alogger.h"
#include "config/config.h"

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

// EasyNTPClient instance to fetch the current NTP time
#define OFFSET_FROM_GMT_S ((OFFSET_FROM_GMT_H*60*60)+(OFFSET_FROM_GMT_M*60))
EasyNTPClient ntpClient(udp, NTP_SERVER, OFFSET_FROM_GMT_S);

// MQTT client to receive temperature updates
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// last temperature and humidity readings
float temperature, humidity;

void setup() {
  // Light sleep can be used only if the WiFi is in STA mode
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  
  // initialize serial port
  Serial.begin(115200);
  while(!Serial) { }
  Serial.println();

  lcd_init();
  // Inform the user that we are loading
  lcd.blink();

  gpios_init();

  // connect to the wifi network
  builtin_LED_on();

  Serial.print("[WiFi] Connecting to ");
  Serial.print(WIFI_SSID);

  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print(".");
  }

  Serial.print("\n[Wifi] Connected. RSSI = ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  builtin_LED_off();

  // setup timeLib to use NTP
  setSyncInterval(NTP_SYNC_INTERVAL_M * 60); // seconds see: https://github.com/PaulStoffregen/Time
  setSyncProvider(getNTPtime);
  
  // Initialize MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttSubCallback);

  // setup tasks
  sched_put_task(&backlightTask, BACKLIGHT_UPDATE_MS);
  sched_put_task(&screenUpdateTask, SCREEN_UPDATE_MS);
  sched_put_task(&mqttLoopTask, MQTT_UPDATE_MS);
  sched_put_task(&ashTask, 1000);
  
  // done loading
  lcd.noBlink();
}

/**
 * Function to control the backlight
 */
void backlightTask() {
  // Keep track of the backlight. When 0 the backlight is turned off.
  // Static to keep the state between invocations
  static byte backlightTimer = 0;

  if (backlightTimer == 0)
    set_LCD_backlight(false, 0);
  else
    backlightTimer--;
  
  if (digitalRead(BACKLIGHT_BUTTON) == 0)
  {
    set_LCD_backlight(true, 0); // set display to minimum brightness
    backlightTimer = BACKLIGHT_DURATION_TICKS;
  }
}

/**
 * Function to update the screen contents
 */
void screenUpdateTask() {
  int dst_offset = 0;

#ifdef ENABLE_DST
  /* compute daylight time offset. An hour should be added if date between:
   * last Sunday of March 01:00UTC to last Sunday of October 01:00UTC
   * 
   * This algorithm is based on LUT. Let's ignore leap years for a moment.
   * Since 365 % 7 = 1, given a reference date, the date for the following year goes back one day. 
   * For example, the last sunday of march is: Sun 28 March 2021 -> Sun 27 March 2022.
   * March and October have both 31 days. So, the last sunday can happen between the 25th and the 31st.
   * Therefore, the date for the last sunday of March/October repeats every 7 years.
   * 
   * Leap years screw everything up. Since I don't expect this clock and/or this code to survive more
   * than a century, I'm just assuming a leap year happens every 4 years (hi guys from the future,
   * sorry I've been very shortsighted. Hopefully thanks to this explaination you can recompute
   * the LUT and be safe for another 100 years or so). Taking this into account, the date repeats every
   * 7*4 = 28 years. As a consequence, my LUT has 28 entries and starts from March 2021.
   * 
   * For october, it is just a matter of setting the reference year in such a way that we can reuse the 
   * same LUT used for March. For this century we need a 20 year offset, so the reference year for October is 2001.
   */
  static const uint8_t dst_lut[] = {28,27,26,31,30,29,28,
                                    26,25,31,30,28,27,26,
                                    25,30,29,28,27,25,31,
                                    30,29,27,26,25,31,29};
  static const uint16_t mar_yr = 2021; // reference year for March (LUT start)
  static const uint16_t oct_yr = 2001; // reference year for October

  // get last sunday of March
  uint8_t mar_ls = dst_lut[(year() - mar_yr)%sizeof(dst_lut)];
  // and October
  uint8_t oct_ls = dst_lut[(year() - oct_yr)%sizeof(dst_lut)];

  if ((month() > 3) && (month() < 11))
    dst_offset = 1;
  else if ((month() == 3) && (day() >= mar_ls))
    dst_offset = 1;
  else if ((month() == 10) && (day() <= oct_ls))
    dst_offset = 1;
#endif 

  if (((millis() / 1000)%60) == 0) // log once every minute
    LOG_INFO("hour=%u dst_offset=%u minute=%u", hour(), dst_offset, minute());
  
  lcd_print_time(hour() + dst_offset, minute());
  lcd_print_temp_humidity(temperature, humidity);
}

/**
 * MQTT utility function to (re)connect to the broker if needed
 * and run the loop() function to receive data.
 */
void mqttLoopTask()
{
  if (!mqttClient.connected())
  {
    LOG_INFO("Attempting connection...");
    
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      LOG_INFO("Connection successful.");
      // (re)subscribe to the required topics
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      LOG_INFO("Connection failed: rc=%i.", mqttClient.state());
    }
  }

  mqttClient.loop();
}

/**
 * Get the curent time from the NTP server
 */
time_t getNTPtime()
{
  time_t curr_time;

  builtin_LED_on();

  LOG_INFO("Querying %s", NTP_SERVER);

  curr_time = ntpClient.getUnixTime();

  // To check if the received time is valid, verify that
  // we should be at least in 2020, otherwhise retry in 15s
  if (curr_time <= 1577836800) {
    setSyncInterval(15);
    LOG_INFO("Wrong time: %lu. Retry in 15s.", curr_time);
    curr_time = now();  // the old time is better than a completely wrong one
  } else {
    setSyncInterval(NTP_SYNC_INTERVAL_M * 60);
    LOG_INFO("Time synchronized: %lu", curr_time);
  }

  builtin_LED_off();
  return curr_time;
}

/**
 * React to MQTT subscribed topic updates
 */
void mqttSubCallback(char* topic, byte* payload, unsigned int len)
{
  char payload_str[MQTT_BUF_SIZE];
  StaticJsonDocument<MQTT_BUF_SIZE> jsonDoc;

  builtin_LED_on();

  if(len > MQTT_BUF_SIZE)
    len = MQTT_BUF_SIZE;
  
  snprintf(payload_str, len+1, "%s", payload);
  
  LOG_INFO("payload_str=%s.", payload_str);

  auto error = deserializeJson(jsonDoc, payload_str);
  if (error) {
    LOG_ERROR("deserializeJson() failed with code %s\n", error.c_str());
    return;
  }

  temperature = jsonDoc["temp"];
  humidity = jsonDoc["rhum"];

  builtin_LED_off();
}
