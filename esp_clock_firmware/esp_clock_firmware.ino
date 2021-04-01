/*
    Digital clock with LCD display using a Wemos D1 / ESP8266

    Features:
     - Time is fetched using NTP
     - Also the weather is fetched from the network
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <EasyNTPClient.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

#include "clock_display_d2x2.h"
#include "d1_gpio_utils.h"
#include "config.h"

/**
 * Data structure that holds the tasks to run.
 * The task are repetitive and are run every rateMillis (more or less).
 */
struct task_t {
  time_t nextMillis; // next time in millis() the task should run
  time_t rateMillis; // the rate at which the task is run
  void (*taskFunc)(void);
};
struct task_t tasks[SCHED_NUM_TASKS] = {0};

#define sched_put_task(id, func, rate_ms, nextRun_ms) {\
  tasks[id].taskFunc = func; \
  tasks[id].rateMillis = rate_ms; \
  tasks[id].nextMillis = nextRun_ms; \
} 

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

  // initialize serial port
  Serial.begin(115200);
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

  Serial.print("\n[Wifi] RSSI = ");
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
  sched_put_task(0, &backlightTask, BACKLIGHT_UPDATE_MS, SCREEN_UPDATE_MS); // start deferred (update screen first)
  sched_put_task(1, &screenUpdateTask, SCREEN_UPDATE_MS, 0); // start immediatelly
  sched_put_task(2, &mqttLoopTask, MQTT_UPDATE_MS, 0);
  
  // done loading
  lcd.noBlink();
}

void loop() {
  time_t entry_time = millis();

  for(size_t i = 0; i < SCHED_NUM_TASKS; i++)
  {
    if ((entry_time >= tasks[i].nextMillis) && tasks[i].taskFunc)
    {
      (*tasks[i].taskFunc)();
      tasks[i].nextMillis = entry_time + tasks[i].rateMillis;
    }
  }
  
  time_t elapsed_time = millis() - entry_time;
 
  // sleep until the next full tick
  delay(SCHED_RATE_MS - (elapsed_time % SCHED_RATE_MS));
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
  /* compute daylight time offset: an hour should be added if date between:
  // last Sunday of March 01:00UTC to last Sunday of October 01:00UTC
  https://github.com/andydoro/DST_RTC
  https://github.com/nseidle/Daylight_Savings_Time_Example/blob/master/Daylight_Savings_Time_Example.ino
  */
  dst_offset = 1;
#endif 

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
    Serial.print("[MQTT] Attempting connection...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println(" Success.");
      // (re)subscribe to the required topics
      mqttClient.subscribe(MQTT_TOPIC);
    } else {
      Serial.print(" Fail: rc=");
      Serial.println(mqttClient.state());
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

  Serial.print("[NTP] Querying ");
  Serial.println(NTP_SERVER);

  curr_time = ntpClient.getUnixTime();

  // To check if the received time is valid, verify that
  // we should be at least in 2020, otherwhise retry in 10s
  if (curr_time <= 1577836800) {
    setSyncInterval(15);
    Serial.print("[NTP] Wrong time: ");
    Serial.print(curr_time);
    Serial.println(". Aborting");
    curr_time = now();  // the old time is better than a completely wrong one
  } else {
    setSyncInterval(NTP_SYNC_INTERVAL_M * 60);
    Serial.print("[NTP] Time synchronized: ");
    Serial.println(curr_time);
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
  
  Serial.print("[MQTT] Got: ");
  Serial.println(payload_str);

  auto error = deserializeJson(jsonDoc, payload_str);
  if (error) {
    Serial.printf("deserializeJson() failed with code %s\n", error.c_str());
    return;
  }

  temperature = jsonDoc["temp"];
  humidity = jsonDoc["rhum"];

  builtin_LED_off();
}
