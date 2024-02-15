#include <cstdio>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sched.h>
#include "dominoex.h"
#include "dominovar.h"
#include "gpio.h"
#include "crc.h"

using namespace std;

DominoEX::DominoEX(){
	pthread_mutex_init(&m_nextString,NULL);
	dominoex_run=false;
	counter=0;
	currString=NULL;
	nextString=NULL;
	enPin=0;
	dataPin=0;
	setBaud(16);
}

DominoEX::~DominoEX(){
	pthread_mutex_destroy(&m_nextString);
}

void DominoEX::Stop(){
	syslog(LOG_NOTICE,"DominoEX: Stopping Thread");
	dominoex_run=false;
	pthread_join(threadid,NULL);

	// Disable Radio
	GPIO_CLR = 1<<enPin;
	syslog(LOG_NOTICE,"DominoEX: Thread Stopped");
}

void DominoEX::Run(){
	syslog(LOG_NOTICE,"DominoEX: Creating Thread");
	
	// Configure GPIO Stuff
        gpio = mapRegisterMemory(GPIO_BASE);
	pwm = mapRegisterMemory(GPIO_PWM);
	clk = mapRegisterMemory(CLOCK_BASE);

	// Check it's not already running
	dominoex_run=true;

	if ((enPin!=0) && (dataPin!=0)){
		// Configure the Enable Pin
		INP_GPIO(enPin); // must use INP_GPIO before we can use OUT_GPIO
		OUT_GPIO(enPin);
		// Enable the Radio
		GPIO_SET = 1<<enPin;

		// Configure the Data Pin
		INP_GPIO(dataPin); // must use INP_GPIO before we can use OUT_GPIO
		// Should check this is a pin that allows PWM
		SET_GPIO_ALT(dataPin, 5);	// For PWM
		delayMicroseconds(110);
		SetupPWM();	// TODO Seperate out parts of the function - We need to be able to change Range on the fly
		pthread_create(&threadid,NULL,&DominoEX::entryPoint,this);
	} else {
		if (enPin==0){
			syslog(LOG_ERR,"DominoEX: Enable Pin not Configured, Not starting DominoEX");
		}
		if (dataPin==0){
			syslog(LOG_ERR,"DominoEX: Data Pin not Configured, Not starting DominoEX");
		}
	}
}

void* DominoEX::entryPoint(void *pthis){
   DominoEX *self= (DominoEX*)pthis;
   self->dominoexThread();
   pthread_exit(0);
}

void DominoEX::dominoexThread(){
	syslog(LOG_NOTICE,"DominoEX: Thread starting");

	// Set Priority
	struct sched_param params;
	pthread_t this_thread = pthread_self();
	int policy;

	pthread_getschedparam(this_thread,&policy, &params);
	printf("Policy: %d, Current Pri %d (Min: %d, Max: %d)\n",policy,params.sched_priority,sched_get_priority_min(SCHED_RR),sched_get_priority_max(SCHED_FIFO));

	params.sched_priority = 5;
	pthread_setschedparam(this_thread, SCHED_FIFO, &params);

	pthread_getschedparam(this_thread,&policy, &params);
	printf("Policy: %d, Current Pri %d (Min: %d, Max: %d)\n",policy,params.sched_priority,sched_get_priority_min(SCHED_FIFO),sched_get_priority_max(SCHED_FIFO));
	// Add mlock / mlockall to prevernt swapping
	// http://www.airspayce.com/mikem/bcm2835/
	// https://rt.wiki.kernel.org/index.php/Threaded_RT-application_with_memory_locking_and_stack_handling_example

	while (dominoex_run){
		bool datasent=false;
		// TODO Future, Only run if the radio is enabled

		// TODO repeat this based on the telem/queue ratio


		// If we don't have a current string to send, get the next one.
		if (currString==NULL){
			// Should only malloc if nextString==NULL. Normall should copy strings at this point
			pthread_mutex_lock(&m_nextString);
			if (nextString!=NULL){
				currString=(char*)malloc(sizeof(char) * (strlen(nextString) + 1 + 5));
				if (currString!=NULL){
					sprintf(currString,"%s*%04X",nextString,crc16(nextString));
					counter++;
					free(nextString);
					nextString=NULL;
				} else {
					syslog(LOG_ERR,"DominoEX: Unable to allocate memory for next string");
				}
			} else {
				syslog(LOG_ERR,"DominoEX: no next string to send");
			}
			pthread_mutex_unlock(&m_nextString);
		}

		if (currString!=NULL){
			syslog(LOG_NOTICE,"DominoEX: Sending String %s(%d)",currString,counter);
			unsigned int i;
			for (i=0; i<strlen(currString); i++){
					txbyte(currString[i],0);
			}
			txbyte('\n',0);
			free(currString);	// Shouldn't free it
			currString=NULL;
			datasent=true;
		}


		// Want to change this so we send 10 bytes of secondary text at a time 
		if (datasent == false) {
			// If nothing was sent send some data.
			unsigned int i;
			char send[]="http://m1ari.co.uk/projects/pi-tracker";
			syslog(LOG_NOTICE,"DominoEX: Sending String %s",send);

			for (i=0; i<strlen(send); i++){
					txbyte(send[i],1);
			}
		}
	}
	syslog(LOG_NOTICE,"DominoEX: Thread ending");
}

