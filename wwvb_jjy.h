#ifndef wwvb_jjy_h
#define wwvb_jjy_h

/*
Designed for the ATmega328p and ATmega32u4

Copyright (c) 2017 Mark Cooke, Martin Sniedze

Author/s: [Mark Cooke](https://www.github.com/micooke), [Martin Sniedze](https://www.github.com/mr-sneezy)

License: MIT license (see LICENSE file).
*/

#ifndef _DEBUG
#define _DEBUG 1
#endif

#ifndef WWVB_TIMECODE
#define WWVB_TIMECODE 1
#endif

#ifndef WWVB_JJY_PWM
#define WWVB_JJY_PAM
#endif

#ifdef __AVR_ATtinyX5__
  #ifdef WWVB_JJY_PAM
  #undef WWVB_JJY_PAM
  #endif
#endif

#include "wwvb_jjy_structs.h"
#include <Time.h>           //http://www.arduino.cc/playground/Code/Time
#include <Timezone.h>       //https://github.com/JChristensen/Timezone
#include <PWM.h>            //https://github.com/micooke/PWM

#ifndef TimeDateTools_h
  //		      									               Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
  const uint16_t cumulativeDIAM[12] PROGMEM = { 0 , 31, 59, 90,120,151,181,212,243,273,304,334 }; // cumulative
  #define cumulative_days_in_a_month(k) pgm_read_word_near(cumulativeDIAM + k)

  template <typename T>
  uint16_t to_day_of_the_year(const T &_DD, const T &_MM, const bool &_is_leap_year)
  {
    // if its a leap year and the month is March -> December, add +1
    return cumulative_days_in_a_month(_MM - 1) + _DD + (_is_leap_year & (_MM > 2));
  }

  bool is_leap_year(const uint16_t &_year)
  {
    const bool is_DivBy4 = (_year % 4) == 0;
    const bool is_DivBy100 = (_year % 100) == 0;
    const bool is_DivBy400 = (_year % 400) == 0;

    return (is_DivBy4 & (!is_DivBy100 | is_DivBy400));
  }
  
  uint8_t ascii_to_int(const char &c0, const char &c1)
  {
    return (c0 - '0') * 10 + (c1 - '0');
  }

  void TimeString_to_HHMMSS(const char timeString[], uint8_t &hh, uint8_t &mm, uint8_t &ss)
  {
    //Example: "23:59:01"

    hh = ascii_to_int(timeString[0], timeString[1]); // [2] = ':'
    mm = ascii_to_int(timeString[3], timeString[4]); // [5] = ':'
    ss = ascii_to_int(timeString[6], timeString[7]);
  }

  void DateString_to_DDMMYY(const char dateString[], uint8_t &DD, uint8_t &MM, uint8_t &YY)
  {
    //Example: "Feb 12 1996"
    //         "Jan  1 2016"

    // convert the Month string to Month index [0->12]
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (dateString[0])
    {
      case 'J': MM = dateString[1] == 'a' ? 1 : dateString[2] == 'n' ? 6 : 7; break;
      case 'F': MM = 2; break;
      case 'A': MM = dateString[2] == 'r' ? 4 : 8; break;
      case 'M': MM = dateString[2] == 'r' ? 3 : 5; break;
      case 'S': MM = 9; break;
      case 'O': MM = 10; break;
      case 'N': MM = 11; break;
      case 'D': MM = 12; break;
    }

    // if the day number is < 10, the day is (unfortunately) space padded instead of zero-padded
    if (dateString[4] == ' ')
    {
      DD = ascii_to_int('0', dateString[5]);
    }
    else
    {
      DD = ascii_to_int(dateString[4], dateString[5]);
    }
    YY = ascii_to_int(dateString[9], dateString[10]);
  }
#endif

static volatile uint8_t WWVB_JJY_TIMECODE_INDEX = 0;
static volatile uint32_t WWVB_PWM_ISR_COUNT = 0;
static uint32_t WWVB_CARRIER_HZ = 0;
static volatile bool WWVB_PWM_IS_TOGGLED = false;
static volatile uint16_t WWVB_PWM_PW = 0;
static uint16_t WWVB_PERIOD = 0;

