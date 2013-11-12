/* Access to GPIO Registeres on the Raspberry Pi
 * Based on example code from
 *   http://elinux.org/RPi_Low-level_peripherals#C_2
 * With Additions from
 *   http://www.frank-buss.de/raspberrypi/pwm.c
 *   http://www.raspberrypi.org/phpBB3/viewtopic.php?t=8467&p=124620
 *   http://git.drogon.net/?p=wiringPi;a=blob;f=wiringPi/wiringPi.c
*/

/* Addresses from:
 * 1) http://git.drogon.net/?p=wiringPi;a=blob;f=wiringPi/wiringPi.c
 * 2) http://www.raspberrypi.org/phpBB3/viewtopic.php?f=91&t=23698&p=220553
*/
#define BCM2708_PERI_BASE			     0x20000000
#define GPIO_BASE		(BCM2708_PERI_BASE + 0x00200000)	// 1,2	/* GPIO Controller */
#define GPIO_PWM		(BCM2708_PERI_BASE + 0x0020C000)	// 1,2	/* PWM Controller */
#define CLOCK_BASE		(BCM2708_PERI_BASE + 0x00101000)	// 1,2
#define GPIO_PADS		(BCM2708_PERI_BASE + 0x00100000)	// 1
#define GPIO_TIMER		(BCM2708_PERI_BASE + 0x0000B000)	// 1

//#define SYST_BASE		(BCM2708_PERI_BASE + 0x00003000)	// 2
//#define DMA_BASE		(BCM2708_PERI_BASE + 0x00007000)	// 2
//#define DMA15_BASE		(BCM2708_PERI_BASE + 0x00E05000)	// 2

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "gpio.h"

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

void setup_io(){
	gpio = mapRegisterMemory(GPIO_BASE);
	pwm = mapRegisterMemory(GPIO_PWM);
	clk = mapRegisterMemory(CLOCK_BASE);
}
