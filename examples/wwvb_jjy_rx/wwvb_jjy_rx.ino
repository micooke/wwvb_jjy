#define _DEBUG 1

#ifndef WWVB_TIMECODE
#define WWVB_TIMECODE 1
#endif

#define DEFINE_TIMEDATETOOLS

#include <TimeLib.h> //https://github.com/PaulStoffregen/Time
#include <wwvb_jjy.h>

uint8_t _idx = 0;
uint32_t pIn;
int8_t time_code = 0;
int8_t prev_time_code = 0;
  
#define IS_WWVB 1
#define INPUT_PIN 2
#define SECS 1000000
#define MILLISECS 1000UL

void setup()
{
#if (_DEBUG > 0)
  Serial.begin(9600);
  Serial.println(F("setup complete"));
#endif

  pinMode(INPUT_PIN, INPUT);
}

void loop()
{
  pIn = pulseIn(INPUT_PIN, LOW, 2*SECS);

  time_code = pw_to_timecode(pIn);

#if (_DEBUG == 2)
  Serial.print(_idx);  Serial.print("|");
  Serial.print(time_code);  Serial.print(":");
  Serial.println(pIn);
#endif

  if (time_code >= 0)
  {
    if ((prev_time_code == 2) && (time_code == 2))
    {
      wwvb_jjy.frame.buffer = 0;
      _idx = 0;
    }
    
    if (time_code < 2)
    {
      wwvb_jjy.frame.buffer.set(_idx, time_code);
    }
    
    if (++_idx > 59)
    {
      print_time();
      wwvb_jjy.print_timecode();
      
      _idx = 0;
    }
  }

  prev_time_code = time_code;
}

int8_t pw_to_timecode(uint32_t pulse_width_us)
{
  if (pulse_width_us == 0)
  {
    return -1;
  }
  else if (pulse_width_us < 300*MILLISECS)
  {
    return 0;
  }
  else if (pulse_width_us < 600*MILLISECS)
  {
    return 1;
  }
  else
  {
    return 2;
  }
}

void print_time()
{
#if (_DEBUG > 0)
#if (WWVB_TIMECODE==1)
  Serial.print(BCD_to_uint8(wwvb_jjy.frame.timecode.hours,1)); Serial.print(":");
  Serial.print(BCD_to_uint8(wwvb_jjy.frame.timecode.minutes,1)); Serial.print(":00 day ");
  
  Serial.print(BCD_to_uint16(wwvb_jjy.frame.timecode.doty,1)); Serial.print(" of the year ");
  Serial.println(2000 + BCD_to_uint8(wwvb_jjy.frame.timecode.year,1)); 
#else
  Serial.print(BCD_to_uint8(wwvb_jjy.frame.timecode.hours,1)); Serial.print(":");
  Serial.print(BCD_to_uint8(wwvb_jjy.frame.timecode.minutes,1)); Serial.print(":00 day ");
  
  Serial.print(BCD_to_uint16(wwvb_jjy.frame.timecode.doty,1)); Serial.print(" of the year ");
  Serial.println(2000 + BCD_to_uint8(wwvb_jjy.frame.timecode.year,0));
#endif
#endif
}