bool parity_8b(const uint8_t in, const bool isEven = true)
{
  uint8_t val = in;
  val ^= val >> 4;
  val ^= val >> 2;
  return (val & !isEven) & 0x01;
}

bool parity_16b(const uint16_t in, const bool isEven = true)
{
  uint16_t val = in;
  val ^= val >> 8;
  val ^= val >> 4;
  val ^= val >> 2;
  return (val & !isEven) & 0x01;
}

uint8_t uint8_to_BCD(const uint8_t in, const bool dec_inc = 0)
{
  return ((in / 10) << (4 + dec_inc)) | (in % 10);
}

uint16_t uint16_to_BCD(const uint16_t in, const bool dec_inc = 0)
{
  return ((in / 100) << (8 + 2*dec_inc)) | ((in / 10)%10 << (4 + dec_inc)) | (in % 10);
}

uint8_t BCD_to_uint8(const uint8_t in, const bool dec_inc = 0)
{
  return ((in >> (4 + dec_inc)) & 0x0F)*10 + (in & 0x0F);
}

uint16_t BCD_to_uint16(const uint16_t in, const bool dec_inc = 0)
{
  return ((in >> (8 + 2*dec_inc)) & 0x0F)*100 + ((in >> (4 + dec_inc)) & 0x0F)*10 + (in & 0x0F);
}

void print_bcd(const uint16_t in, const uint8_t num_bits = 16, const bool dec_inc = 0)
{
#if _DEBUG > 0
  for (int8_t i=num_bits-1; i>=0; --i)
  {
    if ((((i+1) % 5) == 0) && (dec_inc == 1))
    {
      Serial.print("-");
    }
    else
    {
      Serial.print((in >> i) & 0x01);
    }
  }
#endif
}

class TimeCode
{
 public:
  uint32_t pulse_width[3];
  wwvb_jjy_timecode frame;
  
  TimeCode() : output_offset(0) { clear_timecode(); }
  
  void init(const uint32_t carrier_Hz = 60000);
  void start();
  void stop();
  
  void set_time(char dateString[], char timeString[]);
  void set_time(uint8_t _hour, uint8_t _minute, uint8_t _second, uint8_t _day, uint8_t _month, uint8_t _year);

  void set_timezone(TimeChangeRule DST_, TimeChangeRule STD_);
  
  void set_time_offset(int32_t _Seconds = 0);
  
  time_t get_output_time();
  time_t get_local_time();
  
  uint8_t get_timecode(const uint8_t i);  
  
  void clear_timecode();  
  
  void set_timecode();
  
  void print_timecode();
  
 private:
  time_t t_output;
  int32_t output_offset;
    
  TimeChangeRule DST = {"ACDT", First, Sun, Oct, 2, 10*60+30};    //Daylight time = UTC +10:30
  TimeChangeRule STD = {"ACST", First, Sun, Apr, 3, 9*60+30};     //Standard time = UTC +9:30
  TimeChangeRule *tcr;  
};

TimeCode wwvb_jjy;

