#include <cstdlib>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "sandals.h"
#include "gps.h"
#include "gpsPosition.h"
#include "rtty.h"
#include "dominoex.h"

using namespace std;

void sig_handler(int sig);

//volatile bool systemloop=true;
volatile sig_atomic_t systemloop=true;

int main (int argc, char **argv){
	openlog("HabTrack", LOG_CONS, LOG_LOCAL3);
	syslog(LOG_NOTICE,"HAB Tracker mk1 Starting with pid %d",getpid());

	signal(SIGHUP,sig_handler);
	signal(SIGTERM,sig_handler);
	signal(SIGUSR1,sig_handler);
	signal(SIGUSR2,sig_handler);

	signal(SIGQUIT,sig_handler);
	signal(SIGINT,sig_handler);
	signal(SIGKILL,sig_handler);

	long int counter1,counter2;

	GPS gps;
	GPSPosition location;
	RTTY ntx2;
	DominoEX ntx2b;
	//RTTY rtty;

	gps.Setup();
	gps.Run();

	ntx2.setBaud(50);
	ntx2.setBits(7);
	ntx2.setStopBits(2);
	ntx2.setEnablePin(25);
	ntx2.setDataPin(24);
	ntx2.Run();

	//ntx2b.setBaud(300);	// Not currently used for DominoEX
	ntx2b.setEnablePin(23);
	ntx2b.setDataPin(18);
	ntx2b.Run();
	//char tosend[100];
	char *tosend;
	tosend=(char*) malloc(sizeof(char) * 100);
	
	sleep(5);
	int queue=0;
	char queuestr[100];
	while (systemloop){
		// Get information from our threaded classes
		location=gps.GetPosition();

		// Silly test of different baud rate for some strings
		// Needs fix for issue2 to work
		//if ((counter % 10) ==0 ){
			//rtty.setBaud(50);
		//} else {
			//rtty.setBaud(300);
		//}

		// Format time into a useful format
		char time[9];
		struct tm *timein;
		time_t unixtime=location.getUnixTime();
		timein=gmtime(&unixtime);
		strftime(time,9,"%H:%M:%S",timein);

		// Setup flags
		int flags=0;
		if (location.getFix()){
			flags+=0x01;
		}

		// Generate telemetry string to send
		// CALL, Counter,Time, Lat, Long, Alt, Sats, Flags, CSUM (Added in rtty module)

		// Data String for Radio1
		counter1=ntx2.getCounter();
		snprintf(tosend,99,"$$$$%s,%li,%s,%f,%f,%.2f,%d,%#04x",
			callsign1,
			counter1,
			time,
			location.getLatitude(),
			location.getLongitude(),
			location.getAltitude(),
			location.getSats(),
			flags
		);
		ntx2.sendString(tosend);

		// Data String for Radio2
		counter2=ntx2b.getCounter();
		snprintf(tosend,99,"$$$$%s,%li,%s,%f,%f,%.2f,%d,%#04x",
			callsign2,
			counter2,
			time,
			location.getLatitude(),
			location.getLongitude(),
			location.getAltitude(),
			location.getSats(),
			flags
		);
		ntx2b.sendString(tosend);


		// Check the queue size and add new if needed (SSDV)
/*
		if (rtty.getQueueSize() <4){
			for (int i=0; i<10; i++){
				sprintf(queuestr, "Queue Value %d at %s", queue++, time);
				//rtty.queueString(queuestr);
			}
		}
*/
		sleep(2);
	}

	gps.Stop();
	ntx2.Stop();
	ntx2b.Stop();
	return(0);
}

void sig_handler(int sig){
	syslog(LOG_NOTICE,"Got Signal %d",sig);
	switch (sig){
		case SIGHUP:
			// Sig HUP, Do a reload
			syslog(LOG_NOTICE,"Sig: Got SIGHUP");
		break;
		case SIGINT: // 2
			// Interupt (Ctrl c From command line) - Graceful shutdown
			syslog(LOG_NOTICE,"Sig: Got SIGINT - Shutting down");
			systemloop=false;
		break;
		case 15:
			// TERM
			syslog(LOG_NOTICE,"Sig: Got SIGTERM");
		break;
		case 16:
			// USR1
			syslog(LOG_NOTICE,"Sig: Got SIGUSR1");
		break;
		case 17:
			// USR2
			syslog(LOG_NOTICE,"Sig: Got SIGUSR2");
		break;
	}
}

