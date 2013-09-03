#ifndef GPS_H_
#define GPS_H_

#include <termios.h>
#include <string>
#include "gpsPosition.h"

enum gpsType {gpsOTHER,gpsUBLOX};

class GPS {
	private:
		pthread_t threadid;
		int uart0_filestream;
		bool gps_run;			// GPS Thread Running
		GPSPosition Position;
		static void* entryPoint(void *pthis);
		void gps_open();		// Configure the GPS port and start getting data
		void parserThread();
		bool checkChecksum(char *nmea);
		void nmeaGPGGA(char *nmea);	// Parse a GPGGA string
		void nmeaGPRMC(char *nmea);	// Parse a GPRMC string

		bool deviceopen;		// Is the device open
		std::string device;
		speed_t baud;
		int bits;
		int stopbits;
		char parity;

		// thread id
		// static void thread setup
	public:
		GPS();
		~GPS();
		GPSPosition GetPosition();	// Report back current position
		void Run();			// Run as a seperate thread to get data from the GPS
		void Stop();			// Exit the run loop

		// Should we make these bools ?
		void setDevice(std::string d);
		void setBaud(int b);
		void setBits(int b);
		void setStopBits(int b);
		void setParity(char p);
		void setType(gpsType t);

};

#endif /* GPS_H_ */