void DominoEX::txnibble(int nibble){
        static int txtone=0;
        txtone+=nibble;
        txtone+=2;
        if (txtone>=18){
                txtone-=18;
        }
	*(pwm + PWM0_DATA) = txtone+100;
	//delayMicroseconds(64000); // 64mS per tone for 15.625 baud
	delayMicroseconds(baud_delay);
}

void DominoEX::txbyte(char byte, int sec){
	unsigned char *nibble;

	nibble=varicode[byte + ((sec) ? 256 : 0)];

	// Always send the first nibble
	txnibble(nibble[0]);

	// Only send the second nibble if it or the third nibble >0
	if ( (nibble[1] >0) | (nibble[2] > 0)){
		txnibble(nibble[1]);
	}

	// Only send the third byte if its >0
	if (nibble[2] > 0){
		txnibble(nibble[2]);
	}
}

// setBaud doesn't currently affect the sent baudrate - Need to change the tx_ functions
void DominoEX::setBaud(int in){
	syslog(LOG_NOTICE,"DominoEX(setBaud): Set Baud to %d",in); 
	if (dominoex_run==false){
		switch (in){
			case 4:
				baud=in;
				baud_delay=1000000/3.90625;
				tone_rate=2;
				// Set PWM Range
				syslog(LOG_NOTICE,"DominoEX(setBaud): Set Baud to %d (%duS)",baud,baud_delay); 
			break;
			case 5:
				baud=in;
				baud_delay=1000000/5.3833;
				tone_rate=2;
				// Set PWM Range
				syslog(LOG_NOTICE,"DominoEX(setBaud): Set Baud to %d (%duS)",baud,baud_delay); 
			break;
			case 8:
				baud=in;
				baud_delay=1000000/7.8125;
				tone_rate=2;
				// Set PWM Range
				syslog(LOG_NOTICE,"DominoEX(setBaud): Set Baud to %d (%duS)",baud,baud_delay); 
			break;
			case 11:
				baud=in;
				baud_delay=1000000/10.766;
				tone_rate=1;
				// Set PWM Range
				syslog(LOG_NOTICE,"DominoEX(setBaud): Set Baud to %d (%duS)",baud,baud_delay); 
			break;
			case 16:
				baud=in;
				baud_delay=1000000/15.625;
				tone_rate=1;
				// Set PWM Range
				syslog(LOG_NOTICE,"DominoEX(setBaud): Set Baud to %d (%duS)",baud,baud_delay); 
			break;
			case 22:
				baud=in;
				baud_delay=1000000/21.533;
				tone_rate=1;
				// Set PWM Range
				syslog(LOG_NOTICE,"DominoEX(setBaud): Set Baud to %d (%duS)",baud,baud_delay); 
			break;
			default:
				syslog(LOG_ERR,"DominoEX(setBaud): Invalid Baud rate %d",in); 
			break;
		}
	} else {
		syslog(LOG_ERR,"DominoEX(setBaud): Module must be stopped before changing baud rate"); 
	}
}

void DominoEX::setEnablePin(int pin){
	if (dominoex_run==false){
		enPin=pin;
		syslog(LOG_NOTICE,"DominoEX: Set enable pin to %d",enPin); 
	} else {
		syslog(LOG_ERR,"DominoEX(setEnablePin): Module must be stopped before changing the enable pin"); 
	}
}

void DominoEX::setDataPin(int pin){
	if (dominoex_run==false){
		dataPin=pin;
		syslog(LOG_NOTICE,"DominoEX: Set data pin to %d",dataPin); 
	} else {
		syslog(LOG_ERR,"DominoEX(setEnablePin): Module must be stopped before changing the enable pin"); 
	}
}

void DominoEX::sendString(char *send){
	int size=0;

	size=strlen(send);
	syslog(LOG_NOTICE,"DominoEX: Setting String to send to %s(%d)",send,size);
	
	pthread_mutex_lock(&m_nextString);
	// TODO Would be better to keep the length and make it larger if needed
	// Need to store length for currString and nextString

	if (nextString!=NULL){
		//syslog(LOG_DEBUG,"DominoEX: Freeing Next string");
		free(nextString);
	}

	nextString=(char*)malloc(sizeof(char) * (size + 1 ));
	if (nextString==NULL){
		syslog(LOG_ERR,"DominoEX: Unable to allocate memory of next string");
	} else {
		memset(nextString,0,size+1);
		strcpy(nextString,send);
	}
	pthread_mutex_unlock(&m_nextString);
}

int DominoEX::getCounter(){
	return counter;
}