static void WWVB_ISR()
{
#ifdef WWVB_JJY_PAM
  if (WWVB_JJY_TIMECODE_INDEX == 0)
  {
    wwvb_jjy.set_timecode(); // sync the timecode to the current time
  }
  
  const uint8_t chip = wwvb_jjy.get_timecode(WWVB_JJY_TIMECODE_INDEX++);

  if (WWVB_JJY_TIMECODE_INDEX > 59)
  {
    WWVB_JJY_TIMECODE_INDEX = 0;
  }

  // set the pulse width register
  #if defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  OCR1A = wwvb_jjy.pulse_width[chip];
  #elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  OCR3A = wwvb_jjy.pulse_width[chip];
  #endif
  
#else //PWM
  if (WWVB_PWM_ISR_COUNT >= (WWVB_CARRIER_HZ - 1)) // 1s elapsed
  {
    WWVB_PWM_ISR_COUNT= 0;
    
    if (WWVB_JJY_TIMECODE_INDEX == 0)
    {
      wwvb_jjy.set_timecode(); // sync the timecode to the current time
    }
  }
  
  if (WWVB_PWM_ISR_COUNT == 0) // start of the frame
  {
    const uint8_t chip = wwvb_jjy.get_timecode(WWVB_JJY_TIMECODE_INDEX++);
    
    if (WWVB_JJY_TIMECODE_INDEX > 59)
    {
      WWVB_JJY_TIMECODE_INDEX = 0;
    }
    
    WWVB_PWM_PW = wwvb_jjy.pulse_width[chip];
    
    WWVB_PWM_IS_TOGGLED = false;
    WWVB_PWM_ISR_COUNT = 0;
    
    // set the pulse width register of the carrier
    #if defined(__AVR_ATtinyX5__)
      OCR1B = (!WWVB_TIMECODE)*WWVB_PERIOD/2;
    #elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
      OCR2B = (!WWVB_TIMECODE)*WWVB_PERIOD/2;
    #elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
      OCR1A = (!WWVB_TIMECODE)*WWVB_PERIOD/2;
    #endif
  }
  else if ((WWVB_PWM_IS_TOGGLED == false) && (WWVB_PWM_ISR_COUNT >= (WWVB_PWM_PW -1))) // pulse width elapsed
  {
    WWVB_PWM_IS_TOGGLED = true;
    
    // set the pulse width register of the carrier
    #if defined(__AVR_ATtinyX5__)
      OCR1B = WWVB_TIMECODE*WWVB_PERIOD/2;
    #elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
      OCR2B = WWVB_TIMECODE*WWVB_PERIOD/2;
    #elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
      OCR1A = WWVB_TIMECODE*WWVB_PERIOD/2;
    #endif    
  }
  
  ++WWVB_PWM_ISR_COUNT;
#endif
}

void TimeCode::print_timecode()
{
  for (uint8_t i = 0; i < 60; ++i)
		{
			switch (i)
			{
				// markers
			case 0:  // start frame
				Serial.println(F("INDX: [0 1 2 3 4 5 6 7 8 9]"));
				Serial.print(F("MINS: [M ")); break;
			case 9:  // end MINS
				Serial.print(F("M]\n"));
				Serial.print(F("HOUR: [")); break;
			case 19: // end HOUR
				Serial.print(F("M]\n"));
				Serial.print(F("DOTY: [")); break;
			case 29: // end DOTY
				Serial.print(F("M]\n"));
				Serial.print(F("DUT1: [")); break;
			case 39: // end DUT1
				Serial.print(F("M]\n"));
				Serial.print(F("YEAR: [")); break;
			case 49: // end YEAR
				Serial.print(F("M]\n"));
				Serial.print(F("MISC: [")); break;
			case 59: // end MISC, end frame
				Serial.print(F("M]\n")); break;
			default:
				Serial.print(frame.buffer[i]);
				Serial.print(' '); break;
			}
		}
}

