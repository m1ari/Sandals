#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <syslog.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>

#include "gps.h"
#include "gpsPosition.h"

using namespace std;

GPS::GPS(){
	uart0_filestream = -1;
	gps_run=true;
}

GPS::~GPS(){
}

GPSPosition GPS::GetPosition(){
	syslog(LOG_NOTICE,"GPS: Getting Position");
	GPSPosition n;
	n=Position;
	// Get mutex
	// Grab positions
	// Release Mutex
	// Return
	return(n);
}

void GPS::Setup(){
	syslog(LOG_NOTICE,"GPS: Setup");

	//Open in non blocking read/write mode
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (uart0_filestream == -1) {
		syslog(LOG_ERR,"GPS: Unable to open UART.");
	}

	struct termios options;
	tcgetattr(uart0_filestream, &options);
	//options.c_cflag = B4800 | CS8 | CLOCAL | CREAD	//<Set baud rate
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR | IGNCR; 			//ICRNL;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
}

void GPS::Stop(){
	syslog(LOG_NOTICE,"GPS: Stopping Thread");
	gps_run=false;
	pthread_join(threadid,NULL);
	syslog(LOG_NOTICE,"GPS: Thread Stopped");
}

void GPS::Run(){
	pthread_create(&threadid,NULL,&GPS::entryPoint,this);
}
void* GPS::entryPoint(void *pthis){
	GPS *self= (GPS*)pthis;
	self->parserThread();
	pthread_exit(0);
}


void GPS::parserThread(){
	syslog(LOG_NOTICE,"GPS: Thread starting");
	char *gps_str;
	int gps_str_len=10;
	int gps_str_pos=0;
	gps_str=(char*)malloc(sizeof(char) * gps_str_len);
	if (gps_str==NULL){
		syslog(LOG_ERR,"GPS(parserThread): Unable to allocate memory for GPS parsing");
		exit(-1);
	}

	memset(gps_str,0,gps_str_len);

	while (gps_run){
		if (uart0_filestream != -1) {
			unsigned char rx_char;
			int rx_length = read(uart0_filestream, (void*)&rx_char, 1);
			if (rx_length == 1) {
				if (rx_char=='\n'){
					//Got to end of line - Parse it
					// TODO Debug levels
					//syslog(LOG_INFO,"GPS: Got String: %s",gps_str);
					if (checkChecksum(gps_str)){
						if (strncmp(gps_str,"$GPGGA",6)==0){
							nmeaGPGGA(gps_str);
						} else if(strncmp(gps_str,"$GPRMC",6)==0){
							nmeaGPRMC(gps_str);
						}

					}

					// Clear string
					memset(gps_str,0,gps_str_len);
					gps_str_pos=0;
				} else {
					// Read characters from GPS
					if (gps_str_pos+1>=gps_str_len){
						int newlen=gps_str_len+10;
						char *newstr=(char*)realloc(gps_str,newlen);
						if (newstr != NULL){
							memset(&newstr[gps_str_len],0,newlen-gps_str_len);
							gps_str=newstr;
							gps_str_len=newlen;
						} else {
							syslog(LOG_ERR,"GPS: Unable to allocate more memory");
						}
					}
					gps_str[gps_str_pos++]=rx_char;
				}
			}
		}
	}
	free(gps_str);
	syslog(LOG_NOTICE,"GPS: Thread Ending");
}

