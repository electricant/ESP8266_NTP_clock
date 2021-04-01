/*
 * 
 */
#include "clock_display_d2x1.h"

// Month names
const String months[12]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
// Day names
const String days[7]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void lcd_init() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // save custom chars to the lcd
  lcd.createChar(0, chr0);
  lcd.createChar(1, chr1);
  lcd.createChar(2, chr2);
  lcd.createChar(3, chr3);
  lcd.createChar(4, chr4);
  lcd.createChar(5, chr5);
  lcd.createChar(6, chr6);
  lcd.createChar(7, chr7);
  
  lcd.home();  // Go to the home row/col
}

// utility function to pad numbers < 10 with custom padding (default 0)
String prettyDigits(int number, char padding = '0'){
  String digits_str = "";
  
  if(number < 10)
    digits_str += padding;
  
  digits_str += String(number);

  return digits_str;
}

// utility function for printing big digits on the screen
void print_big_digit(uint8_t digit, uint8_t col, uint8_t row) {
  lcd.setCursor(col, row);
  
  if (digit > 9) {
    lcd.print('?');
    return;
  }
  
  lcd.write(number_chars[digit][0]);
  lcd.setCursor(col, row+1);
  lcd.write(number_chars[digit][1]);
}

// utility function to print big two-digit numbers
void print_big_two_digit_num(uint8_t num, uint8_t col, uint8_t row) {
  uint8_t first_digit = num / 10;
  uint8_t second_digit = num % 10;

  print_big_digit(first_digit, col, row);
  print_big_digit(second_digit, col+1, row);
}

void lcd_print_time(uint8_t hrs, uint8_t mins) {
  print_big_two_digit_num(hrs, 0, 0);

  // blinky cursor
  if((second() % 2) == 0) {
    lcd.setCursor(2, 0);
    lcd.write(0b10100101); // center dot
    lcd.setCursor(2, 1);
    lcd.write(0b10100101);
  } else {
    lcd.setCursor(2, 0);
    lcd.write(' ');
    lcd.setCursor(2, 1);
    lcd.write(' '); 
  }
  
  print_big_two_digit_num(mins, 3, 0);
}

void lcd_print_time(uint8_t hrs, uint8_t mins, uint8_t sec) {
  lcd.setCursor(1, 1);
  // digital clock display of the time
  lcd.print(hrs);

  // blinky cursor
  if((second() % 2) == 0)
    lcd.print(":");
  else
    lcd.print(" ");
 
  lcd.print(prettyDigits(mins));
  lcd.print(":");
  lcd.print(prettyDigits(second()));
}

void lcd_print_date(uint8_t week_day, uint8_t month_day, uint8_t mon) {
  lcd.setCursor(6, 1);
  lcd.print(days[week_day - 1]);
  lcd.write(' ');
  lcd.print(month_day);
  lcd.write(' ');
  lcd.print(months[mon - 1]);
}

void lcd_print_temp_humidity(float temp, float hum){
    temp = round(temp);
    hum = round(hum);
    
    lcd.setCursor(7, 0);
    // Put some padding first one for the minus sign the other for the first digit
    if (temp >= 0)
      lcd.write(' ');
    if (temp < 10)
      lcd.write(' ');
    
    lcd.print((int) temp);
    lcd.write(0b11011111); // degree symbol. See: https://mil.ufl.edu/3744/docs/lcdmanual/characterset.html
    lcd.print("C/");
    
    lcd.print((int) hum);
    lcd.print("%");
}
