#ifndef RTTY_H_
#define RTTY_H_

#include <queue>
#include <string>

class RTTY {
	private:
		char *currString;	// The string we're currently sending - updated with mutex
		char *nextString;	// Next string to send (updated by external)
		std::queue<std::string> queue;	// Queue for strings to send (SSDV)
		int baud;		// Baud rate to send data at
		int baud_delay;		// time between bits for chosen baud rate
		int stopbits;		// Number of Stop bits to use (2 is good, 1 is possible)
		int bits;		// Number of Bits to send (7 is good for telem, 8 needed for SSDV)
		int enPin;		// GPIO used for the NTX2 EN pin
		int dataPin;		// GPIO used for the NTX2 TXD pin
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
		void setBaud(int baud);		// Set the baud rate to send at
		void setEnablePin(int pin);	// Set the GPIO connected to the NTX2 EN pin
		void setDataPin(int pin);	// Set the GPIO connected to the NTX2 TXD pin
		void setStopBits(int bits);	// Set number of stop bits to send
		void setBits(int bits);		// Set number of bits to send
		void sendString(char *string);	// Used for telem strings - Will overwrite previously stored strings
		void queueString(char *string);	// Used for SSDV - Will build a queue of strings to send
		void queuePriority(int pri);	// How many SSDV packets to send per telem
		int getQueueSize();		// Get size of queue - used to only add new data to small queues
		int getCounter();		// Return the next counter value to use in a sentence
		void Run();			// Run as a seperate thread to get data from the GPS
		void Stop();			// Exit the run loop
};

#endif /* RTTY_H_ */
