#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "gpio.h"

// Blantantly taken from WiringPi
void delayMicrosecondsHard (unsigned int howLong) {
	struct timeval tNow, tLong, tEnd ;

	gettimeofday (&tNow, NULL) ;
	tLong.tv_sec  = howLong / 1000000 ;
	tLong.tv_usec = howLong % 1000000 ;
	timeradd (&tNow, &tLong, &tEnd) ;

	while (timercmp (&tNow, &tEnd, <))
		gettimeofday (&tNow, NULL) ;
}
// Read from the system Timer (See BCM2835 Library)

void delayMicroseconds (unsigned int howLong) {
	struct timespec sleeper ;
	unsigned int uSecs = howLong % 1000000 ;
	unsigned int wSecs = howLong / 1000000 ;

	/**/ if (howLong ==   0)
		return ;
	else if (howLong  < 100)
		delayMicrosecondsHard (howLong) ;
	else {
		sleeper.tv_sec  = wSecs ;
		sleeper.tv_nsec = (long)(uSecs * 1000L) ;
		nanosleep (&sleeper, NULL) ;
	}
}

void domex_tone_bb(int tone){
	// BitBang
	// This get's us close but not quite close enough.
	int i=0;
	int spacer;
	if (tone>0){
		spacer=4224/(tone*10);
	} else {
		spacer=42240;
	}
	for (i=0; i<42240; i++){	// 4224 should be the magic number for domex16 tones. (This is 10x the pwm rate for 
		if ((i % spacer) == 0) {
			GPIO_SET = 1<<18;
		} else {
			GPIO_CLR = 1<<18;
		}
		delayMicroseconds(1); // Ideally we need to delay for ~1.5 uS - Not easily possible though :(
	}
}
void domex_tone(int tone){
	// Total length of this function should be 1/15.625s
	printf("DomEX: Sending tone %d\n", tone);
	//domex_tone_bb(tone);	// Used for Bit Bang Method


	// 32 bits = 2 milliseconds, init with 1 millisecond
	//setServo(0);

	// PWM
	// Set tone
	// delayMicroseconds(64000);
	*(pwm + PWM0_DATA) = tone;
	delayMicroseconds(42240); // Ideally we need to delay for ~1.5 uS - Not easily possible though :(
}

void domex_nibble(int nibble){
	static int currtone=0;
	currtone+=nibble;
	currtone+=2;
	if (currtone>=18){
		currtone-=18;
	}
	domex_tone(currtone);
}

void domex_txbyte(char byte){


	// Lookup the nibbles for this character

	// A
	domex_nibble(3);
	domex_nibble(9);
	// a
	domex_nibble(4);
	// a
	domex_nibble(4);
}

void sig_handler(int sig){
	printf("Got Signal %d\n",sig);
	switch (sig){
		case SIGINT:
			// Interupt (Ctrl c From command line) - Graceful shutdown
			printf("Sig: Got SIGINT - Shutting down\n");
			//systemloop=false;
			GPIO_CLR = 1<<23;
		break;
		case SIGALRM:
			printf("Sig: Got SIGALRM - Next Tone\n");
		break;
	}
}

void SetupPWM(){
	uint32_t pwm_control;

//SetMode (Balanced)
	// start PWM0
	*(pwm + PWM_CONTROL) = PWM0_ENABLE;			// Balanced mode
	//*(pwm + PWM_CONTROL) = PWM0_MS_MODE|PWM0_ENABLE;	// Mark Space mode (High for Data, then Low until Range)
	//*(pwm + PWM_CONTROL) = PWM0_SERIAL|PWM0_ENABLE;	// Serializer mode

// setRange (1024)
	// filled with 0 for 20 milliseconds = 320 bits
	*(pwm + PWM0_RANGE) = 320;	// 1024 in WiringPi
	delayMicroseconds(10);

//setClock(32)	- 19.2 /32 = 600khz
	// Preserve Current Control
	pwm_control = *(pwm + PWM_CONTROL) ;

	// Stop PWM Control (Needed for MS Mode)
	*(pwm + PWM_CONTROL) = 0 ;

	// stop clock and waiting for busy flag doesn't work, so kill clock
	*(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x01;		//(1 << 5);
	delayMicroseconds(110);  

	// Wait for clock to be !BUSY
	while ((*(clk + PWMCLK_CNTL) & 0x80) != 0)
		delayMicroseconds (1);

	// set frequency
	// DIVI is the integer part of the divisor
	// the fractional part (DIVF) drops clock cycles to get the output frequency, bad for servo motors
	// 320 bits for one cycle of 20 milliseconds = 62.5 us per bit = 16 kHz
	int idiv = 32;	//(int) (19200000.0f / 16000.0f);
	if (idiv < 1 || idiv > 0x1000) {
		printf("idiv out of range: %x\n", idiv);
		exit(-1);
	}
	*(clk + PWMCLK_DIV)  = 0x5A000000 | (idiv<<12);
	
	// Start PWM Clock
	*(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x11;

	// Restore PWM Settings
	*(pwm + PWM_CONTROL) = pwm_control;

}


int main(int argc, char **argv) {
	//int rep;

	signal(SIGINT,sig_handler);
	signal(SIGALRM,sig_handler);

	// Set up gpi pointer for direct register access
	setup_io();

	// GPIO23 - NTX2 enable
	INP_GPIO(23); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(23);

	// GPIO40 and GPIO45 are audio and also support PWM	Connect to analogue audio circitury via R21 and R27
	// GPIO18 - NTX2 Data	
	// Set GPIO18 to PWM Function
	INP_GPIO(18); // must use INP_GPIO before we can use OUT_GPIO or SET_GPIO_ALT

	//OUT_GPIO(18);	// For BitBang
	SET_GPIO_ALT(18, 5);	// For PWM
	delayMicroseconds(110);
	SetupPWM();

/*
	// Setup timer
	struct itimerval new,old;

	new.it_interval.tv_usec = 0;
	new.it_interval.tv_sec = 0;
	new.it_value.tv_usec = 10;
	new.it_value.tv_sec = 0;
	setitimer (ITIMER_REAL, &new, &old);
*/

	GPIO_SET = 1<<23;	// Enable NTX2b

	//char send[]="I'm growing a beard\n";
	//char string[]="AaAaAaAaAaAa";
	//char next[]="BbBbBbBbBbBb";

	int i;
	//for (i=0; i<strlen(send); i++){
	for (i=0; i<=20; i++){
		domex_txbyte('a');	// Currently this is Hard coded to send Aa
	}

	GPIO_CLR = 1<<23;	// Disable NTX2b
	return 0;

}




