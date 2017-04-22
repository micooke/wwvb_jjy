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

#define WWVB_TIMECODE 1

#define WWVB_JJY_PWM

#include <wwvb_jjy.h>

uint32_t t;
uint8_t mins = 30;

void setup()
{
#if (_DEBUG > 0)
  Serial.begin(9600);
  Serial.println(F("<setup>"));
#endif

  //wwvb_jjy.set_time(__DATE__, __TIME__);
  wwvb_jjy.set_time(12, mins, 00, 28, 3, 17);
  wwvb_jjy.init(); // default : (isJJY, carrier_Hz) = (false, 60000) - wwvb @ 60kHz

  //wait for a minute to start
  while(second() > 0);
  
  wwvb_jjy.start();
  
  wwvb_jjy.print_timecode();
  
#if (_DEBUG > 0)
  Serial.println(F("</setup>"));
#endif

  t = millis();
}

void loop()
{
  if (millis() - t >= 60000)
  {
    //wwvb_jjy.set_time(12, mins, 00, 28, 3, 17);
    wwvb_jjy.print_timecode();
    t = millis();
  }
}
