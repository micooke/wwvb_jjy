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

#include <TimeLib.h> //https://github.com/PaulStoffregen/Time
#include <wwvb_jjy.h>
#include <AD9850.h>

const int W_CLK_PIN = 7;
const int FQ_UD_PIN = 8;
const int DATA_PIN = 10;
const int RESET_PIN = 11;

double freq = 60000;
double trimFreq = 124999500;

int phase = 0;

void setup()
{
#if (_DEBUG > 0)
  Serial.begin(9600);
  Serial.println(F("<setup>"));
#endif

  //wwvb_jjy.set_time(__DATE__, __TIME__);
  wwvb_jjy.set_time(8, 43, 00, 7, 2, 05);
  
  wwvb_jjy.init(); // default : (isJJY, carrier_Hz) = (false, 60000) - wwvb @ 60kHz
  
  DDS.begin(W_CLK_PIN, FQ_UD_PIN, DATA_PIN, RESET_PIN);
  DDS.calibrate(trimFreq);
  DDS.down();

#ifndef WWVB_JJY_MILLIS_ISR
  #if defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
    pwm.attachInterrupt(1, 'o', DDS_overflow_ISR);
    pwm.attachInterrupt(1, 'a', DDS_compare_ISR);
  #elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
    pwm.attachInterrupt(3, 'o', DDS_overflow_ISR);
    pwm.attachInterrupt(3, 'a', DDS_compare_ISR);
  #endif
#endif

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

static void DDS_compare_ISR()
{
    wwvb_jjy.output_state = WWVB_TIMECODE;
    
    #if (WWVB_JJY_PULSE_LED)
    digitalWrite(LED_BUILTIN, wwvb_jjy.output_state);
    #endif
    
    if (wwvb_jjy.output_state)
    {
      DDS.setfreq(freq, phase);
    }
    else
    {
      DDS.down();
    }
}

static void DDS_overflow_ISR()
{
    wwvb_jjy.output_state = !WWVB_TIMECODE;
    
    #if (WWVB_JJY_PULSE_LED)
    digitalWrite(LED_BUILTIN, wwvb_jjy.output_state);
    #endif

    if (wwvb_jjy.output_state)
    {
      DDS.down();
    }
    else
    {
      DDS.setfreq(freq, phase);
    }
    
    const uint8_t chip = wwvb_jjy.get_timecode(wwvb_jjy.WWVB_JJY_TIMECODE_INDEX);
    
    ++wwvb_jjy.WWVB_JJY_TIMECODE_INDEX; // increment the TIMECODE INDEX
    
    if (wwvb_jjy.WWVB_JJY_TIMECODE_INDEX == 1)
    {
      wwvb_jjy.timecode_isnew = false;
    }
    // increment the TIMECODE index
    else if (wwvb_jjy.WWVB_JJY_TIMECODE_INDEX > 59)
    {
      wwvb_jjy.WWVB_JJY_TIMECODE_INDEX = 0;
      
      wwvb_jjy.set_timecode(); // sync the timecode to the current time
      wwvb_jjy.timecode_isnew = true;
    } 
}