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

#ifndef WWVB_JJY_PAM
#ifndef WWVB_JJY_PWM
#define WWVB_JJY_PWM
#endif
#endif

#ifndef WWVB_JJY_PULSE_LED
#define WWVB_JJY_PULSE_LED 1
#endif

//#define DEFINE_TIMEDATETOOLS // used if you do not intend to include TimeDateTools

#ifndef DEFINE_TIMEDATETOOLS
  #ifdef REQUIRE_TIMEDATESTRING
    #if (REQUIRE_TIMEDATESTRING == 0)
    #undef REQUIRE_TIMEDATESTRING
    #define REQUIRE_TIMEDATESTRING 1
    #endif
  #else
    #define REQUIRE_TIMEDATESTRING 1
  #endif
#include <TimeDateTools.h>
#endif

#include "wwvb_jjy_structs.h"
#include <TimeLib.h>           //https://github.com/PaulStoffregen/Time
#include <Timezone.h>       //https://github.com/JChristensen/Timezone
#include <PWM.h>            //https://github.com/micooke/PWM

#if defined(__AVR_ATtinyX5__)
#ifndef WWVB_JJY_MILLIS_ISR
#define WWVB_JJY_MILLIS_ISR
#endif
#endif

//                 +--------+
//                 | millis |
//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
//+------------+---+--------+--------+--------+--------+--------+
//| ATtiny85   | A |   D0   |   D1   |   --   |   --   |   --   |
//|            | B |   D1   | C=D3   |   --   |   --   |   --   |
//|       M=D4 (Toggled in software) |   --   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
//| ATmega328p | A |   D6   | M=D9   |  D12   |   --   |   --   |
//|            | B |   D5   |  D10   | C=D3   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
//| ATmega32u4 | A |  D11   | C=D9   |   --   | M=D5   |  D13   |
//+------------+---+--------+--------+--------+--------+--------+
// C = carrier pin
// M = modulation pin

// set the output pins
#if defined(__AVR_ATtinyX5__)
  #define CARRIER_PIN OCR1B_pin
  #define MODULATION_PIN 4
#elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  #define CARRIER_PIN OCR2B_pin
  #define MODULATION_PIN OCR1A_pin
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  #define CARRIER_PIN OCR1A_pin
  #define MODULATION_PIN OCR3A_pin
#endif

#ifdef DEFINE_TIMEDATETOOLS
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
#if (_DEBUG > 0)
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
  uint16_t pulse_width[3];
  volatile uint8_t WWVB_JJY_TIMECODE_INDEX = 0;
  volatile bool timecode_isnew;
  bool is_active_ = false;
  uint32_t CARRIER_FREQ_HZ = 0;
  
  wwvb_jjy_timecode frame;
    
  TimeCode() : output_offset(0) { clear_timecode(); }
  #if WWVB_TIMECODE
  void init(const uint32_t carrier_Hz = 60000);
  #else
  void init(const uint32_t carrier_Hz = 40000);
  #endif
  void start();
  void stop();

  bool is_active() { return is_active_; }
  
  void set_time(char dateString[], char timeString[]);
  void set_time(uint8_t _hour, uint8_t _minute, uint8_t _second, uint8_t _day, uint8_t _month, uint8_t _year);

  void set_timezone(TimeChangeRule DST_, TimeChangeRule STD_);
  
  void set_time_offset(int32_t _Seconds = 0);
  void set_time_offset(int8_t hours = 0, int8_t mins = 0);
  
  time_t get_output_time();
  time_t get_local_time();
  
  uint8_t get_timecode(const uint8_t i);  
  
  void clear_timecode();  
  
  void set_timecode();
  
  void print_timecode();
  void print_timecode_serial();
  
  bool is_updated() { if (timecode_isnew) { timecode_isnew = false; return true;} else {return false;} }
  
#ifdef WWVB_JJY_MILLIS_ISR
  void wwvb_jjy_millis_ISR();
#endif
 
 volatile bool output_state = !WWVB_TIMECODE;
 
#ifdef WWVB_JJY_MILLIS_ISR
  uint32_t t0;
#endif
  time_t t_output;
  int32_t output_offset;
  uint16_t carrier_period_register = 0;
  uint16_t modulation_period_register = 0;
    
  TimeChangeRule DST = {"ACDT", First, Sun, Oct, 2, 10*60+30};    //Daylight time = UTC +10:30
  TimeChangeRule STD = {"ACST", First, Sun, Apr, 3, 9*60+30};     //Standard time = UTC +9:30
};

TimeCode wwvb_jjy;

static void wwvb_jjy_compare_ISR()
{
    wwvb_jjy.output_state = WWVB_TIMECODE;
    
    #if (WWVB_JJY_PULSE_LED)
    digitalWrite(LED_BUILTIN, wwvb_jjy.output_state);
    #endif
    
    #ifdef WWVB_JJY_PWM
    // set the pulse width register of the carrier signal
    #if defined(__AVR_ATtinyX5__)
    OCR1B = wwvb_jjy.output_state*wwvb_jjy.carrier_period_register/2;
    #elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
    OCR2B = wwvb_jjy.output_state*wwvb_jjy.carrier_period_register/2;
    #elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
    OCR1A = wwvb_jjy.output_state*wwvb_jjy.carrier_period_register/2;
    #endif
    #endif
}

