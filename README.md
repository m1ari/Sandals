Pi Based Tracker 
================

This is a High Altitude Balloon payload tracker written in multithreaded C++ designed to run on a Raspberry Pi. This version uses radio modules connected to the GPIO pins to generate the signal leaving the UART free to talk to the GPS module (currently all RPi payloads have shared the UART with the GPS and single radio module).

The code has been succesfully tested on the ground running two RTTY modules at 600 baud or a single RTTY module and single DominoEX module (NB: using the gpio header it's only possible to use a single DominoEX radio as this requires PWM).

Further Details of this Project (and flights when they happen) will be details on the [Project Website] [2]

No warranty of the fitness of this code for any purpose is given, As it's directly accessing the memory used for the GPIO interface it could cause kernel panics (or worse), backup and backup often. You have been warned!

GPS Class
---------
This connects to the GPS device and parses incomming NMEA sentences. The Process runs as a seperate thread and makes the GPS data available via the GPSPosition class.

Features To Add
* Configuration of GPS via class interface
* Set System Date and Time if not already set
* GPS
  * Track number of seen sats (and which ones)
  * UBlox specific features in GPS Class
  * Sentences to Add
    * $GPGSA - Lists satellites used and DOP
    * GPGSV - Satellites in View

RTTY Class
----------
This class creates a new thread that will enable and NTX2 / NTX2B radio and send RTTY, Connecting up the NTX2 is done in a similar way to the [UKHAS Arduino Guide][1]

With this class it's possible to set the baud rate, stop bits and start bits. You should also define the which gpio pins are used for the enable line and data line before starting the class.

You can feed two types of string into the class. The next string will overwrite the existing pending string to be sent, this is ideal for telemetry data and means the thread will always send the latest data it's been given. There's also a queue string method which will queue up data to be sent. This is designed for SSDV packets and will send the queued strings in between telemetry stings.

This class run's with a higher priority as it relies on reasonably accurate timing.

DominoEX Class
--------------
Connect NTX2B data pin to the PWN output (GPIO18) via a 10K resistor, This reduces Logic High on the NTX2B to 3V.

* DominoEX
  * Require small and accurate shifts as well as accurate timing.
  * 18 tones, Next tone is (current + 2 + nibble) % 18
  * Tone spacing and timing is based on the Baud rate (15.625 for DominoEX16)

Use PWM range to configure the tone spacing
Range should be: 3 / ( baud /2000 )
For DominoEx16 this comes to 384: (3 / (15.625 /2000)

With the Logic capped at 3V, DomEX4, DomEX6 and DomEX16 have good values

The Detailed DominoEX Spec (and details of other Domino Versions can be found [here] [3] and [here] [4]


Sentence Structure
------------------

Standard sentence looks like
$$$SANDALS,counter,time,latitude,longitude,altitude,sats,flags*checksum

flags is made up by adding powers of 2:

| Power2 | Hex     | Use       |
| ------ | ------- | ----------|
| 2^0    | 0x01    | GPS Lock  |
| 2^1    | 0x02    | Undefined  
| 2^2    | 0x04    | Undefined |
| 2^3    | 0x08    | Undefined |
| 2^4    | 0x10    | Undefined |
| 2^5    | 0x20    | Undefined |
| 2^6    | 0x40    | Undefined |
| 2^7    | 0x80    | Undefined |



Future Additions
----------------
* Images
 * Grab Images from Pi CAM
 * SSDV
 * Add Location EXIF Data to images
* APRS
 * PWM on GPIO, or Soundard






Possible future changes
  * Move RTTY / DominoEX into kernel space
  * Use timer interupt for timing (rather than usleep)

Typing changes
  use the C++ String Class rather than char*
  use unint16_t from <csdtint> instead of "unsigned short int" for checksum


RTTY (PWM or kernel module)
	dma pwm http://pythonhosted.org/RPIO/pwm_py.html



[1]: http://ukhas.org.uk/guides:linkingarduinotontx2
[2]: http://m1ari.co.uk/projects/pi-tracker
[3]: http://www.qsl.net/zl1bpu/MFSK/DEX.htm
[4]: http://dl4dss.blogspot.ie/2006/12/dominoex.html
