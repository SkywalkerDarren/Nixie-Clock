/* ------------------------------------------------------------------- 
  Nixie Tube Clock project: - 6 IN-12 Nixie tubes
                             - 6 74141 Driver chips
                             - 3 74HC595 Shift Registers
                             - 1 Arduino on the board
  
  Released into the public domain under the open-source GNU license.
  https://www.gnu.org/copyleft/gpl.html
  Summary of the above statement in real words that non-lawyers can
  understand: Take my code, project, ideas, design files etc... and
  make them even more awesome! Do whatever the hell you like with
  them! 
 
  Blog about this project:
 
  -------------------------------------------------------------------- */
  
#include <SoftI2C.h>       //Include all the relevent libraries
#include <DS3232RTC.h>
#include <avr/pgmspace.h>
#include <string.h>
#define data 2    //Define the shift register interface pins
#define clock 3
#define latch 4
#define bottom 5
#define middle 6
#define top 7

SoftI2C i2c(A4, A5);  //Initialize I2C line 
DS3232RTC rtc(i2c);   //Define rtc as an object for the DS3232 (with communication over I2C)

byte fNumbers[11] = { //Binary for the numbers, when given to the first shift register in each pair - 11th number is a blank
  B00000000,
  B00010000,
  B00100000,
  B00110000,
  B01000000,
  B01010000,
  B01100000,
  B01110000,
  B10000000,
  B10010000,
  B11110000 };
  
byte sNumbers[11] = { //Binary for the numbers, when given to the latter shift register in each pair - 11th number is a blank
  B00000000,
  B00000001,
  B00000010,
  B00000011,
  B00000100,
  B00000101,
  B00000110,
  B00000111,
  B00001000,
  B00001001,
  B00001111 };

byte dNumbers[3] = { //Numbers to display - temporary storage for whatever numbers we are currently displaying, one byte per shift register
  B00000000,
  B00000000,
  B00000000 };

void setup() 
{
  pinMode(data, 1);  //Set up the data, clock, and latch pins as outputs
  pinMode(clock, 1);
  pinMode(latch, 1);
  pinMode(top, 0);   //Set up the buttons as inputs
  pinMode(middle, 0);
  pinMode(bottom, 0);
  digitalWrite(top, 1);  //Enable the internal pull-up resistors for the buttons (also inverts the logic)
  digitalWrite(middle, 1);
  digitalWrite(bottom, 1);
}

void loop()
{
  RTCTime time;                 //Create object time - which will hold the data from the DS3232
  rtc.readTime(&time);          //Read the time from the DS3232 into the object 'time'
  int a = (time.hour / 10),     //Set the first segment to the tens unit of the hour variable eg. for 21 it would be 21/10 = 2
      b = (time.hour % 10),     //Set the second segment to the remainder when dividing the hour by 10 eg. for 21 it would be 21/10 = 2 remainder 1, so it would be 1
      c = (time.minute / 10),   //Above but for minutes
      d = (time.minute % 10),
      e = (time.second / 10),   //Above but for seconds
      f = (time.second % 10);   
  displayNumber(a, b, c, d, e, f);  //Display the numbers (custom subroutine)
  delay(100);                     //Delay 1/10th of a second
  if(digitalRead(middle) == LOW) //If the middle button is pressed
  {
    while(digitalRead(middle) == LOW); //Wait for it to not be pressed
    delay(100); //Delay to debounce
    setTime(); //Set the time (custom subroutine)
  }
}

void displayNumber(int a, int b, int c, int d, int e, int f) //Used to display the numbers
{
  dNumbers[0] = (sNumbers[a] | fNumbers[b]); //Creates on 8 bit binary number of the first two digits - for the first shift register
  dNumbers[1] = (sNumbers[c] | fNumbers[d]); //Above - but with the third and fourth digits - for the second shift register
  dNumbers[2] = (sNumbers[e] | fNumbers[f]); //Above - but with the fifth and sixth digits - for the third shift register
  digitalWrite(latch, 0);                           //Hold the latch low while we're shifting out data
  for(int i = 3; i >= 0; i--)                       //Shift everything out in reverse, because that's how they're daisychained/hooked up to the nixie drivers
  {
    shiftOut(data, clock, MSBFIRST, dNumbers[i]);
  }
  digitalWrite(latch, 1); //Pull the latch high again
}

void setTime() //used to set the time
{  
  int timeToSet[3] = {0, 0, 0}; //array we will use as temporary storage
  for(int j = 0; j < 3; j++)    //for loop, runs 3 times, with j starting at 0 and ending at 2 - used to index the array
  {
    timeToSet[j] = 0; //Set the element we are changing to 0
    while(digitalRead(middle) == HIGH) //Do this until the middle button is pressed
    {
      if(digitalRead(top) == LOW) //If the top button is pressed
      {
        while(digitalRead(top) == LOW); //Wait for it to stop being pressed
        delay(100); //Delay to debounce
        timeToSet[j]++; //Increment the value in the element we are currently setting
        if ((j == 0) && (timeToSet[j] > 23)) timeToSet[j] = 0; //If we are altering the hours, and it is greater than 23, set it to 0
        if (timeToSet[j] > 59) timeToSet[j] = 0; //If we are not altering the hours, and it is greater than 59, set it to 0
      }
      if(digitalRead(bottom) == LOW) //If the bottom button is pressed
      {
        while(digitalRead(bottom) == LOW); //Wait for it to stop being pressed
        delay(100); //Delay to debounce
        timeToSet[j]--; //Decrement the value in the element we are currently setting
        if ((j == 0) && (timeToSet[j] < 0)) timeToSet[j] = 23; //If we are altering the hours, and it is less than 0, set it to 23
        if (timeToSet[j] < 0) timeToSet[j] = 59; //If we are not altering the hours, and it is less than 0, set it to 59
      }
      switch (j) { //Case statement
        case 0: //If j = 0
          displayNumber(timeToSet[0] / 10, timeToSet[0] % 10, 10, 10, 10, 10); //Display the hours we are setting - blank the rest
          break;
        case 1: //If j = 1
          displayNumber(timeToSet[0] / 10, timeToSet[0] % 10, timeToSet[1] / 10, timeToSet[1] % 10, 10, 10); //Display the hours we have set, the minutes we are setting - blank the seconds
          break;
        case 2: //If j = 2
          displayNumber(timeToSet[0] / 10, timeToSet[0] % 10, timeToSet[1] / 10, timeToSet[1] % 10, timeToSet[j] / 10, timeToSet[j] % 10); //Display everything
          break;
        default: //If j is something else (should never happen)
          break; //Exit and cry.
      }
    }
    while(digitalRead(middle) == LOW); //Wait for middle button (which must have been pressed to get here) to not be pressed
    delay(100); //Delay to debounce
  }
  RTCTime time; //Set the time to the settings we configured
  time.hour = timeToSet[0];
  time.minute = timeToSet[1];
  time.second = timeToSet[2];
  rtc.writeTime(&time);
}