static void wwvb_jjy_overflow_ISR()
{
    wwvb_jjy.output_state = !WWVB_TIMECODE;
    
    #if (WWVB_JJY_PULSE_LED)
    digitalWrite(LED_BUILTIN, wwvb_jjy.output_state);
    #endif
    
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
    
    // set the pulse width register of the modulation signal
    #ifndef WWVB_JJY_MILLIS_ISR
    #if defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
    OCR1A = wwvb_jjy.pulse_width[chip];
    #elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
    OCR3A = wwvb_jjy.pulse_width[chip];
    #endif
    #endif
    
    // set the pulse width register of the carrier signal
    #ifdef WWVB_JJY_PWM
    #if defined(__AVR_ATtinyX5__)
    OCR1B = wwvb_jjy.output_state*wwvb_jjy.carrier_period_register/2;
    #elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
    OCR2B = wwvb_jjy.output_state*wwvb_jjy.carrier_period_register/2;
    #elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
    OCR1A = wwvb_jjy.output_state*wwvb_jjy.carrier_period_register/2;
    #endif
    #endif    
}

#ifdef WWVB_JJY_MILLIS_ISR
void TimeCode::wwvb_jjy_millis_ISR()
{
  const uint32_t t = millis();
  uint32_t duration_ms = t - t0;
  
  if (duration_ms > 1000)
  {
    wwvb_jjy_overflow_ISR();
    
    wwvb_jjy.t0 = t;
  }
  else
  {
    const uint8_t chip = get_timecode(WWVB_JJY_TIMECODE_INDEX);
    
    if ((duration_ms >= wwvb_jjy.pulse_width[chip]) & (output_state != WWVB_TIMECODE))
    {
      wwvb_jjy_compare_ISR();
    }
  }
}
#endif

void TimeCode::init(const uint32_t carrier_Hz)
{
  #if (WWVB_JJY_PULSE_LED)
  pinMode(LED_BUILTIN, OUTPUT);
  #endif
  
  pinMode(CARRIER_PIN, OUTPUT); // carrier pin
  
  #ifndef WWVB_JJY_MILLIS_ISR
  pinMode(MODULATION_PIN, OUTPUT); // modulation pin
  #endif
  
  wwvb_jjy.WWVB_JJY_TIMECODE_INDEX = 0;
  CARRIER_FREQ_HZ = carrier_Hz;
  
#if defined(__AVR_ATtinyX5__)
  pwm.set(1, 'b', CARRIER_FREQ_HZ, 2, 0); // carrier

  carrier_period_register = pwm.get_register(1, 'o');
#elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  pwm.set(2, 'b', CARRIER_FREQ_HZ, 2, 0); // carrier
  
  carrier_period_register = pwm.get_register(2, 'o');
  #ifndef WWVB_JJY_MILLIS_ISR
  pwm.set(1, 'a', 1, 2, WWVB_TIMECODE); // modulation
  pwm.attachInterrupt(1, 'o', wwvb_jjy_overflow_ISR);
  pwm.attachInterrupt(1, 'a', wwvb_jjy_compare_ISR);
  modulation_period_register = pwm.get_register(1, 'o');
  #endif
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  pwm.set(1, 'a', CARRIER_FREQ_HZ, 2, 0); // carrier
  carrier_period_register = pwm.get_register(1, 'o');
  #ifndef WWVB_JJY_MILLIS_ISR
  pwm.set(3, 'a', 1, 2, WWVB_TIMECODE); // modulation
  pwm.attachInterrupt(3, 'o', wwvb_jjy_overflow_ISR);
  pwm.attachInterrupt(3, 'a', wwvb_jjy_compare_ISR);
  modulation_period_register = pwm.get_register(3, 'o');
  #endif
#endif

#ifdef WWVB_JJY_MILLIS_ISR
  modulation_period_register = 1000;
#endif

  // set reference pulse widths
  // WWVB (JJY is inverted)
  // LOW :   Low for 0.2s / 1.0s (20% low duty cycle)
	// HIGH:   Low for 0.5s / 1.0s
	// MARKER: Low for 0.8s / 1.0s

#if (WWVB_TIMECODE)
  // WWVB
  //   TIME 0.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 1.0s
  // [       |     |________|________|______| ]
  // [ LOW   :____/          _______________\ ]
  // [ HIGH  :______________/         ______\ ]
  // [ MARKER:_______________________/      \ ]
  pulse_width[0] = modulation_period_register/5; // 0.2s
  pulse_width[1] = modulation_period_register/2; // 0.5s
  pulse_width[2] = modulation_period_register*4/5; // 0.8s
#else
  // JJY
  //   TIME 0.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 1.0s
  // [       |_____|________|________|      | ]
  // [ LOW   :______________         \______/ ]
  // [ HIGH  :_____         \_______________/ ]
  // [ MARKER:     \________________________/ ]
  pulse_width[0] = modulation_period_register*4/5; // 0.8s
  pulse_width[1] = modulation_period_register/2; // 0.5s
  pulse_width[2] = modulation_period_register/5; // 0.2s
#endif

#if (_DEBUG > 1)
  Serial.print("modulation_period_register = ");
  Serial.println(modulation_period_register);
  Serial.print("carrier_period_register = ");
  Serial.println(carrier_period_register);
  Serial.print("pulse_width = {");
  Serial.print(pulse_width[0]);  Serial.print(",");
  Serial.print(pulse_width[1]);  Serial.print(",");
  Serial.print(pulse_width[2]);  Serial.println("}");
#endif
}

