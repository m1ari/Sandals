#ifndef RTTY_H_
#define RTTY_H_

#include <queue>
#include <string>

class RTTY {
	private:
		char *currString;
		char *nextString;	// Next string to send (updated by external)
		std::queue<std::string> queue;	// Queue for strings to send (SSDV)
		int baud;
		int baud_delay;
		int stopbits;		// Number of Stop bits to use (2 is good, 1 is possible)
		int bits;			// Number of Bits to send (7 is good for telem, 8 needed for SSDV)
		int enPin;
		int dataPin;
		int counter;		// Telemetry Counter

		void txbit(int bit);
		void txbyte(char byte);
		unsigned short int crc16(char *data);
		unsigned short int crc16_update(unsigned short int crc, char c);

		pthread_t threadid;
		pthread_mutex_t m_nextString;
		bool rtty_run;
		static void* entryPoint(void *pthis);
		void rttyThread();
		void setup_io();

		int  mem_fd;
		void *gpio_map;
		// I/O access
		volatile unsigned *gpio;

	public:
		RTTY();
		~RTTY();
		void setBaud(int baud);
		void setEnablePin(int pin);
		void setDataPin(int pin);
		void setStopBits(int bits);
		void setBits(int bits);

		void sendString(char *string);	// Used for telem strings - Will overwrite previously stored strings
		void queueString(char *string);	// Used for SSDV - Will build a queue of strings to send
		void queuePriority(int pri);		// How many SSDV packets to send per telem
		int getQueueSize();					// Get size of queue - so caller can determine whether to add a new image
		int getCounter();
		void Run();								// Run as a seperate thread to get data from the GPS
		void Stop();							// Exit the run loop

};

#endif /* RTTY_H_ */
