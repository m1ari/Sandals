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

using namespace std;

DominoEX::DominoEX(){
	pthread_mutex_init(&m_nextString,NULL);
	counter=0;
	currString=NULL;
	nextString=NULL;
	baud=0;
	baud_delay=0;
	enPin=0;
	dataPin=0;
	dominoex_run=false;
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
		SetupPWM();
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

// We should move the CRC16 functions into a seperate library as this is the same as the RTTY code
unsigned short int DominoEX::crc16(char *data){
	int firstchar=0;

	// Get first non $
	while(data[firstchar]=='$'){
		firstchar++;
	}

	unsigned short int crc=0xFFFF;
	unsigned int i;
	for (i=firstchar; i<strlen(data); i++){
		crc=crc16_update(crc, data[i]);
	}
	return(crc);
}

unsigned short int DominoEX::crc16_update(unsigned short int crc, char c){
	int i;
	crc = crc ^ ((unsigned short int)c << 8);
	for (i=0; i<8; i++) {
		if (crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}
	return crc;
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
	delayMicroseconds(64000); // 64mS per tone for 15.625 baud
}

void DominoEX::txbyte(char byte, int sec){
	unsigned char *nibble;

	nibble=varicode[byte + ((sec) ? 256 : 0)];

	//Always send the first nibble
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

void DominoEX::setBaud(int in){
	// Code currently fixed for DominoEX16, would be nice to calculate the values to allow other speeds
	baud=in;
	//baud_delay=(1000000/in)*0.975;
	baud_delay=(1000000/in);
	syslog(LOG_NOTICE,"DominoEX: Set Baud to %d (%duS)",baud,baud_delay); 
}

void DominoEX::setEnablePin(int pin){
	enPin=pin;
	syslog(LOG_NOTICE,"DominoEX: Set enable pin to %d",enPin); 
}

void DominoEX::setDataPin(int pin){
	dataPin=pin;
	syslog(LOG_NOTICE,"DominoEX: Set data pin to %d",dataPin); 
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
