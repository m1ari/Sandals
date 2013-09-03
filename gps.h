#ifndef GPS_H_
#define GPS_H_

#include "gpsPosition.h"
#include <string>

class GPS {
	private:
		pthread_t threadid;
		int uart0_filestream;
		int gps_run;
		GPSPosition Position;
		static void* entryPoint(void *pthis);
		void parserThread();
		bool checkChecksum(char *nmea);
		void nmeaGPGGA(char *nmea);	// Parse a GPGGA string
		void nmeaGPRMC(char *nmea);	// Parse a GPRMC string

		bool deviceopen;		// Is the device open
		string device;
		int baud;
		int bits;
		int stopbits;
		char parity;

		// thread id
		// static void thread setup
	public:
		GPS();
		~GPS();
		GPSPosition GetPosition();	// Report back current position
		void Setup();			// Configure the GPS port and start getting data
		void Run();			// Run as a seperate thread to get data from the GPS
		void Stop();			// Exit the run loop

		// Should we make these bools ?
		void setDevice(string d);
		void setBaud(int b);
		void setBits(int b);
		void setStopBits(int b);
		void setParity(char p);

};

#endif /* GPS_H_ */
