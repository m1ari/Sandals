Pi Based Tracker written in multithreaded C++

This is an attempt at bit banging a GPIO Pin to generate RTTY rather than sharing the UART with GPS and Radio Module

No warranty of the fitness of this code for any purpose is given, As it's directly accessing the memory used for the GPIO interface it could cause kernel panics (or worse), backup and backup often. You have been warned!

Standard sentence looks like
$$$SANDALS,counter,time,latitude,longitude,altitude,sats,flags*checksum

flags are powers of 2
2^0	 1	GPS Lock
2^1	 2	Undefined
2^2	 4	Undefined
2^3	 8	Undefined

2^4	1	Undefined
2^5	2	Undefined
2^6	4	Undefined
2^7	8	Undefined





Features To Add
 * Set System Date and Time if not already set
 * GPS
   * Track number of seen sats (and which ones)
   * UBlox specific features in GPS Class
   * Sentences to Add
     * $GPGSA - Lists satellites used and DOP
     * GPGSV - Satellites in View

 * Images
   * Grab Images from Pi CAM
   * SSDV
   * Add Location EXIF Data to images
 * APRS
   * PWM on GPIO, or Soundard
 * DominoEX16
   * Small shifts needed
   * 18 tones with 355Hz BW
   * Tones increase by the value in the lookup table + 2 * baud, if new tone >18 remove 18 from the value
   * Baud for DomEx16 is 15.625 (0.0078125V per tone)
   * Tone spacing matches Baud rate - 0.0078125V per tone for NTX2 (2000Hz/V)


Logic High is 3v3

 Over 1s
 -> 1V needs on 1/3.3 of the time
 .1v needs on 1/33 of the time

 Per tone 0.00236742424242424242424242424242V
 Need to go high once every 422.4 Cycles


   * Run at multiple of 6600Hz (allows Baud * minimum PWN Rate Required)



Possible future changes
  * Move RTTY / DominoEX into kernel space
  * Use timer interupt for timing (rather than usleep)

Typing changes
  use the C++ String Class rather than char*
  use unint16_t from <csdtint> instead of "unsigned short int" for checksum


RTTY (PWM or kernel module)
	dma pwm http://pythonhosted.org/RPIO/pwm_py.html
