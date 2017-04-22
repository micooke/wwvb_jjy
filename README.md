# Arduino WWVB & JJY Transmitter

## NOT WORKING... YET

An Arduino based WWVB & JJY transmitter for ATmega328p and ATmega32u4 based Arduino boards
Works with 16MHz or 8MHz boards

* Author/s: [Mark Cooke](https://www.github.com/micooke), [Martin Sniedze](https://www.github.com/mr-sneezy)
* @micooke : Software
* @mr-sneezy : Hardware

## Requirements
* [Time.h - standard Arduino library](http://www.arduino.cc/playground/Code/Time)
* [Timezone.h](https://github.com/JChristensen/Timezone)
* [PWM.h](https://github.com/micooke/PWM/PWM.h)

## About WWVB
http://www.nist.gov/pml/div688/grp40/wwvb.cfm

WWVB: 60kHz carrier, Amplitude modulated to Vp -17dB for signal low

Hopefully your wwvb receiver is insensitive to this -17dB value as this library uses pulse amplitude modulation to set a 0% duty cycle for the low signal

## About JJY
 #TODO

## Hookup

![wwvb wiring options](wwvb_bb.png?raw=true)

*See examples folder (#TODO)*