void TimeCode::init(const uint32_t carrier_Hz)
{
  WWVB_JJY_TIMECODE_INDEX = 0;
  WWVB_CARRIER_HZ = carrier_Hz;
  //                 +--------+
  //                 | millis |
  //+------------+---+--------+--------+--------+--------+--------+
  //| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
  //+------------+---+--------+--------+--------+--------+--------+
  //| ATtiny85   | A |   D0   |   D1   |   --   |   --   |   --   |
  //|            | B |   D1   | C=D3   |   --   |   --   |   --   |
  //+------------+---+--------+--------+--------+--------+--------+
  //| ATmega328p | A |   D6   | M=D9   |  D12   |   --   |   --   |
  //|            | B |   D5   |  D10   | C=D3   |   --   |   --   |
  //+------------+---+--------+--------+--------+--------+--------+
  //| ATmega32u4 | A |  D11   | C=D9   |   --   | M=D5   |  D13   |
  //+------------+---+--------+--------+--------+--------+--------+
  // C = carrier pin
  // M = modulation pin

#if defined(__AVR_ATtinyX5__)
  // ignore PAM at this stage
  pwm.set(1, 'b', WWVB_CARRIER_HZ, 2, 0); // carrier
  pwm.attachInterrupt(1, WWVB_ISR);
  WWVB_PERIOD = pwm.getPeriodRegister(1);
#elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  pwm.set(2, 'b', WWVB_CARRIER_HZ, 2, 0); // carrier
  
  #ifdef WWVB_JJY_PAM
  pwm.set(1, 'a', 1, 2, WWVB_TIMECODE); // modulation
  pwm.attachInterrupt(1, WWVB_ISR);
  WWVB_PERIOD = pwm.getPeriodRegister(1);
  #else
  pwm.attachInterrupt(2, WWVB_ISR);
  WWVB_PERIOD = pwm.getPeriodRegister(2);
  #endif
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  pwm.set(1, 'a', WWVB_CARRIER_HZ, 2, 0); // carrier

  #ifdef WWVB_JJY_PAM
  pwm.set(3, 'a', 1, 2, WWVB_TIMECODE); // modulation
  pwm.attachInterrupt(3, WWVB_ISR);
  WWVB_PERIOD = pwm.getPeriodRegister(3);
  #else
  pwm.attachInterrupt(1, WWVB_ISR);
  WWVB_PERIOD = pwm.getPeriodRegister(1);
  #endif
#endif
  
  // set reference pulse widths
  // WWVB (JJY is inverted)
  // LOW :   Low for 0.2s / 1.0s (20% low duty cycle)
	// HIGH:   Low for 0.5s / 1.0s
	// MARKER: Low for 0.8s / 1.0s
  

#if (WWVB_TIMECODE==1)
  // WWVB
  //   TIME 0.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 1.0s
  // [       |     |________|________|______| ]
  // [ LOW   :____/          _______________\ ]
  // [ HIGH  :______________/         ______\ ]
  // [ MARKER:_______________________/      \ ]
  #ifdef WWVB_JJY_PAM
  pulse_width[0] = WWVB_PERIOD/5; // 0.2s
  pulse_width[1] = WWVB_PERIOD/2; // 0.5s
  pulse_width[2] = WWVB_PERIOD*4/5; // 0.8s
  #else
  pulse_width[0] = WWVB_CARRIER_HZ/5; // 0.2s
  pulse_width[1] = WWVB_CARRIER_HZ/2; // 0.5s
  pulse_width[2] = WWVB_CARRIER_HZ*4/5; // 0.8s
  #endif

#else
  // JJY
  //   TIME 0.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 1.0s
  // [       |_____|________|________|      | ]
  // [ LOW   :______________         \______/ ]
  // [ HIGH  :_____         \_______________/ ]
  // [ MARKER:     \________________________/ ]
  #ifdef WWVB_JJY_PAM
  pulse_width[0] = WWVB_PERIOD*4/5; // 0.8s
  pulse_width[1] = WWVB_PERIOD/2; // 0.5s
  pulse_width[2] = WWVB_PERIOD/5; // 0.2s
  #else
  pulse_width[0] = WWVB_CARRIER_HZ*4/5; // 0.8s
  pulse_width[1] = WWVB_CARRIER_HZ/2; // 0.5s
  pulse_width[2] = WWVB_CARRIER_HZ/5; // 0.2s
  #endif

#endif

  // set the output pins
#if defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  pinMode(OCR2B_pin, OUTPUT); // carrier pin
  pinMode(OCR1A_pin, OUTPUT); // modulation pin
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  pinMode(OCR1A_pin, OUTPUT); // carrier pin
  pinMode(OCR3A_pin, OUTPUT); // modulation pin
#endif
}

void TimeCode::start()
{
#if defined(__AVR_ATtinyX5__)
  pwm.start(1);
#elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  #ifdef WWVB_JJY_PAM
  pwm.start(1);
  #endif
  pwm.start(2);
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  #ifdef WWVB_JJY_PAM
  pwm.start(3);
  #endif
  pwm.start(1);
#endif
}

void TimeCode::stop()
{
#if defined(__AVR_ATtinyX5__)
  pwm.stop(1);
#elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  #ifdef WWVB_JJY_PAM
  pwm.stop(1);
  #endif
  pwm.stop(2);
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  #ifdef WWVB_JJY_PAM
  pwm.stop(3);
  #endif
  pwm.stop(1);
#endif
}

uint8_t TimeCode::get_timecode(const uint8_t i)
{
#if (_DEBUG > 0)
  switch (i)
  {
    case  0: // M - start of frame
    case  9: // P1
    case 19: // P2
    case 29: // P3
    case 39: // P4
    case 49: // P5
    case 59: // P0 - end of frame
      return 2;
    default:
      return frame.buffer[i];
  }
#endif
}

