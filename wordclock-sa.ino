
#include <Wire.h>

#include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <FastLED.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <fastpin.h>
#include <fastspi.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>

#define DATA_PIN 7
#define CLOCK_PIN 8

#define NUM_LEDS 144
#define COLOR_ORDER BGR
#define LED_TYPE APA102

//create an array of leds
CRGBArray<NUM_LEDS>leds;

uint8_t max_bright = 255;
uint8_t hue = 0;

//button setups
const byte btnHour = 10;
const byte btnMin = 11;
const byte btnCol = 12;
int valHour = 0;
int valMin = 0;
int valCol = 0;
byte oldHourSwitchState = HIGH;
byte oldMinSwitchState = HIGH;
byte oldColSwitchState = HIGH;
const unsigned long debounceTime = 100;
const unsigned long colTime = 500; 
unsigned long switchHourPressTime;
unsigned long switchMinPressTime;
unsigned long switchColPressTime;





// =========
// RTC SETUP
// =========

// I2C bus address
#define DS1307_I2C_ADDRESS 0x68

// Static variables
// Declare time variables
byte second, minute, hour, weekDay, monthDay, month, year;

// Function to convert normal decimal number to numbers 0-9 (BCD)
byte decBcd(byte val) {
  return ( (val/10*16) + (val%10) ); }

// Function to convert numbers 0-9 (BCD) to normal decimal number
byte bcdDec(byte val) {
  return ( (val/16*10) + (val%16) ); }

// Function to set the system clock
void configureTime() {
   // 1) Set the date and time values
   second       = 00; 
   minute       = 24; 
   hour         = 23;
   weekDay      = 3;
   monthDay     = 30; // Ali 21/3 (1)  Sandy 4/10 (2)
   month        = 11; 
   year         = 16;

   // 2) Commands to start up the clock
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   Wire.write(0x00);
   Wire.write(decBcd(second));
   Wire.write(decBcd(minute));
   Wire.write(decBcd(hour));
   Wire.write(decBcd(weekDay));
   Wire.write(decBcd(monthDay));
   Wire.write(decBcd(month));
   Wire.write(decBcd(year));
   Wire.endTransmission(); 
}

