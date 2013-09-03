#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <syslog.h>
#include <pthread.h>
#include <malloc.h>
#include <cstring>
#include <cstdlib>
#include <time.h>

#include "gpsPosition.h"

using namespace std;

GPSPosition::GPSPosition(){
	clearAll();
}

GPSPosition::~GPSPosition(){
}

void GPSPosition::clearAll(){
	latitude=0;
	longitude=0;
	altitude=0;
	sats=0;
	gotfix=false;
}
void GPSPosition::setTime(char *time){
	// Check the string looks like a time we can parse
	hour=(time[0]-'0') * 10;
	hour=(time[1]-'0') * 1;
	minute=(time[2]-'0') * 10;
	minute=(time[3]-'0') * 1;
	second=(time[4]-'0') * 10;
	second=(time[5]-'0') * 1;
}

void GPSPosition::setLatitude(float lat){
	latitude=lat;
	//syslog(LOG_INFO,"GPSPosition: Latitude %f",lat);
}
void GPSPosition::setLatitude(char *lat, char *ind){
	float result=0;
	
	// TODO Check position of . to see if lat is valid
	if (strlen(ind)==1) {
		// lat is in form DDMM.MMMMM
		result+=(lat[0]-'0')*10;
		result+=(lat[1]-'0');
		result+=(strtof(&lat[2],NULL) /60);

		// ind will be N or S, multiply be -1 if S
		if (strcmp(ind,"S")==0){
			result*=-1;
		}
		setLatitude(result);
		//printf("Latitude %s %s -> %f\n",lat,ind,result);
	} else {
		syslog(LOG_ERR,"GPSPosition: N/S Indicator is too long (%s)",ind);
	}
}
void GPSPosition::setLongitude(float lon){
		longitude=lon;
		//syslog(LOG_INFO,"GPSPosition: Longitude %f",lon);
}
void GPSPosition::setLongitude(char *lon, char *ind){
	float result=0;
	
	// TODO Check position of . to see if lon is valid
	if (strlen(ind)==1) {
		// lon is in form DDDMM.MMMMM
		result+=(lon[0]-'0')*100;
		result+=(lon[1]-'0')*10;
		result+=(lon[2]-'0');
		result+=(strtof(&lon[3],NULL) /60);

		// ind will be E or W, multiply be -1 if W
		if (strcmp(ind,"W")==0){
			result*=-1;
		}
		//printf("Longitude %s %s -> %f\n",lon,ind,result);
		setLongitude(result);
	} else {
		syslog(LOG_ERR,"GPSPosition: E/W Indicator is too long (%s)",ind);
	}

}
void GPSPosition::setAltitude(float alt){
		altitude=alt;
		//syslog(LOG_INFO,"GPSPosition: Altitude %f",alt);
}
void GPSPosition::setAltitude(char *alt, char *unit){
	float result=0;
	
	if (strcmp(unit,"M")==0){
		result=strtof(alt,NULL);
	} else {
		syslog(LOG_ERR,"GPSPosition: Altitude Unit not recognised (%s)",unit);
	}
	setAltitude(result);
}
void GPSPosition::setSats(int s){
	sats=s;
}
void GPSPosition::setSats(char* s){
	int i=0;
	i=atoi(s);
	setSats(i);
}
void GPSPosition::setFix(bool f){
	gotfix=f;
}
void GPSPosition::setFix(char* f){
	if (f[0]=='0'){
		setFix(false);
	} else if ((f[0]=='1') || (f[0]='2')){
		setFix(true);
	} else {
		syslog(LOG_ERR,"GPS: GPGGA Invalid fix value %s",f);
		setFix(false);
	}
}
void GPSPosition::setUnixTime(char *date, char *time){
	//Date DDMMYY
	//Time HHMMSS
	struct tm t;
	t.tm_sec=(time[4]-'0') * 10;
	t.tm_sec+=(time[5]-'0') * 1;
	t.tm_min=(time[2]-'0') * 10;
	t.tm_min+=(time[3]-'0') * 1;
	t.tm_hour=(time[0]-'0') * 10;
	t.tm_hour+=(time[1]-'0') * 1;

	t.tm_mday=(date[0]-'0') * 10;
	t.tm_mday+=(date[1]-'0') * 1;
	t.tm_mon=(date[2]-'0') * 10;
	t.tm_mon+=(date[3]-'0') * 1;
	t.tm_mon-=1;
	t.tm_year=100;
	t.tm_year+=(date[4]-'0') * 10;
	t.tm_year+=(date[5]-'0') * 1;

	t.tm_wday=0;	// Unused
	t.tm_yday=0;	// Unused
	t.tm_isdst=0;	// Time is UTC
	unixtime=mktime(&t);
}
void GPSPosition::setSpeed(char *speed){
}
void GPSPosition::setCourse(char *course){
}

float GPSPosition::getLatitude(){
	return latitude; 
}
float GPSPosition::getLongitude(){
	return longitude; 
}
float GPSPosition::getAltitude(){
	return altitude; 
}
int GPSPosition::getSats(){
	return sats; 
}
bool GPSPosition::getFix(){
	return gotfix;
}
time_t GPSPosition::getUnixTime(){
	return unixtime;
}

time_t GPSPosition::getTime(){
	struct tm time;
	time.tm_mday=0;
	time.tm_mon=0;
	time.tm_year=0;
	time.tm_hour=hour;
	time.tm_min=minute;
	time.tm_sec=second;
	time.tm_isdst=0;
	return mktime(&time);
	// TODO Doesn't return a valid value :S
}

struct tm *GPSPosition::getTime(struct tm *time){
	time->tm_mday=0;
	time->tm_mon=0;
	time->tm_year=0;
	time->tm_hour=hour;
	time->tm_min=minute;
	time->tm_sec=second;
	time->tm_isdst=0;
	return time;
}
