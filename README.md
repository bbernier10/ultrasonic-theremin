# Ultrasonic Theremin

Final project for ECE 3525, Embedded Systems

Coded by [Brandon Bernier](mailto:bbernier@gwu.edu) 

This project combined many of the topics covered during my Embedded Systems course including serial communication and PWM. An ATmega16 microcontroller is programmed to read in the output of an ultrasonic rangefinder and output different distance-dependant tones to a speaker. While not truly a theremin, this could be played in much the same way as one.

## Usage

Compile the code using Atmel Studio and write the hex file to the ATmega16 microprocessor using an STK500 board. The serial output from the ultrasonic rangefinder is used to read its values, so ensure that is properly connected to the serial input (RX) pin of the microprocessor. Similarly, ensure a speaker is connected to the PWM output pin of the microprocessor. Once the board is setup correctly and the code is uploaded, move your hand above the ultrasonic rangefinder to hear the tones change. 