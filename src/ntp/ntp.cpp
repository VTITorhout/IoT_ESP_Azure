#include "ntp.h"
#include "./../time/Time.h"

time_t lastNtpUpdate=0;
NTP_STATE ntpSetup = AWAIT_INIT;

uint8_t ntpWorker(){
	switch(ntpSetup){
		case AWAIT_INIT:		//no setup, requesting time
								configTime(0, 0, "pool.ntp.org", "time.nist.gov");
								Serial.println("NTP: \tRequesting time");
								//ntpSetup++;
								ntpSetup = SET_ENVIROMENT;
								break;
		case SET_ENVIROMENT:	//requesting time done, set enviroment
								setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
								tzset(); // save the TZ variable
								//ntpSetup++;
								ntpSetup = CHECK_VALID_TIME;
								break;
		case CHECK_VALID_TIME:	//time requested, await sync
								if(time(nullptr) > 8 * 3600 * 2){
									//got valid time
									lastNtpUpdate = time(nullptr);
									struct tm timeinfo;
									localtime_r(&lastNtpUpdate, &timeinfo);
									Serial.print("NTP:\tGot time: ");
									Serial.print(asctime(&timeinfo));
									//ntpSetup++;
									setTime(lastNtpUpdate);
									ntpSetup = SYNCHRONIZED;
								}
								break;
		case SYNCHRONIZED:		//got valid time, check if drift becomes too big
								if((lastNtpUpdate+(NTP_UPDATE*60))<time(nullptr)){
									Serial.println("NTP:\tSynchronizing");
									configTime(0, 0, "pool.ntp.org", "time.nist.gov");
									lastNtpUpdate = time(nullptr);
								}
								break;
		default:				//should never come here (flowthrough of case 3)
								break;
	}
	return ntpSetup;
}