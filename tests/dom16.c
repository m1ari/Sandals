//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013


// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
// #include <util/crc16.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;


// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

void setup_io();

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
	int c;
	struct timespec wt;
	wt.tv_sec=0;
	wt.tv_nsec=50;
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
	int rep;

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

	for (rep=0; rep<8; rep++) {
		int i;
	//	for (i=0; i<strlen(send); i++){
			domex_txbyte(send[i]);
	//	}
	}
	GPIO_CLR = 1<<23;
	return 0;

}


//
// Set up a memory regions to access GPIO
//

void setup_io()
{
	/* open /dev/mem */
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("can't open /dev/mem \n");
		exit(-1);
	}

	/* mmap GPIO */
	gpio_map = mmap(
		NULL,						//Any adddress in our space will do
		BLOCK_SIZE,				//Map length
		PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		MAP_SHARED,				//Shared with other processes
		mem_fd,					//File to map
		GPIO_BASE				//Offset to GPIO peripheral
	);

	close(mem_fd); //No need to keep mem_fd open after mmap

	if (gpio_map == MAP_FAILED) {
		printf("mmap error %d\n", (int)gpio_map);//errno also set!
		exit(-1);
	}

	// Always use volatile pointer!
	gpio = (volatile unsigned *)gpio_map;


} // setup_io
