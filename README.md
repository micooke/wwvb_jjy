# Arduino WWVB & JJY Transmitter

02/05/2017 - Tx example now working! But its kinda useless as no sync examples are provided yet (put it in my TODO list)

An Arduino based WWVB & JJY transmitter for ATmega328p and ATmega32u4 based Arduino boards
Works with 16MHz or 8MHz boards

* Author/s: [Mark Cooke](https://www.github.com/micooke), [Martin Sniedze](https://www.github.com/mr-sneezy)
* @micooke : Software
* @mr-sneezy : Hardware

## Requirements
* [Time.h - standard Arduino library](http://www.arduino.cc/playground/Code/Time)
* [PWM.h](https://github.com/micooke/PWM/PWM.h)
* (optional) [Timezone.h](https://github.com/JChristensen/Timezone)

## Implementation notes
Note: the following `#define`'s must come in your .ino file prior to including [wwvb_jjy.h] (https://github.com/micooke/wwvb_jjy/blob/master/wwvb_jjy.h)
* Normally the inbuilt LED is flashed along with the modulation. `#define WWVB_JJY_PULSE_LED 0` will disable it
* `#define WWVB_JJY_PWM` : PAM method outputs a CW carrier and a modulation signal on two separate pins
* (default) : PWM method sets the duty cycle of the carrier to 0 for signal low. the modulation signal is still output (incase you want a blinky light?)

## About WWVB
http://www.nist.gov/pml/div688/grp40/wwvb.cfm

WWVB: 60kHz carrier, Amplitude modulated to Vp -17dB for signal low


## About JJY
http://jjy.nict.go.jp/jjy/trans/index-e.html

JJY: 40kHz carrier, Amplitude modulated to Vp -17dB for signal low (same as WWVB)

JJY is inverted compared to WWVB. At the start of a pulse it is HIGH (WWVB starts low) and the time at which it goes LOW determines the code type.

As explained in : https://en.wikipedia.org/wiki/JJY
> There are three different signals that are sent each second:
> * 0 bits consist of 0.8 s of full power, followed by 0.2 s of reduced power.
> * 1 bits consist of 0.5 s of full power, followed by 0.5 s of reduced power.
> * Marker bits consist of 0.2 s of full power, followed by 0.8 s of reduced power.

## Hookup

![wwvb wiring options](wwvb_bb.png?raw=true)

* Tx example - static time set, non-syncing examples
* Rx example - receives the modulation pin to decode your timecode for debug purposes
