#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "gpio.h"

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

void sig_handler(int sig){
	printf("Got Signal %d\n",sig);
	switch (sig){
		case SIGHUP: // Sig HUP, Do a reload
			//syslog(LOG_NOTICE,"Sig: Got SIGHUP");
		break;
		case SIGINT:
			// Interupt (Ctrl c From command line) - Graceful shutdown
			printf("Sig: Got SIGINT - Shutting down\n");
			//systemloop=false;
			GPIO_CLR = 1<<23;
		break;
		case SIGALRM:
			printf("Sig: Got SIGALRM - Next Tone\n");
		break;
		case SIGTERM:
			printf("Sig: Got SIGTERM\n");
		break;
		case SIGUSR1:
			//syslog(LOG_NOTICE,"Sig: Got SIGUSR1");
		break;
		case SIGUSR2:
			//syslog(LOG_NOTICE,"Sig: Got SIGUSR2");
		break;
	}
}

int main(int argc, char **argv) {
	//int rep;

	//signal(SIGHUP,sig_handler);
	signal(SIGTERM,sig_handler);
	//signal(SIGUSR1,sig_handler);
	//signal(SIGUSR2,sig_handler);
	signal(SIGQUIT,sig_handler);
	signal(SIGINT,sig_handler);
	signal(SIGKILL,sig_handler);
	signal(SIGALRM,sig_handler);

	// Set up gpi pointer for direct register access
	setup_io();

	// GPIO23 - NTX2 enable
	INP_GPIO(23); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(23);
	// GPIO18 - NTX2 Data	
	INP_GPIO(18); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(18);

	// Setup timer
	struct itimerval new,old;

	new.it_interval.tv_usec = 0;
	new.it_interval.tv_sec = 0;
	new.it_value.tv_usec = 10;
	new.it_value.tv_sec = 0;
	setitimer (ITIMER_REAL, &new, &old);

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




