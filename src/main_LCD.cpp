#include <wiringPi.h>
#include <lcd.h>
#include <unistd.h>
#include "components/LCD.h"

//USE WIRINGPI PIN NUMBERS
#define LCD_RS  25               //Register select pin
#define LCD_E   24               //Enable Pin
#define LCD_D4  23               //Data pin 4
#define LCD_D5  22               //Data pin 5
#define LCD_D6  21               //Data pin 6
#define LCD_D7  14               //Data pin 7

int main()
{
    LCD* lc = new LCD();
     for(int i=0;i<250;i++){
         delay(10);
         lc->print_score(i);
     }

}