// Function to call up the system time
void requestTime()
{
  // Reset the pointer to the register
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();

  // Call up the time and date
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  second        = bcdDec(Wire.read() & 0x7f);
  minute        = bcdDec(Wire.read());
  hour          = bcdDec(Wire.read() & 0x3f);
  weekDay       = bcdDec(Wire.read());
  monthDay      = bcdDec(Wire.read());
  month         = bcdDec(Wire.read());
  year          = bcdDec(Wire.read());

  // Print the date and time via serial monitor
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  Serial.print("  ");
  Serial.print(monthDay, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print("  ");
}

void configureHour() {
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  hour = bcdDec(Wire.read() & 0x3f);
  hour = ((hour + 1) % 24);
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(decBcd(hour));
  Wire.endTransmission();
}





// =====================
// DEFINE THE LED GROUPS
// =====================

//define the minutes
int MFIVE[4] = {43, 44, 45, 46};
int MTEN[3] = {1, 2, 3};
int MQUARTER[7] = {25, 26, 27, 28, 29, 30, 31};
int MTWENTY[6] = {36, 37, 38, 39, 40, 41};
int MTWENTYFIVE[10] = {36, 37, 38, 39, 40, 41, 43, 44, 45, 46};
int MHALF[4] = {19, 20, 21, 22};

//define the hours
int HONE[3] = {84, 85, 86};
int HTWO[3] = {87, 88, 89};
int HTHREE[5] = {67, 68, 69, 70, 71};
int HFOUR[4] = {115, 116, 117, 118};
int HFIVE[4] = {127, 128, 129, 130};
int HSIX[3] = {100, 101, 102};
int HSEVEN[5] = {121, 122, 123, 124, 125};
int HEIGHT[5] = {103, 104, 105, 106, 107};
int HNINE[4] = {132, 133, 134, 135};
int HTEN[3] = {97, 98, 99};
int HELEVEN[6] = {90, 91, 92, 93, 94, 95};
int HTWELVE[6] = {109, 110, 111, 112, 113, 114};

//define the other things
int ITIS[4] = {6, 7, 9, 10};
int PAST[4] = {62, 63, 64, 65};
int TO[2] = {60, 61};
//int HBA[19] = {13, 14, 15, 16, 17, 50, 51, 52, 53, 54, 55, 56, 57, 78, 79, 80, 81, 82, 83};
//int HBS[18] = {13, 14, 15, 16, 17, 50, 51, 52, 53, 54, 55, 56, 57, 72, 73, 74, 75, 76};
int OCLOCK[6] = {137, 139, 140, 141, 142, 143};

// int LEDSWHITE = {CRGB::White};
// int LEDSRED = {CRGB::Red};
// int LEDSCOLOUR = {CHSV(timehue,255,255)};





// ===========
// SETUP PHASE
// ===========
void setup() {

  // Initialize the I2C
  Wire.begin();
  
  delay(1000);
  Serial.begin(9600);
  LEDS.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  // FastLED.setMaxPowerInVoltsAndMilliamps(5, 100);
  FastLED.setBrightness(max_bright);

pinMode(btnHour, INPUT);
pinMode(btnMin, INPUT);
pinMode(btnCol, INPUT);





// SET THE TIME. Comment out and reupload once the time is set.
//configureTime(); 
}



void lightsOff() {
    for (int dot = 0; dot < NUM_LEDS; dot++) { 
        leds[dot] = CRGB::Black;    
    }
}

void lightsOn(int word[], int len) {
    int x;
    for (int dot = 0; dot < len; dot++){
      x = word[dot];
      leds[x] = CRGB::White;
    }
}





// ==========
// LOOP PHASE
// ==========

void loop() {
  
//Request the time from the RTC
requestTime();

//Send time to the monitor
Serial.println(" ");

//Kill unnecessary lights
lightsOff();
  
//Keep "IT IS" lit  
lightsOn(ITIS, 4);

//LIGHT UP THE MINUTES BABY
//On the hour
if (minute < 5) {  lightsOn(OCLOCK, 6); }
//Five past
if ((minute > 4) && (minute < 10)) {  lightsOn(MFIVE, 4);  lightsOn(PAST, 4);}
//Ten past
if ((minute > 9) && (minute < 15)) {  lightsOn(MTEN, 3);  lightsOn(PAST, 4);}
//Quarter past
if ((minute > 14) && (minute < 20)) {  lightsOn(MQUARTER, 7);  lightsOn(PAST, 4);}
//Twenty past
if ((minute > 19) && (minute < 25)) {  lightsOn(MTWENTY, 6);  lightsOn(PAST, 4);}
//Twenty-five past
if ((minute > 24) && (minute < 30)) {  lightsOn(MTWENTYFIVE, 10);  lightsOn(PAST, 4);}
//Half past
if ((minute > 29) && (minute < 35)) {  lightsOn(MHALF, 4);  lightsOn(PAST, 4);}
//Twenty-five to
if ((minute > 34) && (minute < 40)) {  lightsOn(MTWENTYFIVE, 10);  lightsOn(TO, 2);}
//Twenty to
if ((minute > 39) && (minute < 45)) {  lightsOn(MTWENTY, 6);  lightsOn(TO, 2);}
//Quarter to
if ((minute > 44) && (minute < 50)) {  lightsOn(MQUARTER, 7);  lightsOn(TO, 2);}
//Ten to
if ((minute > 49) && (minute < 55)) {  lightsOn(MTEN, 3);  lightsOn(TO, 2);}
//Five to
if ((minute > 54) && (minute < 60)) {  lightsOn(MFIVE, 4);  lightsOn(TO, 2);}
//if ((minute > 54) && (minute < 60)) { leds(0,4);}

//LIGHT UP THE HOURS BABY
if ((hour == 1) && (minute <= 34)) {  lightsOn(HONE, 3);}
if ((hour == 1) && (minute >= 35)) {  lightsOn(HTWO, 3);}
if ((hour == 13) && (minute <= 34)) {  lightsOn(HONE, 3);}
if ((hour == 13) && (minute >= 35)) {  lightsOn(HTWO, 3);}

if ((hour == 2) && (minute <= 34)) {  lightsOn(HTWO, 3);}
if ((hour == 2) && (minute >= 35)) {  lightsOn(HTHREE, 5);}
if ((hour == 14) && (minute <= 34)) {  lightsOn(HTWO, 3);}
if ((hour == 14) && (minute >= 35)) {  lightsOn(HTHREE, 5);}

if ((hour == 3) && (minute <= 34)) {  lightsOn(HTHREE, 5);}
if ((hour == 3) && (minute >= 35)) {  lightsOn(HFOUR, 4);}
if ((hour == 15) && (minute <= 34)) {  lightsOn(HTHREE, 5);}
if ((hour == 15) && (minute >= 35)) {  lightsOn(HFOUR, 4);}

if ((hour == 4)  && (minute <= 34)) {  lightsOn(HFOUR, 4);}
if ((hour == 4) && (minute >= 35)) {  lightsOn(HFIVE, 4);}
if ((hour == 16) && (minute <= 34)) {  lightsOn(HFOUR, 4);}
if ((hour == 16) && (minute >= 35)) {  lightsOn(HFIVE, 4);}

if ((hour == 5) && (minute <= 34)) {  lightsOn(HFIVE, 4);}
if ((hour == 5) && (minute >= 35)) {  lightsOn(HSIX, 3);}
if ((hour == 17) && (minute <= 34)) {  lightsOn(HFIVE, 4);}
if ((hour == 17) && (minute >= 35)) {  lightsOn(HSIX, 3);}

if ((hour == 6) && (minute <= 34)) {  lightsOn(HSIX, 3);}
if ((hour == 6) && (minute >= 35)) {  lightsOn(HSEVEN, 5);}
if ((hour == 18) && (minute <= 34)) {  lightsOn(HSIX, 3);}
if ((hour == 18) && (minute >= 35)) {  lightsOn(HSEVEN, 5);}

if ((hour == 7) && (minute <= 34)) {  lightsOn(HSEVEN, 5);}
if ((hour == 7) && (minute >= 35)) {  lightsOn(HEIGHT, 5);}
if ((hour == 19) && (minute <= 34)) {  lightsOn(HSEVEN, 5);}
if ((hour == 19) && (minute >= 35)) {  lightsOn(HEIGHT, 5);}

if ((hour == 8) && (minute <= 34)) {  lightsOn(HEIGHT, 5);}
if ((hour == 8) && (minute >= 35)) {  lightsOn(HNINE, 4);}
if ((hour == 20) && (minute <= 34)) {  lightsOn(HEIGHT, 5);}
if ((hour == 20) && (minute >= 35)) {  lightsOn(HNINE, 4);}

if ((hour == 9) && (minute <= 34)) {  lightsOn(HNINE, 4);}
if ((hour == 9) && (minute >= 35)) {  lightsOn(HTEN, 3);}
if ((hour == 21) && (minute <= 34)) {  lightsOn(HNINE, 4);}
if ((hour == 21) && (minute >= 35)) {  lightsOn(HTEN, 3);}

if ((hour == 10) && (minute <= 34)) {  lightsOn(HTEN, 3);}
if ((hour == 10) && (minute >= 35)) {  lightsOn(HELEVEN, 6);}
if ((hour == 22) && (minute <= 34)) {  lightsOn(HTEN, 3);}
if ((hour == 22) && (minute >= 35)) {  lightsOn(HELEVEN, 6);}

if ((hour == 11) && (minute <= 34)) {  lightsOn(HELEVEN, 6);}
if ((hour == 11) && (minute >= 35)) {  lightsOn(HTWELVE, 6);}
if ((hour == 23) && (minute <= 34)) {  lightsOn(HELEVEN, 6);}
if ((hour == 23) && (minute >= 35)) {  lightsOn(HTWELVE, 6);}

if ((hour == 12) && (minute <= 34)) {  lightsOn(HTWELVE, 6);}
if ((hour == 12) && (minute >= 35)) {  lightsOn(HONE, 3);}
if ((hour == 0) && (minute <= 34)) {  lightsOn(HTWELVE, 6);}
if ((hour == 0) && (minute >= 35)) {  lightsOn(HONE, 3);}

// BIRTHDAYS BABY
// Sandy birthday
if (monthDay == 4) {  if (month == 10) { static uint8_t hue=0; leds(72,76).fill_rainbow(hue++); leds(13,17).fill_rainbow(hue++); leds(50,57).fill_rainbow(hue++);}}

// Ali birthday
if (monthDay == 21) {  if (month == 3) { static uint8_t hue=0; leds(78,83).fill_rainbow(hue++); leds(13,17).fill_rainbow(hue++); leds(50,57).fill_rainbow(hue++);}}

//LIGHTS ON
FastLED.show();

//HOUR BUTTON BABY
byte HourSwitchState = digitalRead (btnHour);
if (HourSwitchState != oldHourSwitchState)
  { 
    if (millis () - switchHourPressTime >= debounceTime)
    {
      switchHourPressTime = millis ();
      oldHourSwitchState = HourSwitchState;
      if (HourSwitchState == LOW)
        {
          configureHour();
          Serial.println ("Hour switch pressed.");
                    //INSERT: plus one hour        
        }
      else
        {
          Serial.println ("Hour switch released.");
        }
    }
  }

//MINUTE BUTTON BABY
byte MinSwitchState = digitalRead (btnMin);
if (MinSwitchState != oldMinSwitchState)
  { 
    if (millis () - switchMinPressTime >= debounceTime)
    {
      switchMinPressTime = millis ();
      oldMinSwitchState = MinSwitchState;
      if (MinSwitchState == LOW)
        {
          Serial.println ("Min switch pressed.");
          
          //INSERT: plus one minute
          
        }
      else
        {
          Serial.println ("Min switch released.");
        }
    }
  }

//COLOUR BUTTON BABY
byte ColSwitchState = digitalRead (btnCol);
if (ColSwitchState != oldColSwitchState)
  { 
    if (millis () - switchColPressTime >= debounceTime)
    {
      switchColPressTime = millis ();
      oldColSwitchState = ColSwitchState;
      if (ColSwitchState == LOW)
        {
          Serial.println ("Colour switch pressed.");
          
          // COLOUR SHIFTER / NIGHT MODE THING
          
          // if (switchColPressTime < colTime)
          //  {
          //    NIGHT MODE
          //  }
          // else
          //  {
          //     SLIDE THROUGH COLOUR PALETTE 
          //  }  
        }
    }
      else
        {
          Serial.println ("Colour switch released.");
        }
    }
  }







//SAHIL PSEUDOCODE
//int btnHourPressed(){
//   hour = (hour + 1) % 24 // In most languages % is modulus function (I think it's the same in C)
//}
//  // Again assuming global variable is 'minute' and it's an INT
//int btnMinutePressed() {
//  minute = ((minute + (5 - (minute % 5))) % 60)



