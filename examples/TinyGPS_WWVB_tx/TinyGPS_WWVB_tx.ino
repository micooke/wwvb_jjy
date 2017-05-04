//                 +--------+
//                 | millis |
//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
//+------------+---+--------+--------+--------+--------+--------+
//| ATmega328p | A |   D6   | M=D9   |  D12   |   --   |   --   |
//|            | B |   D5   |  D10   | C=D3   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
//| ATmega32u4 | A |  D11   | C=D9   |   --   | M=D5   |  D13   |
//+------------+---+--------+--------+--------+--------+--------+
// C = carrier pin
// M = modulation pin
  
#define _DEBUG 1

//#define WWVB_TIMECODE 0 // JJY

//#define WWVB_JJY_PAM

//#define WWVB_JJY_MILLIS_ISR

#define DEFINE_TIMEDATETOOLS

#include <wwvb_jjy.h>
#include <TimeLib.h>       //https://github.com/PaulStoffregen/Time
#include <TinyGPS.h>       //https://github.com/mikalhart/TinyGPS
#include <SoftwareSerial.h>
// TinyGPS and SoftwareSerial libraries are the work of Mikal Hart

//const int8_t clock_timezone[2] = { -6, 0 }; // This is the timezone of your wwvb/jjy clock : CST (UTC -6:00)
const int8_t clock_timezone[2] = { 6, 0 }; // This is the timezone of your wwvb/jjy clock : UTC

SoftwareSerial SerialGPS = SoftwareSerial(10, 11);  // receive on pin 10
TinyGPS gps; 

void setup()
{
#if (_DEBUG > 0)
  Serial.begin(9600);
  Serial.println(F("<setup>"));
#endif
  
  wwvb_jjy.set_time(__DATE__, __TIME__);
  //wwvb_jjy.set_time(8, 43, 00, 7, 2, 05);
  wwvb_jjy.set_time_offset(clock_timezone[0],clock_timezone[1]);
  wwvb_jjy.init(); // default : (isJJY, carrier_Hz) = (false, 60000) - wwvb @ 60kHz
  
  SerialGPS.begin(9600);
  Serial.println("Waiting for GPS time ... ");

  // sync the time with gps
  while (SerialGPS.available())
  {
    if (gps.encode(SerialGPS.read()))
    { // process gps messages
      // when TinyGPS reports new data...
      unsigned long age;
      int Year;
      byte Month, Day, Hour, Minute, Second;
      gps.crack_datetime(&Year, &Month, &Day, &Hour, &Minute, &Second, NULL, &age);
      if (age < 500)
      {
        // set the Time to the latest GPS reading
        setTime(Hour, Minute, Second, Day, Month, Year);
      }
    }
  }
  
  //wait for a minute to start
  while(second() > 0);
  wwvb_jjy.start();
   
  //wwvb_jjy.print_timecode();
  wwvb_jjy.print_timecode_serial();
  
#if (_DEBUG > 0)
  Serial.println(F("</setup>"));
#endif
}

void loop()
{
#if (_DEBUG > 0)
  // print the timecode serial every minute
  if (wwvb_jjy.is_updated())
  {
    //wwvb_jjy.set_time(12, mins, 00, 28, 3, 17);
    //wwvb_jjy.print_timecode();
    wwvb_jjy.print_timecode_serial();
  }
#endif

#if defined(WWVB_JJY_MILLIS_ISR)
  // the main loop is used to call the ISR without a timer (uses calls to millis())
  wwvb_jjy.wwvb_jjy_millis_ISR(); 
#endif

  // sync the time with gps
  while (SerialGPS.available())
  {
    if (gps.encode(SerialGPS.read()))
    { // process gps messages
      // when TinyGPS reports new data...
      unsigned long age;
      int Year;
      byte Month, Day, Hour, Minute, Second;
      gps.crack_datetime(&Year, &Month, &Day, &Hour, &Minute, &Second, NULL, &age);
      if (age < 500)
      {
        // set the Time to the latest GPS reading
        setTime(Hour, Minute, Second, Day, Month, Year);
      }
    }
  }
  //

}
