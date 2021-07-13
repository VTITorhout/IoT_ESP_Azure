#include "config.h"					//contains all settings needed for network & credentials
#include "src/ntp/ntp.h"			//library for timekeeping (needed for httpS)
#include "src/azure/iot.h"			//library to access IoT central/hub on Azure

IoT azure;							//create class azure of type IoT

#if defined(ARDUINO_ARCH_ESP8266)
	#include <ESP8266WiFi.h>		//needed for MAC
#elif defined(ARDUINO_ARCH_ESP32)
	#include <WiFi.h>				//needed for MAC
#endif

#if defined USE_LAN && USE_LAN == true
	#include <SPI.h>
	#include "src/eth/ethernet.h"	//adapted version because @2.0.0 from Arduino give errors and @1.0.4 included in ESP is too old
#endif

//create enum for wired network steps
typedef enum{
	NOT_YET_INITIALIZED,	//no setup has been done
	INITIALIZED_OK,			//found Wizz hardware
	INITIALIZED_NOK,		//did not found Wizz hardware
	CABLE_DISCONNECTED,		//not connected to transport medium
	DHCP					//received IP from DHCP
} NETWORK_STATUS;

/*Next subroutine will create a char array
	--> prefix can be given, e.g. IoT
	--> prefix will be seperated from MAC with a dash
	--> last 6 hex values of MAC of device will be added to the array
*/
void ssidPrefixMac(char* buff, const char* prefix) {
  uint8_t macAddr[6];
  WiFi.softAPmacAddress(macAddr);					//get the MAC
  for (uint8_t i = 0; i < strlen(prefix); i++) {	//add the prefix to the array
    buff[i] = prefix[i];
  }
  buff[strlen(prefix)] = '-';						//add a dash as seperator sign
  for (uint8_t i = 0; i < 3; i++) {					//add the last 3 bytes of the MAC
    sprintf(&buff[strlen(prefix) + 1 + i * 2], "%02X", macAddr[i + 3]);
  }
  buff[strlen(prefix) + 7] = '\0';					//null terminated array
}

/*Next subroutine will setup the network, depending on user needs
	--> WiFi can be used on ESP8266 or ESP32
	--> Wired network can be used on ESP8266 (through Wizznet SPI)
	--> Wired network will not work on ESP32, this is not (yet) implemented
	(different libraries need to be used, because ESP32 does have a MAC)
*/
void setupNetwork() { 
	#if defined USE_LAN && USE_LAN == true	//check if we want WiFi or Wired network connection
		NETWORK_STATUS wiredStatus = NOT_YET_INITIALIZED;
		Serial.println("ETH:\tSetup");
		//SPI.pins(6, 7, 8, 0);				//switch to flash SPI0 (extended mode)
		Ethernet.init(ETH_CS);				//set CS pin --> see config.h
		uint8_t macAddr[6];
		WiFi.softAPmacAddress(macAddr);		//will use MAC of WiFi for Wired
		//Try to get IP from DHCP
		//If failed, check why?
		while (!Ethernet.begin(macAddr, 1000, 1000)) {	//begin DHCP request with timeout of 1 second --> blocking code!
			if (wiredStatus==NOT_YET_INITIALIZED) {
				switch (Ethernet.hardwareStatus()) {
					case 0:	Serial.println("\tNo hardware detected.");
							wiredStatus = INITIALIZED_NOK; //no hardware found
							break;
					case 1:	Serial.println("\tWizNet W5100 detected.");
							wiredStatus = INITIALIZED_OK; //init passed
							break;
					case 2:	Serial.println("\tWizNet W5200 detected.");
							wiredStatus = INITIALIZED_OK; //init passed
							break;
					case 3: Serial.println("\tWizNet W5500 detected.");
							wiredStatus = INITIALIZED_OK; //init passed
							break;
					default:Serial.println("\tUNKNOWN hardware");
							wiredStatus = INITIALIZED_NOK; //no usefull hardware found
				}
			}
			if (wiredStatus == INITIALIZED_OK) {
				//once initialized, check link status
				if (Ethernet.linkStatus() == LinkOFF) {
					//first time here, print the problem
					Serial.write("\tEthernet cable is not connected.\n");
					wiredStatus = CABLE_DISCONNECTED;
				}
			} else {
				while (true) {
					delay(1); // do nothing, no point running with none or unknown hardware
				}
			}
		}
		//passed DHCP init, so IP received
		wiredStatus = DHCP;
		Serial.print("\n\tGot wired IP: ");
		Serial.println(Ethernet.localIP());
	#else
		//use WiFi
		Serial.println("WIFI:\tsetup");
		WiFi.mode(WIFI_STA);
		//create hostname for device, based on prefix and mac
		char prefixedMac[3 + 1 + 6 + 1];	//prefix of 3 characters, 1 dash, 6 hex values of MAC and 1  terminating character
		ssidPrefixMac(&prefixedMac[0], "IoT");
		#if defined(ARDUINO_ARCH_ESP8266)
			WiFi.hostname(prefixedMac);
		#elif defined(ARDUINO_ARCH_ESP32)
			WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Workaround: makes setHostname() work
			WiFi.setHostname(prefixedMac);
		#endif
		Serial.print("\tconnecting ");
		WiFi.begin(WIFI_SSID, WIFI_PSK);	//set credentials in config.h
		while (WiFi.status() != WL_CONNECTED) {
			delay(250);
			Serial.print(".");
		}
		Serial.print("\n\tGot wireless IP: ");
		Serial.println(WiFi.localIP());
	#endif
} // setupNetwork

