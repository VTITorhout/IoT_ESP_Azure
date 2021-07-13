#ifndef _NTP_H
#define _NTP_H

#include <Arduino.h>

#define NTP_UPDATE	60		//minutes to pass before updating NTP clock

typedef enum {
	AWAIT_INIT,
	SET_ENVIROMENT,
	CHECK_VALID_TIME,
	SYNCHRONIZED
} NTP_STATE;

//public funtioncs
uint8_t ntpWorker();	//to keep track of time

#endif // _NTP_H