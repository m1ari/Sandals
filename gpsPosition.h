#ifndef GPSPOSITION_H_
#define GPSPOSITION_H_

class GPSPosition {
	private:
		unsigned int hour;
		unsigned int minute;
		unsigned int second;
		time_t unixtime;
		float latitude;
		float longitude;
		float altitude;
		int sats;
		bool gotfix;
		int time;
	public:
		GPSPosition();
		~GPSPosition();
		void setTime(char *time);
		void setLatitude(float lat);
		void setLatitude(char *lat, char *ind);
		void setLongitude(float lon);
		void setLongitude(char *lon, char *ind);
		void setAltitude(float alt);
		void setAltitude(char *alt, char* unit);
		void setSats(int sats);
		void setFix(int fix);
		void clearAll();
		void setUnixTime(char *date, char *time);
		void setSpeed(char *speed);
		void setCourse(char *course);
		float getLatitude();
		float getLongitude();
		float getAltitude();
		time_t getUnixTime();
		time_t getTime();
		struct tm *getTime(struct tm *time);
};
#endif /* GPSPOSITION_H_ */
