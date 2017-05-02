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

#include <wwvb_jjy.h>

void setup()
{
#if (_DEBUG > 0)
  Serial.begin(9600);
  Serial.println(F("<setup>"));
#endif
  
  //wwvb_jjy.set_time(__DATE__, __TIME__);
  wwvb_jjy.set_time(8, 43, 00, 7, 2, 05);
  
  wwvb_jjy.init(); // default : (isJJY, carrier_Hz) = (false, 60000) - wwvb @ 60kHz
  
  //wait for a minute to start
  //while(second() > 0);
  
  wwvb_jjy.start();
  
  //wwvb_jjy.print_timecode();
  wwvb_jjy.print_timecode_serial();
  
#if (_DEBUG > 0)
  Serial.println(F("</setup>"));
#endif
}

void loop()
{
  // print the timecode serial every minute
  if (wwvb_jjy.is_updated())
  {
    //wwvb_jjy.set_time(12, mins, 00, 28, 3, 17);
    //wwvb_jjy.print_timecode();
    wwvb_jjy.print_timecode_serial();
  }
  
  #if defined(WWVB_JJY_MILLIS_ISR)
  // the main loop is used to call the ISR without a timer (uses calls to millis())
  wwvb_jjy.wwvb_jjy_millis_ISR(); 
  #endif
}
