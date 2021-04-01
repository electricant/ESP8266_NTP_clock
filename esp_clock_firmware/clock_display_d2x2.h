/*
 * Header file for the functions controlling the LCD display of the clock.
 *
 * It uses a 2x2 font for displaying the digits.
 *
 * The fonts are defined by a set of 8 custom chars which, combined, look like
 * digits. The custom chars are loaded when the display is initialized and are
 * defined by the chrN (0<= N < 8) arrays.
 * The way in which the custom chars are combined to form digits is defined in
 * the 'number_chars' array. This array has an entry for each digit. Within this
 * entry the index of the custom chars from left to right, top to bottom is present.	
 */
#ifndef CLOCK_DISPLAY_D2X2_H
#define CLOCK_DISPLAY_D2X2_H

#include <LiquidCrystal.h>
#include <pins_arduino.h>

/*
 * Custom chars
 */
byte chr0[] = {
  B01111,
  B11111,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000
};
byte chr1[] = {
  B11110,
  B11111,
  B00011,
  B00011,
  B00011,
  B00011,
  B00011,
  B00011
};
byte chr2[] = {
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11111,
  B01111
};
byte chr3[] = {
  B00011,
  B00011,
  B00011,
  B00011,
  B00011,
  B00011,
  B11111,
  B11110
};
byte chr4[] = {
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
byte chr5[] = {
  B01111,
  B11111,
  B11000,
  B11000,
  B11000,
  B11000,
  B11111,
  B01111
};
byte chr6[] = {
  B11110,
  B11111,
  B00011,
  B00011,
  B00011,
  B00011,
  B11111,
  B11110
};
byte chr7[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};

byte number_chars[10][4] = {
  {0,  1,  2, 3}, // 0
  {1, 32,  3, 7}, // 1
  {4,  6,  5, 7}, // 2
  {4,  6,  7, 3}, // 3
  {2,  3, 32, 1}, // 4
  {5,  4,  7, 6}, // 5
  {5,  4,  2, 6}, // 6
  {4,  1, 32, 0}, // 7
  {5,  6,  2, 3}, // 8
  {5,  6,  7, 3}  // 9 
};

// Global LCD instance definition
const int rs = D2, en = D1, d4 = D8, d5 = D7, d6 = D6, d7 = D5;
LiquidCrystal lcd = LiquidCrystal(rs, en, d4, d5, d6, d7);

/*
 * Initialize the LCD display
 */
void lcd_init();

/*
 * Prints the given time on the LCD
 */
void lcd_print_time(uint8_t hrs, uint8_t mins);
void lcd_print_time(uint8_t hrs, uint8_t mins, uint8_t sec);

/*
 * Prints temperature and humidity
 */
void lcd_print_temp_humidity(float temp, float hum);

#endif
