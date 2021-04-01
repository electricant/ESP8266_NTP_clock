/*
 * header file for various helper function controlling the GPIOs for the wemos D1
 */
#ifndef D1_GPIO_UTILS_H
#define D1_GPIO_UTILS_H

#include <pins_arduino.h>

// Input pin to enable backlight when pressed
#define BACKLIGHT_BUTTON D3
// Output pin to be used as LCD backlight control
#define BACKLIGHT_PIN D0
// Minimum backlight brightness value (must be betwwen 0 and 1023)
#define MIN_BACKLIGHT_BRIGHTNESS 256
// Maximum backlight brightness value (must be between 0 and 1023)
#define MAX_BACKLIGHT_BRIGHTNESS 1023

/*
 * Initialize the GPIO pins
 */
void gpios_init();

/* 
 * The controls for the builtin LED are reversed.
 * When the pin is set to HIGH the LED is off and vice-versa.
 * This subroutine makes it easy to set the brightness appropriately.
 * Always inline it for performance
 */
inline void builtin_LED_set_brightness(uint8_t percent) __attribute__((always_inline));

/* 
 * The controls for the builtin LED are reversed.
 * When the pin is set to HIGH the LED is off and vice-versa.
 * Those two subroutines make it easy to set the LED state appropriately.
 * Always inline it for performance
 */
inline void builtin_LED_on() __attribute__((always_inline));

inline void builtin_LED_off() __attribute__((always_inline));

/*
 * Control the LCD backlight.
 * 
 *  state      -> sets the backlight on or off
 *  brightness -> sets the brightness value. Must be between MIN and 1023
 */
inline void set_LCD_backlight(bool state, int brightness);

#endif
