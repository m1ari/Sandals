/* Access to GPIO Registeres on the Raspberry Pi
 * Based on example code from
 *   http://elinux.org/RPi_Low-level_peripherals#C_2
 * With Additions from
 *   http://www.frank-buss.de/raspberrypi/pwm.c
 *   http://www.raspberrypi.org/phpBB3/viewtopic.php?t=8467&p=124620
 *   http://git.drogon.net/?p=wiringPi;a=blob;f=wiringPi/wiringPi.c
 * GPIO Switching speed tests at
 *   http://codeandlife.com/2012/07/03/benchmarking-raspberry-pi-gpio-speed/
*/

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "gpio.h"

/* Addresses from:
 * 1) http://git.drogon.net/?p=wiringPi;a=blob;f=wiringPi/wiringPi.c
 * 2) http://www.raspberrypi.org/phpBB3/viewtopic.php?f=91&t=23698&p=220553
*/
//#define BCM2708_PERI_BASE			     0x20000000
//#define GPIO_BASE		(BCM2708_PERI_BASE + 0x00200000)	// 1,2	/* GPIO Controller */
//#define GPIO_PWM		(BCM2708_PERI_BASE + 0x0020C000)	// 1,2	/* PWM Controller */
//#define CLOCK_BASE		(BCM2708_PERI_BASE + 0x00101000)	// 1,2
//#define GPIO_PADS		(BCM2708_PERI_BASE + 0x00100000)	// 1
//#define GPIO_TIMER		(BCM2708_PERI_BASE + 0x0000B000)	// 1

//#define SYST_BASE		(BCM2708_PERI_BASE + 0x00003000)	// 2
//#define DMA_BASE		(BCM2708_PERI_BASE + 0x00007000)	// 2
//#define DMA15_BASE		(BCM2708_PERI_BASE + 0x00E05000)	// 2

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

volatile unsigned *mapRegisterMemory(int base){
	static int mem_fd=0;
	void *map;

	/* open /dev/mem */
	if (!mem_fd) {
		if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
			printf("can't open /dev/mem \n");
			exit(-1);
		}
	}

	/* mmap GPIO */
	map = mmap(
		NULL,
		BLOCK_SIZE,		// Map length
		PROT_READ|PROT_WRITE,	// Enable reading & writting to mapped memory
		MAP_SHARED,		// Shared with other processes
		mem_fd,			// File to map
		base			// Offset to GPIO peripheral
	);

	if (map == MAP_FAILED) {
		printf("mmap error %d\n", (int)map);	// Could also get result of errno
		exit (-1);
	}

	// Always use volatile pointer!
	return (volatile unsigned *)map;
}

/*
void setup_io(){
	gpio = mapRegisterMemory(GPIO_BASE);
	pwm = mapRegisterMemory(GPIO_PWM);
	clk = mapRegisterMemory(CLOCK_BASE);
}
*/

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

void setPWMRange(volatile unsigned *pwm, unsigned int range){
	*(pwm + PWM0_RANGE) = range;
	delayMicroseconds(10);

}


void SetupPWM(){
	uint32_t pwm_control;

	// Should pass these in from the calling function - but lets make life easy for now
	volatile unsigned *pwm;
	volatile unsigned *clk;
	pwm = mapRegisterMemory(GPIO_PWM);
	clk = mapRegisterMemory(CLOCK_BASE);

//SetMode (Balanced)
	// start PWM0
	*(pwm + PWM_CONTROL) = PWM0_ENABLE;			// Balanced mode
	//*(pwm + PWM_CONTROL) = PWM0_MS_MODE|PWM0_ENABLE;	// Mark Space mode (High for Data, then Low until Range)
	//*(pwm + PWM_CONTROL) = PWM0_SERIAL|PWM0_ENABLE;	// Serializer mode

	setPWMRange(pwm, 384);

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
	int idiv = 5;	//(int) (19200000.0f / 16000.0f);
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

