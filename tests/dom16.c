#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "gpio.h"
// #include <util/crc16.h>

//https://projects.drogon.net/accurate-delays-on-the-raspberry-pi/
/*
void delayMicrosecondsHard (unsigned int howLong) {
	*(timer + TIMER_LOAD)    = howLong ;
	*(timer + TIMER_IRQ_CLR) = 0 ;
	while (*timerIrqRaw == 0)
		;
}
*/

// http://raspberrypi.stackexchange.com/questions/3898/nanosleep-wont-sleep-short-time
void udelay(long us){
	struct timeval current;
	struct timeval start;

	gettimeofday(&start, NULL);
	do {
		gettimeofday(&current, NULL);
	} while( ( current.tv_sec*1000000 + current.tv_usec ) - ( start.tv_sec*1000000 + start.tv_usec ) < us );
}

void domex_tone(int tone){
	// Total length of this function should be 1/15.625s
	printf("DomEX: Sending tone %d\n", tone);

	int i=0;
	int spacer;
	if (tone>0){
		spacer=4224/(tone*10);
	} else {
		spacer=42240;
	}
	//int c;
	//struct timespec wt;
	//wt.tv_sec=0;
	//wt.tv_nsec=50;
	//for (c=0; c<10; c++){
		for (i=0; i<42240; i++){	// 4224 should be the magic number for domex16 tones. (This is 10x the pwm rate for 
			if ((i % spacer) == 0) {
				GPIO_SET = 1<<18;
			} else {
				GPIO_CLR = 1<<18;
			}
			udelay(1);
		}
	//}
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


int main(int argc, char **argv) {
	//int rep;

	// Set up gpi pointer for direct register access
	setup_io();

	// GPIO23 - NTX2 enable
	INP_GPIO(23); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(23);
	// GPIO18 - NTX2 Data	
	INP_GPIO(18); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(18);


	GPIO_SET = 1<<23;

	char send[]="I'm growing a beard\n";

	//for (rep=0; rep<8; rep++) {
		int i;
		for (i=0; i<strlen(send); i++){
			domex_txbyte(send[i]);
		}
	//}
	GPIO_CLR = 1<<23;
	return 0;

}