/*Function that returns data for Azure
	User can customize this funtion to his needs
	--> this is just for demonstration
	--> JSON string with unique device ID (based on MAC) & some random data
*/
String buildPayload() {
  char prefixedMac[3 + 1 + 6 + 1];	//prefix of 3 characters, 1 dash, 6 hex values of MAC and 1  terminating character
  ssidPrefixMac(&prefixedMac[0], "IoT");  //create unique device name
  String json = "{\"device\":\""    + (String)prefixedMac +
                "\",\"timestamp\":\"" + time(nullptr) +
                "\",\"rand1\":\"" + random(1, 100000) +
                "\",\"rand2\":\"" + random(1, 100000) +
                "\"}";
  return json;
} // buildPayload

void setup() {
	Serial.begin(115200);	//debug the needed things on serial port
	while (!Serial);		//is this needed? 
	Serial.println("\r\nPWR:\tBooting ESP");
	//provide noise to the random generator --> can be left out
	#if defined(ARDUINO_ARCH_ESP8266)
		randomSeed(analogRead(A0));
	#elif defined(ARDUINO_ARCH_ESP32)
		randomSeed(analogRead(36));
	#endif
	//set a digital output to demonstrate C2D (Cloud To Device)
	pinMode(DBG_PIN, OUTPUT);
	digitalWrite(DBG_PIN, LOW); //change to default state when power on
	//setup network depending on needs (wired/wireless)
	setupNetwork();
} // setup

//two timeout counters for C2D and D2C
uint32_t lastCheckC2D, lastCheckD2C;

