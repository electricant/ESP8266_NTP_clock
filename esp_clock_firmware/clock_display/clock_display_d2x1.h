/*
 * Header file for the functions controlling the LCD display of the clock.
 *
 * It uses a 1x2 font for displaying the digits.
 *
 * The fonts are defined by a set of 8 custom chars which, combined, look like
 * digits. The custom chars are loaded when the display is initialized and are
 * defined by the chrN (0<= N < 8) arrays.
 * The way in which the custom chars are combined to form digits is defined in
 * the 'number_chars' array. This array has an entry for each digit. Within this
 * entry the index of the custom char from top to bottom is present.	
 */
#ifndef CLOCK_DISPLAY_D2X1_H
#define CLOCK_DISPLAY_D2X1_H

#include <LiquidCrystal.h>
#include <pins_arduino.h>

/*
 * Use the 'sunstart' font for displaying digits.
 * 
 * It has bin inspired by the 'big digits' in the sunstart LCD module.
 * The digits have a 7-segment-y look.
 */
#define DIGITS_FONT_SUNSTART

/*
 * Use the 'supriokundu' font for displaying digits.
 * 
 * It is taken from user 'supriokundu' on avrfreaks.net:
 * https://www.avrfreaks.net/forum/alphanumeric-lcd-hd44780-big-font-big-digits-generator-excel-sheet
 * It has a more square-ish look.
 */
//#define DIGITS_FONT_SUPRIOKUNDU

// ensure that one and only one font has been selected
#if !defined(DIGITS_FONT_SUPRIOKUNDU) && !defined(DIGITS_FONT_SUNSTART)
  #error "Please choose one font for displaying the digits"
#endif
#if defined(DIGITS_FONT_SUPRIOKUNDU) && defined(DIGITS_FONT_SUNSTART)
  #error "Please choose ONLY one font for displaying the digits"
#endif

#ifdef DIGITS_FONT_SUNSTART
byte chr0[] = {B01110, B01110, B11011, B11011, B11011, B11011, B11011, B11011};
byte chr1[] = {B00011, B00011, B00011, B00011, B00011, B00011, B00011, B00011};
byte chr2[] = {B11110, B11110, B00011, B00011, B00011, B00011, B00011, B00011};
byte chr3[] = {B01110, B01110, B11000, B11000, B11000, B11000, B01111, B01111};
byte chr4[] = {B11011, B11011, B11011, B11011, B11011, B11011, B01110, B01110};
byte chr5[] = {B01110, B01110, B00011, B00011, B00011, B00011, B11110, B11110};
byte chr6[] = {B01111, B01111, B11000, B11000, B11000, B11000, B11000, B11000};
byte chr7[] = {B01110, B01110, B11011, B11011, B11011, B11011, B01110, B01110};

byte number_chars[10][2] = {
  {0, 4}, // 0
  {1, 1}, // 1
  {2, 3}, // 2
  {2, 5}, // 3
  {4, 1}, // 4
  {6, 5}, // 5
  {6, 7}, // 6
  {2, 1}, // 7
  {0, 7}, // 8
  {0, 5}  // 9 
};
#endif

#ifdef DIGITS_FONT_SUPRIOKUNDU
byte chr0[] = {B11111, B11111, B11011, B11011, B11011, B11011, B11011, B11011};
byte chr1[] = {B11011, B11011, B11011, B11011, B11011, B11011, B11111, B11111};
byte chr2[] = {B00011, B00011, B00011, B00011, B00011, B00011, B00011, B00011};
byte chr3[] = {B11111, B00011, B00011, B00011, B00011, B00011, B00011, B11111};
byte chr4[] = {B11111, B11000, B11000, B11000, B11000, B11000, B11000, B11111};
byte chr5[] = {B00011, B00011, B00011, B00011, B00011, B00011, B00011, B11111};
byte chr6[] = {B11111, B11011, B11011, B11011, B11011, B11011, B11011, B11111};
byte chr7[] = {B11111, B00011, B00011, B00011, B00011, B00011, B00011, B00011};

byte number_chars[10][2] = {
	{0, 1}, // 0
	{2, 2}, // 1
	{3, 4}, // 2
	{3, 3}, // 3
	{1, 2}, // 4
	{4, 5}, // 5
	{4, 6}, // 6
	{7, 2}, // 7
	{6, 6}, // 8
	{6, 2}  // 9 
};
#endif

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
 * Prints the given date on the LCD
 */
void lcd_print_date(uint8_t year_day, uint8_t mon);

/*
 * Prints temperature and humidity
 */
void lcd_print_temp_humidity(float temp, float hum);

#endif