time_t TimeCode::get_output_time()
{
  return t_output;
}

time_t TimeCode::get_local_time()
{
  return (time_t)(t_output - output_offset);
}

void TimeCode::set_time_offset(int32_t _Seconds)
{
  output_offset = _Seconds;
}

void TimeCode::set_timecode()
{
  Timezone myTZ(DST, STD);
  t_output = myTZ.toLocal(now());
  
  bool is_dst = myTZ.locIsDST(t_output);
  
  const uint8_t hour_ = hour(t_output);
  const uint8_t minute_ = minute(t_output);
  const uint8_t day_ = day(t_output);
  const uint8_t month_ = month(t_output);
  const uint32_t year_ = year(t_output)%100;
  
  const bool is_leap_year_ = is_leap_year(30+year_); // Time.cpp
  const uint16_t doty_ = to_day_of_the_year<uint8_t>(day_, month_, is_leap_year_);
    
  const uint8_t hour_bcd = uint8_to_BCD(hour_,1);
  const uint8_t minute_bcd = uint8_to_BCD(minute_,1);
  const uint16_t doty_bcd = uint16_to_BCD(doty_,1);
    
#if (WWVB_TIMECODE==1)
  uint16_t year_bcd = uint16_to_BCD(year_,1);
#else
  uint16_t year_bcd = uint16_to_BCD(year_,0);
#endif

  
#if (_DEBUG > 0)
  Serial.print("hour  :"); Serial.println(hour_);
  Serial.print("minute:"); Serial.println(minute_);
  Serial.print("day   :"); Serial.println(day_);
  Serial.print("month :"); Serial.println(month_);
  Serial.print("year  :"); Serial.println(year_);
  Serial.print("doty  :"); Serial.println(doty_);
  
  Serial.print("hour_bcd   [1]("); Serial.print(hour_bcd); Serial.print("):"); Serial.println(hour_bcd,BIN);
  Serial.print("minute_bcd [1]("); Serial.print(minute_bcd); Serial.print("):");Serial.println(minute_bcd,BIN);
  Serial.print("doty_bcd   [1]("); Serial.print(doty_bcd); Serial.print("):");Serial.println(doty_bcd,BIN);
  Serial.print("year_bcd   [1]("); Serial.print(year_bcd); Serial.print("):");Serial.println(year_bcd,BIN);
#endif
  
  // set the timecode
  frame.timecode.minutes = minute_bcd;
  frame.timecode.hours = hour_bcd;
  frame.timecode.doty = doty_bcd;
  frame.timecode.year = year_bcd;
    
#if (WWVB_TIMECODE==1)
  frame.timecode.dut1 = 2; // 010 = -
  //frame.timecode.dut1value = 0; // DUT1 = UTC
  frame.timecode.lyi = is_leap_year_;
  //frame.timecode.lsw = 0;
  frame.timecode.dst = is_dst*3; // 11 = DST, 0 = no DST
#else
  frame.timecode.pa1 = parity_8b(hour_bcd);
  frame.timecode.pa2 = parity_8b(minute_bcd);
  frame.timecode.dotw = weekday(t_output);
  //frame.timecode.ls1 = 0; // no leap second at end of month
  //frame.timecode.ls2 = 0; // leap second is deleted
#endif
}

void TimeCode::clear_timecode()
{
  frame.buffer = 0;
}

void TimeCode::set_time(char dateString[], char timeString[])
{
  uint8_t _minute, _hour, _second, _day, _month, _year;

  DateString_to_DDMMYY(dateString, _day, _month, _year);
  TimeString_to_HHMMSS(timeString, _hour, _minute, _second);

  setTime(_hour, _minute, _second, _day, _month, 2000 + _year); //Time::setTime
  adjustTime(output_offset); // add local -> output offset
}

void TimeCode::set_time(uint8_t _hour, uint8_t _minute, uint8_t _second, uint8_t _day, uint8_t _month, uint8_t _year)
{
  setTime(_hour, _minute, _second, _day, _month, 2000 + _year); //Time::setTime
  adjustTime(output_offset); // add local -> output offset
}

void TimeCode::set_timezone(TimeChangeRule DST_, TimeChangeRule STD_)
{
  DST = DST_; //Daylight time
  STD = STD_; //Standard time
}

#endif