void GPS::nmeaGPRMC(char *nmea){
	// $GPRMC,192531.000,A,5056.6446,N,00124.3473,W,0.93,73.99,020813,,*28
	char *tempmsg=(char*)malloc(sizeof(char) * (strlen(nmea)+1));
	if (tempmsg!=NULL){
		char *ptempmsg=tempmsg;	// Store the original start address of tempmsg so we can free it later
		strcpy(tempmsg,nmea);
		char *token=strsep(&tempmsg,",*");
		if (token != NULL){
			if (strcmp(token,"$GPRMC")==0){	// Test we really have a GPRMC string
				//syslog(LOG_INFO,"GPS(nmeaGPRMC): Decoding GPRMC String: %s",nmea);
				int state=0;
				char *gps_time=NULL;
				//char pos[11]; // Copes to 4dp dddmm.mmmm
				while (token != NULL){
					switch (state++){
						case 1:	// Time
							gps_time=(char*)malloc(sizeof(char) * (strlen(token)+1));
							if (gps_time!=NULL){
								strcpy(gps_time,token);
							}
						break;
						//case 2:	// Status
						//case 3:	// Lat
						//case 4:	// N/S Indicator
						//case 5:	// Long
						//case 6:	// E/W Indicator
						case 7:	//	Speed over Ground
								Position.setSpeed(token);
						break;
						case 8:	//	Course over Ground
								Position.setCourse(token);
						break;
						case 9:	//	UTC Date
							// Set date
							if (gps_time!=NULL){
								Position.setUnixTime(token,gps_time);
							}
						break;
						//case 10:	// Magnetic Variation
						//case 11:	// E/W Indicator
						//case 12:	// CSum
					}
					token=strsep(&tempmsg,",*");
				}	// Close te While loop
				if (gps_time!=NULL){
					free(gps_time);
				}
				free(ptempmsg);
			} else {
				syslog(LOG_ERR,"GPS(nmeaGPRMC): Not a valid string %s",nmea);
			}
		} else {
			syslog(LOG_ERR,"GPS(nmeaGPRMC): Couldn't get token: %s",nmea);
		}
	} else {
		syslog(LOG_ERR,"GPS(nmeaGPRMC): Couldn't aloocate memory to parse the string");
	}

}
void GPS::nmeaGPGGA(char *nmea){
	// $GPGGA,212748.000,5056.6505,N,00124.3531,W,2,07,1.8,102.1,M,47.6,M,0.8,0000*6B
	char *tempmsg=(char*)malloc(sizeof(char) * (strlen(nmea)+1));
	if (tempmsg!=NULL){
		char *ptempmsg=tempmsg;	// Store the original start address of tempmsg so we can free it later
		strcpy(tempmsg,nmea);
		char *token=strsep(&tempmsg,",*");
		if (token != NULL){
			if (strcmp(token,"$GPGGA")==0){	// Test we really have a GPGGA string
				//syslog(LOG_INFO,"GPS(nmeaGPGGA): Decoding GPGGA String: %s",nmea);
				int state=0;
				char pos[11]; // Copes to 4dp dddmm.mmmm
				while (token != NULL){
					switch (state++){
						case 1:	// Time
							Position.setTime(token);
						break;
						case 2:	// Latitude Position
							strncpy(pos,token,sizeof(pos));
						break;
						case 3:	// Latitude N/S Indicator
							Position.setLatitude(pos,token);
						break;
						case 4:	// Longitude Position
							strncpy(pos,token,sizeof(pos));
						break;
						case 5:	// Longitude E/W Indicator
							Position.setLongitude(pos,token);
						break;
						case 6:	// Lock
							Position.setFix(token);
						break;
						case 7:	// Sats
							Position.setSats(token);
							// 07
						break;
						// case 8:	// Horizontal dilution of Precision
						case 9:	// Altitude
							strncpy(pos,token,sizeof(pos));
						break;
						case 10: 	// Altitude Units
							Position.setAltitude(pos,token);
						break;
						// case 11:	// Geodial Seperation
						// case 12:	// Units of GS
						// case 13:	// Age of Differential Corrections
						// case 14:	// Differential Reference staton ID
						// case 15:	// Checksum
					}
					token=strsep(&tempmsg,",*");
				}	// Close te While loop
				free(ptempmsg);
			} else {
				syslog(LOG_ERR,"GPS(nmeaGPGGA): Not a valid string %s",nmea);
			}
		} else {
			syslog(LOG_ERR,"GPS(nmeaGPGGA): Couldn't get token: %s",nmea);
		}
	} else {
		syslog(LOG_ERR,"GPS(nmeaGPGGA): Couldn't aloocate memory to parse the string");
	}
}

bool GPS::checkChecksum(char *nmea){
	int nmea_len=strcspn(nmea,"*");
	int csum=0;
	for (int i=1; i<nmea_len; i++){
		csum ^= nmea[i];
	}
	int nmea_csum=strtol(&nmea[nmea_len+1],NULL,16);
	if (csum==nmea_csum){
		return(true);
	}else{
		syslog(LOG_ERR,"GPS: sentence didnt pass checksum - %s (%02X==%02X)",nmea,nmea_csum,csum);
		return(false);
	}
}
