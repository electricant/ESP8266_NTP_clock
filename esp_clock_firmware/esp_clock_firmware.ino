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
#include <qd_sched.h>
#include <ArduinoJson.h>

#include "clock_display_d2x2.h"
#include "ash.h"
#include "alogger.h"
#include "config/config.h"

/*
 * Macros for hardware configuration.
 * 
 * See 'clock_display_d2x2.h' for macros defining where the display is connected
 */
// Input pin to enable backlight when pressed
#define BACKLIGHT_BUTTON D3
// Output pin to be used as LCD backlight control
#define BACKLIGHT_PIN D0

/* 
 * The controls for the builtin LED are reversed.
 * When the pin is set to HIGH the LED is off and vice-versa.
 * Those two macors make it easy to set the LED state appropriately.
 */
#define LED_BUILTIN_ON  0
#define LED_BUILTIN_OFF 1

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

// EasyNTPClient instance to fetch the current NTP time
#define OFFSET_FROM_UTC_S ((OFFSET_FROM_UTC_H*60*60)+(OFFSET_FROM_UTC_M*60))
EasyNTPClient ntpClient(udp, NTP_SERVER, OFFSET_FROM_UTC_S);

// MQTT client to receive temperature updates
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// last temperature and humidity readings
float temperature, humidity;

/**
 * Initialize the various GPIOs
 */ 
void gpios_init()
{
  // initialize onboard LED as output
  pinMode(LED_BUILTIN, OUTPUT);
  // set BACKLIGHT_BUTTON pin as input pullup to detect button press
  pinMode(BACKLIGHT_BUTTON, INPUT_PULLUP);
  // also set the backlight pin as output
  pinMode(BACKLIGHT_PIN, OUTPUT);
  // set PWM frequency to minimum to lower CPU load
  analogWriteFreq(100);
}

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
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
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
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

  // setup timeLib to use NTP
  setSyncInterval(NTP_SYNC_INTERVAL_M * 60); // seconds see: https://github.com/PaulStoffregen/Time
  setSyncProvider(getNTPtime);
  
  // Initialize MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttSubCallback);
  // set MQTT keepalive value to allow for a safety margin
  mqttClient.setKeepAlive(2*MQTT_UPDATE_MS/1000);

  // setup tasks
  sched_put_task(&backlightTask, BACKLIGHT_UPDATE_MS, true);
  sched_put_task(&screenUpdateTask, SCREEN_UPDATE_MS, true);
  sched_put_task(&mqttLoopTask, MQTT_UPDATE_MS, true);
  sched_put_task(&ashTask, 1000, false);
  
  // done loading
  lcd.noBlink();
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
}

/**
 * Function to control the backlight.
 * 
 * It uses a state machine that reads two inputs: the backlight buttona and the ambient light.
 * backlight_state_t describes the state the machine is.
 * Depending on the state the backlight can be either on or off.
 */
enum backlight_state_t {
  BKL_IDLE,   // backlight idle (off)
  BKL_MANUAL, // backlight turned on manually (button pressed)
  BKL_AUTO    // backlight turned on automatically (tiriggered by the ambient light sensor)
};
typedef enum backlight_state_t backlight_state_t;

void backlightTask() {
  // keep track of the current state (start in idle mode)
  static backlight_state_t state = BKL_IDLE;
  // used to keep the backlight on for some time after the button has been pressed
  static byte backlightTimer = 0;
  // current ambient light reading
  uint16_t ambientLight = analogRead(A0);

  switch (state) {
    case BKL_IDLE:
      digitalWrite(BACKLIGHT_PIN, 0);
      
      if (digitalRead(BACKLIGHT_BUTTON) == 0) {
        state = BKL_MANUAL;
        backlightTimer = BACKLIGHT_DURATION_TICKS;
        LOG_INFO("Moving to state %i. Reason: backlight button press.", state);
      } else if (ambientLight > BACKLIGHT_ON_THRESHOLD) {
        state = BKL_AUTO;
        LOG_INFO("Moving to state %i. Reason: ambient light.", state);
      }
      break;
    
    case BKL_MANUAL:
      analogWrite(BACKLIGHT_PIN, BACKLIGHT_BRIGHTNESS);

      if (backlightTimer == 0) {
        state = BKL_IDLE;
        LOG_INFO("Moving to state %i. Reason: backlight timer expired.", state);
      } else if (digitalRead(BACKLIGHT_BUTTON) == 0)
        backlightTimer = BACKLIGHT_DURATION_TICKS;
      else if (backlightTimer > 0)
        backlightTimer--;
      
      break;
    
    case BKL_AUTO:
      analogWrite(BACKLIGHT_PIN, BACKLIGHT_BRIGHTNESS);

      if (ambientLight < (BACKLIGHT_ON_THRESHOLD - BACKLIGHT_DEADBAND)) {
        state = BKL_IDLE;
        LOG_INFO("Moving to state %i. Reason: ambient light.", state);
      }
      break;
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
   * This algorithm is based on a LUT. Let's ignore leap years for a moment.
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
  uint8_t mar_last_sun = dst_lut[(year() - mar_yr)%sizeof(dst_lut)];
  // and October
  uint8_t oct_last_sun = dst_lut[(year() - oct_yr)%sizeof(dst_lut)];

  if ((month() > 3) && (month() < 10))
    dst_offset = 1;
  if ((month() == 3) && (day() >= mar_last_sun))
    dst_offset = 1;
  else if ((month() == 10) && (day() < oct_last_sun))
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

  return curr_time;
}

/**
 * React to MQTT subscribed topic updates
 */
void mqttSubCallback(char* topic, byte* payload, size_t payloadLen)
{
  // The input JSON has 4 objets and no arrays.
  // Moreover, since the input is char* no string copy is needed.
  // See: https://arduinojson.org/v6/assistant/
  //      https://arduinojson.org/v6/how-to/determine-the-capacity-of-the-jsondocument/
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> jsonDoc;

  auto error = deserializeJson(jsonDoc, (char*)payload, payloadLen);
  if (error) {
    LOG_ERROR("deserializeJson() failed with code %s\n", error.c_str());
    return;
  }

  temperature = jsonDoc["temp"];
  humidity = jsonDoc["rhum"];
  LOG_INFO("%s -> temp: %.1f rhum: %.1f", topic, temperature, humidity);
}
