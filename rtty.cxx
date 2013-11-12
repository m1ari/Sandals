#include <cstdio>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sched.h>
#include "rtty.h"
#include "gpio.h"

using namespace std;

RTTY::RTTY(){
	pthread_mutex_init(&m_nextString,NULL);
	counter=0;
	currString=NULL;
	nextString=NULL;
	baud=0;
	baud_delay=0;
	stopbits=0;
	bits=0;
	enPin=0;
	dataPin=0;
	rtty_run=false;
}

RTTY::~RTTY(){
	pthread_mutex_destroy(&m_nextString);
}

void RTTY::Stop(){
	syslog(LOG_NOTICE,"RTTY: Stopping Thread");
	rtty_run=false;
	pthread_join(threadid,NULL);

	// Disable Radio
	GPIO_CLR = 1<<enPin;
	syslog(LOG_NOTICE,"RTTY: Thread Stopped");
}

void RTTY::Run(){
	syslog(LOG_NOTICE,"RTTY: Creating Thread");
	
	// Configure GPIO Stuff
        gpio = mapRegisterMemory(GPIO_BASE);

	// Check it's not already running
	rtty_run=true;

	if ((enPin!=0) && (dataPin!=0)){
		// Configure the Enable Pin
		INP_GPIO(enPin); // must use INP_GPIO before we can use OUT_GPIO
		OUT_GPIO(enPin);
		// Enable the Radio
		GPIO_SET = 1<<enPin;

		// Configure the Data Pin
		INP_GPIO(dataPin); // must use INP_GPIO before we can use OUT_GPIO
		OUT_GPIO(dataPin);
		pthread_create(&threadid,NULL,&RTTY::entryPoint,this);
	} else {
		if (enPin==0){
			syslog(LOG_ERR,"RTTY: Enable Pin not Configured, Not starting RTTY");
		}
		if (dataPin==0){
			syslog(LOG_ERR,"RTTY: Data Pin not Configured, Not starting RTTY");
		}
	}
}

unsigned short int RTTY::crc16(char *data){
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

unsigned short int RTTY::crc16_update(unsigned short int crc, char c){
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

void* RTTY::entryPoint(void *pthis){
   RTTY *self= (RTTY*)pthis;
   self->rttyThread();
   pthread_exit(0);
}

void RTTY::rttyThread(){
	syslog(LOG_NOTICE,"RTTY: Thread starting");

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

	while (rtty_run){
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
					syslog(LOG_ERR,"RTTY: Unable to allocate memory for next string");
				}
			} else {
				syslog(LOG_ERR,"RTTY: no next string to send");
			}
			pthread_mutex_unlock(&m_nextString);
		}

		if (currString!=NULL){
			syslog(LOG_NOTICE,"RTTY: Sending String %s(%d)",currString,counter);
			unsigned int i;
			for (i=0; i<strlen(currString); i++){
					txbyte(currString[i]);
			}
			txbyte('\n');
			free(currString);	// Shouldn't free it
			currString=NULL;
			datasent=true;
		}


			// TODO, Check queue length and only send if the Queue is empty
			// Only do if the Queue is also empty
		if (queue.size() >0 ){
			string queuedstr=(string)queue.front();
			unsigned int i;
			for (i=0; i<queuedstr.size(); i++){
					txbyte(queuedstr[i]);
			}
			txbyte('\n');
			syslog(LOG_NOTICE,"RTTY: Sending Queued data %s",queuedstr.c_str());
			// Should check it was sent properly
			queue.pop();
			datasent=true;
		}


		if (datasent == false) {
			// If nothing was sent send some data.
			unsigned int i;
			char send[]="zzzzzzzzz";
			syslog(LOG_NOTICE,"RTTY: Sending String %s",send);

			for (i=0; i<strlen(send); i++){
					txbyte(send[i]);
			}
		}
	}
	syslog(LOG_NOTICE,"RTTY: Thread ending");
}


void RTTY::txbit(int bit){
	if (bit){
		GPIO_SET = 1<<dataPin;
	} else {
		GPIO_CLR = 1<<dataPin;
	}
	usleep(baud_delay);	// Resolution is around 100uS This should be fine up to around 1200 baud. Above that we might have issues
}

void RTTY::txbyte(char byte){
	int i;
	txbit(0); //Start bit

	for (i=0;i<bits; i++){
		if (byte & 1) {
			txbit(1);
		} else {
			txbit(0);
		}
		byte=byte>>1;
	}
	for (i=0;i<stopbits; i++){
		txbit(1); //Stop bit
	}
}

void RTTY::setBaud(int in){
	baud=in;
	//baud_delay=(1000000/in)*0.975;
	baud_delay=(1000000/in);
	syslog(LOG_NOTICE,"RTTY: Set Baud to %d (%duS)",baud,baud_delay); 
}

void RTTY::setBits(int in){
	bits=in;
	syslog(LOG_NOTICE,"RTTY: Set Bits to %d",bits); 
}

void RTTY::setStopBits(int in){
	stopbits=in;
	syslog(LOG_NOTICE,"RTTY: Set Stop Bits to %d",stopbits); 
}

void RTTY::setEnablePin(int pin){
	enPin=pin;
	syslog(LOG_NOTICE,"RTTY: Set enable pin to %d",enPin); 
}
void RTTY::setDataPin(int pin){
	dataPin=pin;
	syslog(LOG_NOTICE,"RTTY: Set data pin to %d",dataPin); 
}

void RTTY::sendString(char *send){
	int size=0;

	size=strlen(send);
	syslog(LOG_NOTICE,"RTTY: Setting String to send to %s(%d)",send,size);
	
	pthread_mutex_lock(&m_nextString);
	// TODO Would be better to keep the length and make it larger if needed
	// Need to store length for currString and nextString

	if (nextString!=NULL){
		//syslog(LOG_DEBUG,"RTTY: Freeing Next string");
		free(nextString);
	}

	nextString=(char*)malloc(sizeof(char) * (size + 1 ));
	if (nextString==NULL){
		syslog(LOG_ERR,"RTTY: Unable to allocate memory of next string");
	} else {
		memset(nextString,0,size+1);
		strcpy(nextString,send);
	}
	pthread_mutex_unlock(&m_nextString);
}


void RTTY::queueString(char *send){
	// TODO: This is probably broken for SSDV as it needs to transmit NULLS as well as other characters
	int size=0;

	size=strlen(send);
	syslog(LOG_NOTICE,"RTTY: Adding String to Queue %s(%d)",send,size);
	queue.push(send);
}
void RTTY::queuePriority(int pri){
}
int RTTY::getQueueSize(){
	return queue.size();
}
int RTTY::getCounter(){
	return counter;
}
