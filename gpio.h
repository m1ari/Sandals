// I/O access
//volatile unsigned *gpio;
//volatile unsigned *pwm;
//volatile unsigned *clk;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g)       *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))	// Set bits to 0 Input
#define OUT_GPIO(g)       *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+((g)/10)) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

// Defines for managing PWM (Taken from WiringPi)
#define PWM_CONTROL 0
#define PWM_STATUS  1
#define PWM0_RANGE  4
#define PWM0_DATA   5
#define PWM1_RANGE  8
#define PWM1_DATA   9

#define PWMCLK_CNTL 40
#define PWMCLK_DIV  41

#define PWM0_MS_MODE    0x0080  // Run in MS mode
#define PWM0_USEFIFO    0x0020  // Data from FIFO
#define PWM0_REVPOLAR   0x0010  // Reverse polarity
#define PWM0_OFFSTATE   0x0008  // Ouput Off state
#define PWM0_REPEATFF   0x0004  // Repeat last value if FIFO empty
#define PWM0_SERIAL     0x0002  // Run in serial mode
#define PWM0_ENABLE     0x0001  // Channel Enable

#define PWM1_MS_MODE    0x8000  // Run in MS mode
#define PWM1_USEFIFO    0x2000  // Data from FIFO
#define PWM1_REVPOLAR   0x1000  // Reverse polarity
#define PWM1_OFFSTATE   0x0800  // Ouput Off state
#define PWM1_REPEATFF   0x0400  // Repeat last value if FIFO empty
#define PWM1_SERIAL     0x0200  // Run in serial mode
#define PWM1_ENABLE     0x0100  // Channel Enable

//BCM Magic
#define BCM_PASSWORD            0x5A000000

extern "C" {
	volatile unsigned *mapRegisterMemory(int base);
	//void setup_io();
	void delayMicrosecondsHard (unsigned int howLong);
	void delayMicroseconds (unsigned int howLong);
	void SetupPWM();
	void setPWMRange(volatile unsigned *pwm, unsigned int range);
}

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

