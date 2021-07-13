//Global settings (hardware, WiFi credentials)
	#define WIFI_SSID "YOUR-SSID-HERE"	//set to SSID to connect to
	#define WIFI_PSK "YOUR-PSWD-HERE"	//set to password of the SSID

	#define USE_LAN		false			//set true if wired network must be used
										//removing this setting will use WiFi
										//will only work with Wizznet SPI board on ESP8266
										//other libraries are needed for ESP32 (future?)

	#define ETH_CS	4					//wired ethernet WIZ5500 CS pin
	
	#define DBG_PIN	2					//LED is connected to GPIO2 on the Firebeetle ESP32 board

//Azure interface setup
	#define IOT_CENTRAL	false			//set to true if we use central, if false, direct hub access will be used
	#define SAS_RENEW	15    			//SAS regenerate every x minutes
	#define IOT_UPDATE	300				//send new message to cloud every x seconds
	#define POLL_TIME	60				//check for new C2D message every x seconds

//Azure credentials
	//when using hub, a primary connection string is needed (containing hostname, device and SAS key)
	//	--> you can find this connection string in the azure portal
	const char* primaireVerbindingsreeks = "HostName=xyz.azure-devices.net;DeviceId=ESP8266;SharedAccessKey=XcDOTsMyDhfFrkKu/WuajID4nx5gdrhfc1InPrLuER8=";

	//when using central, you have to provide the scope, device and key
	//	--> you can find these settings in the azure portal
	const char* SCOPE_ID = "0ne00241B52";
	const char* DEVICE_ID = "1oikfewjvtb";
	const char* DEVICE_KEY = "omvqlYa9x6+uZBGfNdfhghAjv8kRQMVaBzroOc0NDCes=";