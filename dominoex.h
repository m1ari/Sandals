#ifndef DominoEX_H_
#define DominoEX_H_

#include <queue>
#include <string>

class DominoEX {
	private:
		char *currString;	// The string we're currently sending - updated with mutex
		char *nextString;	// Next string to send (updated by external)
		int baud;		// Baud rate to send data at
		int baud_delay;		// Time between nibbles for DominoEX
		int tone_rate;		// Tone spacing for DominoEX
		int enPin;		// GPIO used for the NTX2 EN pin
		int dataPin;		// GPIO used for the NTX2 TXD pin
		int counter;		// Telemetry Counter

		void txnibble(int bit);
		void txbyte(char byte,int sec);
		unsigned short int crc16(char *data);
		unsigned short int crc16_update(unsigned short int crc, char c);

		pthread_t threadid;
		pthread_mutex_t m_nextString;
		bool dominoex_run;
		static void* entryPoint(void *pthis);
		void dominoexThread();

		volatile unsigned *gpio;	// Memory access for GPIO
		volatile unsigned *pwm;		// Memory access for PWM
		volatile unsigned *clk;		// Memory access for Clock
	public:
		DominoEX();
		~DominoEX();
		void setBaud(int baud);		// Set the baud rate to send at
		void setEnablePin(int pin);	// Set the GPIO connected to the NTX2 EN pin
		void setDataPin(int pin);	// Set the GPIO connected to the NTX2 TXD pin
		void sendString(char *string);	// Used for telem strings - Will overwrite previously stored strings
		int getCounter();		// Return the next counter value to use in a sentence
		void Run();			// Run as a seperate thread to get data from the GPS
		void Stop();			// Exit the run loop
};

#endif /* DominoEX_H_ */
