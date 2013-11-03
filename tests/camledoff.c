#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include "gpio.h"

int main(int argc, char **argv) {

	// Set up gpi pointer for direct register access
	setup_io();

	// GPIO5 - Camera LED
	INP_GPIO(5); // must use INP_GPIO before we can use OUT_GPIO
	OUT_GPIO(5);


	GPIO_SET = 0<<5;

	return 0;

}