void loop() {
	String methodName, payload;
	AZURE_REP reply;
	#if defined USE_LAN && USE_LAN == true
		while(Ethernet.linkStatus() == LinkOFF)
			if(wiredStatus==DHCP){
				//lost connection
				Serial.println("ETH:\tEthernet cable disconnected.");
				wiredStatus = CABLE_DISCONNECTED;
			}
		}
		if(wiredStatus==DHCP){
			Ethernet.maintain();  //maintain DHCP lease for wired ethernet
		}else{
			if(wiredStatus==CABLE_DISCONNECTED){
				//linked again
				Serial.print("\tRequesting IP");
				while (!Ethernet.begin(macAddr, 1000, 1000)){
					Serial.print(".");
				}
				//passed DHCP init, so IP received
				wiredStatus = DHCP;
				Serial.print("\n\tGot wired IP: ");
				Serial.println(Ethernet.localIP());
			}else{
				//can we ever come here?
			}
		}
	#else
		//maintain DHCP lease for WiFi?
		yield();
		//check if wifi is disconnected
		if (WiFi.status() != WL_CONNECTED) {
			WiFi.reconnect();
		}
	#endif
	if (lastCheckC2D > millis()) { //48-days overflow check of timer
		lastCheckC2D = millis();
	}
	if (lastCheckD2C > millis()) { //48-days overflow check of timer
		lastCheckD2C = millis();
	}
	azure.worker();	//check for problems (SAS renew, ...)
	//start with time
	if (ntpWorker() == SYNCHRONIZED) {
		//we are only able to do communication if time is correct
		if (!azure.isConnected()) {
			azure.setSasExpiry((uint16_t)SAS_RENEW);
			//azure is not connected/initalized
			#if defined IOT_CENTRAL && IOT_CENTRAL  == true
				//use function overloading
				azure.initAzure(SCOPE_ID, DEVICE_ID, DEVICE_KEY); //do or retry
			#else
				//use function overloading
				azure.initAzure(primaireVerbindingsreeks); //do or retry
			#endif
		} else {
			//azure is connected, we are able to transmit/receive messages
			if ((lastCheckD2C + (IOT_UPDATE * 1000) < millis()) || (lastCheckD2C == 0)) {
				Serial.println("MAIN:\tSending message... ");
				payload = buildPayload();
				reply = azure.send(payload); //Send to azure
				if (reply == AOK || reply == OK_EMPTY) {
					Serial.println("\ttransmit succeeded");
					lastCheckD2C = millis();
				} else {
					Serial.print("\terror! (");
					switch (reply) {  //you can create a custom message for every reply you got
						case CLOCK_NOT_SET:	Serial.print("Clock has not been set.");
											break;
						case TLS_ERROR:		Serial.print("TLS could not be established.");
											break;
						case DPS_ERROR:		Serial.print("DPS has failed.");
											break;
						case UNAUTH:		//Unauthorized
											Serial.print("Server failed to authenticate the request.");
											break;
						case BAD_REQUEST:	//bad request
											Serial.print("Bad format and/or request.");
											break;
						case FORBIDDEN:		//forbidden
											Serial.print("Forbidden.");
											break;
						case UNKNOWN:		Serial.print("Got an unknown reply.");
											break;
						default:			//none of the above
											Serial.printf("%u - huh?", reply);
											break;
					}
					Serial.println(")");
					//create 5 seconds wait to prevent overloading the service
					if (millis() < ((IOT_UPDATE * 1000) - 5000)) {
						delay(5000); //use delay, because unable to make subtraction
					} else {
						lastCheckD2C = millis() - ((IOT_UPDATE * 1000) + 5000);
					}
				}
			}
			//once every POLL_TIME
			if (lastCheckC2D + (POLL_TIME * 1000) < millis()) {
				Serial.println("MAIN:\tChecking for new message... ");
				reply = azure.receive(methodName, payload);
				if (reply == AOK) { //there was data
					Serial.printf("MAIN:\tmethodName: %s, payload: %s\r\n", methodName.c_str(), payload.c_str());
					if (methodName == "ledOn") {
						Serial.println("\tturning LED on");
						digitalWrite(DBG_PIN, HIGH); //change to default state when power on
					}
					if (methodName == "ledOff") {
						Serial.println("\tturning LED off");
						digitalWrite(DBG_PIN, LOW); //change to default state when power on
					}
				} else {
					Serial.println("\tqueue empty");
				}
				lastCheckC2D = millis();
			}
		}//azure.isConnected
	}//ntpWorker==SYNCHRONIZED
} // loop
