#include "d1_gpio_utils.h"

void gpios_init()
{
  // initialize onboard LED as output
  pinMode(BUILTIN_LED, OUTPUT);
  // set BACKLIGHT_BUTTON pin as input pullup to detect button press
  pinMode(BACKLIGHT_BUTTON, INPUT_PULLUP);
  // also set the backlight pin as output
  pinMode(BACKLIGHT_PIN, OUTPUT);
  // set PWM frequency to minimum to lower CPU load
  analogWriteFreq(100);
}

void builtin_LED_set_brightness(uint8_t percent)
{
  int pwm_val = 1023 - (1023*percent)/100;
  analogWrite(BUILTIN_LED, pwm_val);
}

inline void builtin_LED_on()
{
  digitalWrite(BUILTIN_LED, 0);
}

inline void builtin_LED_off()
{
  digitalWrite(BUILTIN_LED, 1);
}

inline void set_LCD_backlight(bool state, int brightness)
{
  if (state == false) {
    digitalWrite(BACKLIGHT_PIN, 0); // this turns PWM off
  } else {
    brightness = min(brightness, MAX_BACKLIGHT_BRIGHTNESS);
    brightness = max(brightness, MIN_BACKLIGHT_BRIGHTNESS);
    analogWrite(BACKLIGHT_PIN, brightness);
  }
}