void TimeCode::start()
{
  wwvb_jjy.set_timecode(); // sync the timecode to the current time
  
#if defined(__AVR_ATtinyX5__)
  pwm.start(1);
  t0 = millis();
#elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  pwm.start(1);
  pwm.start(2);
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  pwm.start(3);
  pwm.start(1);
#endif
  is_active_ = true;
}

void TimeCode::stop()
{
#if defined(__AVR_ATtinyX5__)
  pwm.stop(1);
#elif defined(__AVR_ATmega168__) | defined(__AVR_ATmega168P__) | defined(__AVR_ATmega328P__)
  pwm.stop(1);
  pwm.stop(2);
#elif defined(__AVR_ATmega16U4__) | defined(__AVR_ATmega32U4__)
  pwm.stop(3);
  pwm.stop(1);
#endif
  is_active_ = false;
}

void TimeCode::print_timecode()
{
#if (_DEBUG > 1)
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
#endif
}

// print_timecode_serial()
// output string is in the same format as : http://www.leapsecond.com/notes/wwvb2.htm
void TimeCode::print_timecode_serial()
{
#if (_DEBUG > 0)
  const uint8_t hour_ = BCD_to_uint8(frame.timecode.hours,1);
  const uint8_t minute_ = BCD_to_uint8(frame.timecode.minutes,1);
  const uint16_t doty_ = BCD_to_uint16(frame.timecode.doty,1);
  
#if (WWVB_TIMECODE==1)
  uint16_t year_ = BCD_to_uint16(frame.timecode.year,1);
#else
  uint16_t year_ = BCD_to_uint16(frame.timecode.year,0);
#endif

  Serial.print("'");
  if (year_ < 10) {Serial.print("0");}
  Serial.print(year_);
  Serial.print("+");
  if (doty_ < 100) {Serial.print("0");}
  if (doty_ < 10) {Serial.print("0");}
  Serial.print(doty_);
  Serial.print(" ");
  if (hour_ < 10) {Serial.print("0");}
  Serial.print(hour_);
  Serial.print(":");
  if (minute_ < 10) {Serial.print("0");}
  Serial.print(minute_);
  Serial.print("  ");

  for (uint8_t i = 0; i < 60; ++i)
  {
    switch (i)
    {
    // markers
    case 0:  // start frame
    case 9:  // end MINS
    case 19: // end HOUR
    case 29: // end DOTY
    case 39: // end DUT1
    case 49: // end YEAR
    case 59: // end MISC, end frame
      Serial.print(2); break;
    default:
      Serial.print(frame.buffer[i]);break;
    }
  }
  Serial.print("\n");
#endif
}

uint8_t TimeCode::get_timecode(const uint8_t i)
{
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
}

time_t TimeCode::get_output_time()
{
  return t_output;
}

time_t TimeCode::get_local_time()
{
  return (time_t)(t_output - output_offset);
}

void TimeCode::set_time_offset(int8_t hours, int8_t mins)
{
  output_offset = hours*3600 + mins*60;
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

  
#if (_DEBUG > 1)
  uint16_t val = 0;
  val = hour_;   Serial.print((val>10)?"":"0"); Serial.print(val); Serial.print(":");
  val = minute_; Serial.print((val>10)?"":"0"); Serial.print(val); Serial.print(" ");
  val = day_;    Serial.print((val>10)?"":"0"); Serial.print(val); Serial.print("/");
  val = month_;  Serial.print((val>10)?"":"0"); Serial.print(val); Serial.print("/20");
  val = year_;   Serial.print((val>10)?"":"0"); Serial.print(val); Serial.print(" (doty ");
  val = doty_;   Serial.print((val>100)?"":(val>10)?"0":"00"); Serial.print(val); Serial.println(")");
  
  val = hour_bcd;   Serial.print("hour_bcd   ("); Serial.print((val>100)?"":(val>10)?"0":"00"); Serial.print(val); Serial.print("):"); Serial.println(val,BIN);
  val = minute_bcd; Serial.print("minute_bcd ("); Serial.print((val>100)?"":(val>10)?"0":"00"); Serial.print(val); Serial.print("):"); Serial.println(val,BIN);
  val = doty_bcd;   Serial.print("doty_bcd   ("); Serial.print((val>100)?"":(val>10)?"0":"00"); Serial.print(val); Serial.print("):"); Serial.println(val,BIN);
  val = year_bcd;   Serial.print("year_bcd   ("); Serial.print((val>100)?"":(val>10)?"0":"00"); Serial.print(val); Serial.print("):"); Serial.println(val,BIN);
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