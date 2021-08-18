#include "iot.h"

bool IoT::isConnected(void){
	return azureInitialised;
}

void IoT::setSasExpiry(uint16_t timeInMinutes){
	sasExpiryPeriodInSeconds = timeInMinutes * 60;
}

void IoT::worker(void){
	#if (defined(ARDUINO_ARCH_ESP8266) && defined AUTO_CLOSE_8266 && AUTO_CLOSE_8266 == true)||(defined(ARDUINO_ARCH_ESP32))
	//check connection time out
	if(tlsClientTimeout+AUTO_CLOSE_TIME*60<now()){
		if(tlsClient.connected()){
			Serial.println("AZURE:\tclosing connection (timeout)");
			tlsClient.flush();
			tlsClient.stop(); //close a (possible) open connection
		}
	}
	#endif
}

bool IoT::initAzure(String cs){
	//make connection with azure through HUB
	useCentral = false;
	host = GetStringValue(splitStringByIndex(splitStringByIndex(cs, ';', 0), '=', 1));
	deviceId = GetStringValue(splitStringByIndex(splitStringByIndex(cs, ';', 1), '=', 1));
	key = (char*)GetStringValue(splitStringByIndex(splitStringByIndex(cs, ';', 2), '=', 1));
	//set needed things for communication
	sasUrl = (String)host + "/devices/" + (String)deviceId;
	endPoint = "/devices/" + (String)deviceId + "/messages/events/?api-version=2016-11-14";
	if(timeStatus()==timeNotSet){	//see Time library from PaulStoffregen
		return false;
	}
	//create SaS (for authentication)
	delete[] fullSas;
	fullSas = (char*)GetStringValue(createSas(key, sasUrl));
	//create BearSSL session
	#if defined(ARDUINO_ARCH_ESP8266)
		tlsClient.setSession(&tlsSession);
	#endif
	//connect to azure
	tlsClient.flush();
	tlsClient.stop(); //close a (possible) open connection and then reconnect
	azureInitialised = true;
	return true;
}

bool IoT::initAzure(String scope_id,String device_id,String device_key){
	String response;
	//make connection with azure through DPS
	useCentral = true;
	centralStep = NOT_RETRIEVED;	//three step process, begin as uninitialized
	host = "global.azure-devices-provisioning.net";
	scope = GetStringValue(scope_id);
	deviceId = GetStringValue(device_id);
	deviceId = GetStringValue(device_id);
	key = (char*)GetStringValue(device_key);
	//set needed things for communication
	sasUrl = (String)scope + "/registrations/" + (String)deviceId;
	endPoint = "/" + sasUrl + "/register?api-version=2018-11-01";
	if(timeStatus()==timeNotSet){	//see Time library from PaulStoffregen
		return false;
	}
	//create SaS (for authentication)
	delete[] fullSas;
	fullSas = (char*)GetStringValue(createSas(key, sasUrl));
	//create BearSSL session
	#if defined(ARDUINO_ARCH_ESP8266)
		tlsClient.setSession(&tlsSession);
	#endif
	//connect to azure
	tlsClient.stop(); //close a (possible) open connection and then reconnect
	if(connectToAzure()==TLS_ERROR){
		Serial.println("DPS:\tTLS could not be established!");
		azureInitialised = false;
		return false;	
	}else{
		tlsClient.flush();
		centralStep = REQUESTING_OID;	//we can continue
	}
	//begin retrieving hub
	if(centralStep==REQUESTING_OID){
		//get operation ID from DPS service
		Serial.print("DPS:\tTracing operation ID... ");
		fullSas = (char*)GetStringValue(fullSas + (String)"&skn=registration");
		response = exeHttpRequest(buildHttpRequest("PUT", "{\"registrationId\":\"" + (String)deviceId + "\"}"));
		//parse JSON for operation ID --> {"operationId":"4.9054b6031136fb51.c234c764-33b8-43bd-b26e-64431c5d8864","status":"assigning"}
		operationId = splitStringByIndex(splitStringByIndex(splitStringByIndex(response, '{', 1),'}',0),'"',3);
		if(operationId.length()>0){
			Serial.println(operationId);
			centralStep=REQUESTING_HUB;
		}else{
			Serial.println("error, did not found the ID in response...");
			Serial.print("\tResponse was: ");
			Serial.println(response);
			centralStep = COM_ERROR;
		}
	}
	if(centralStep==REQUESTING_HUB){
		//get hub from operation ID
		Serial.print("\tTracing assigned hub...");
		//we have to wait a few seconds so azure can assign a hub (takes time)
		//if too fast, response will be status":"assigning" instead of "status":"assigned"
		uint8_t timeoutCnt = 0;
		do{
			#if defined(ARDUINO_ARCH_ESP8266)
				//don't sleep, yield in between
				for(uint8_t i=0;i<100;i++){
					delay(10);
					yield();
				}
			#elif defined(ARDUINO_ARCH_ESP32)
				vTaskDelay(1000/portTICK_RATE_MS);
			#endif
			endPoint = "/" + sasUrl + "/operations/" + operationId + "?api-version=2018-11-01";
			response = exeHttpRequest(buildHttpRequest("GET", ""));
			//{"operationId":"4.9054b6031136fb51.9d9a9ae8-f528-4bee-bbac-ca789bf20403","status":"assigned","registrationState":{"registrationId":"2o6bjrqzry6","createdDateTimeUtc":"2021-03-14T20:08:50.8829917Z","assignedHub":"iotc-8f801c77-b388-4ef5-900c-a0ede1c90050.azure-devices.net","deviceId":"2o6bjrqzry6","status":"assigned","substatus":"initialAssignment","lastUpdatedDateTimeUtc":"2021-03-14T20:08:51.1309183Z","etag":"IjI1MDA1MjlhLTAwMDAtMGQwMC0wMDAwLTYwNGU2ZDUzMDAwMCI="}}
			assignedHub = splitStringByIndex(splitStringByIndex(splitStringByIndex(response, '{', 1),':',2),'"',1);
			Serial.print(".");
			timeoutCnt++;
		}while(assignedHub!="assigned");
		if(timeoutCnt<6){
			//we retrieved the assigned hub
			assignedHub = splitStringByIndex(splitStringByIndex(splitStringByIndex(response, '{', 2),'}',0),'"',11);
		}else{
			//status was never assigned
			assignedHub = "";
		}
		if(assignedHub.length()>0){
			Serial.print(" ");
			Serial.println(assignedHub);
			centralStep = READY;
		}else{
			Serial.println(" error, did not found the hub in response...");
			Serial.print("\tResponse was: ");
			Serial.println(response);
			centralStep = COM_ERROR;
		}
		
	}
	if(centralStep==READY){
		centralTimeout = time(nullptr);
		//reset SAS to hub
		sasUrl = assignedHub + "/devices/" + (String)deviceId;
		endPoint = "/devices/" + (String)deviceId + "/messages/events/?api-version=2016-11-14";
		host = GetStringValue(assignedHub);
		delete[] fullSas;
		fullSas = (char*)GetStringValue(createSas(key, sasUrl));
		tlsClient.flush();
		tlsClient.stop();
		azureInitialised = true;
		return true;
	}else{
		//something went wrong
		return false;
	}
}

AZURE_REP IoT::connectToAzure(){
	if(tlsClient.connected()){
		tlsClientTimeout = now(); //reset  timeout of connection
		return TLS_CONNECTED;
	}
	// give network connection a moment to settle
	#if defined(ARDUINO_ARCH_ESP8266)
		//don't sleep, yield in between
		for(uint8_t i=0;i<50;i++){
			delay(10);
			yield();
		}
	#elif defined(ARDUINO_ARCH_ESP32)
		vTaskDelay(500/portTICK_RATE_MS);
	#endif
	//try MFLN (Maximum Fragmentation Length) for ESP8266
	#if defined(ARDUINO_ARCH_ESP8266)
	tlsClient.setInsecure();
	bool mfln = tlsClient.probeMaxFragmentLength(host, 443, 512);
		Serial.print("AZURE:\t");
		Serial.printf("MFLN supported? %s\r\n", mfln ? "yes" : "no");
	if (mfln) {
		tlsClient.setBufferSizes(512, 512);
		Serial.println("\tusing 512 bytes buffer instead of 16 kbytes");
	}
	#endif
	// give network connection a moment to settle
	#if defined(ARDUINO_ARCH_ESP8266)
		//don't sleep, yield in between
		for(uint8_t i=0;i<50;i++){
			delay(10);
			yield();
		}
	#elif defined(ARDUINO_ARCH_ESP32)
		vTaskDelay(500/portTICK_RATE_MS);
	#endif
	tlsClient.flush();
	tlsClient.stop();
	Serial.print("AZURE:\t");
	Serial.print(deviceId);
	Serial.print(" connecting to ");
	Serial.println(host);
	#if defined(ARDUINO_ARCH_ESP8266)
	//tlcCert will only live inside if...
		if(useCentral){
			BearSSL::X509List tlsCert(rootCACertificateCentral);
			tlsClient.setTrustAnchors(&tlsCert);
			tlsClient.connect(host, 443);
		}else{
			BearSSL::X509List tlsCert(rootCACertificateHub);
			tlsClient.setTrustAnchors(&tlsCert);
			tlsClient.connect(host, 443);
		}
    #elif defined(ARDUINO_ARCH_ESP32)
        if(useCentral){
			tlsClient.setCACert(rootCACertificateCentral);
		}else{
			tlsClient.setCACert(rootCACertificateHub);
		}
		tlsClient.connect(host, 443);
    #endif
	if(!tlsClient.connected()) {
		return TLS_ERROR;
	}
	yield(); // give firmware some time 
	tlsClientTimeout = now();
	return TLS_CONNECTED;
}

String IoT::splitStringByIndex(String data, char separator, int index){
	int found = 0;
	int strIndex[] = { 0, -1 };
	int maxIndex = data.length() - 1;
	for (int i = 0; i <= maxIndex && found <= index; i++){
		if (data.charAt(i) == separator || i == maxIndex){
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}
	return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

const char* IoT::GetStringValue(String value){
	int len = value.length() + 1;		//add one byte to string length for array termination
	char *temp = new char[len];			//create a pointer to an array
	value.toCharArray(temp, len);		//convert string to array for length len
	return temp;						//return the pointer to the array
}

AZURE_REP IoT::send(String json){
	String response = "";
	if(useCentral&&((centralTimeout+(24*3600))<time(nullptr))){
		//one day has passed, reconnect with central
		Serial.println("AZURE:\tResetting connection with Central");
		if(!initAzure((String)scope,(String)deviceId,(String)key)){
			return NO_CONNECTION;
		}
	}
	if(azureInitialised){
		if(now()>sasExpiryTime){
			//have old key, so renew
			delete[] fullSas;
			fullSas = (char*)GetStringValue(createSas(key, sasUrl));
		}
		AZURE_REP reply = connectToAzure();
		if(reply!=TLS_CONNECTED){
			return reply;
		}
		response = exeHttpRequest(buildHttpRequest("POST", json));
		//Serial.println(response);
	}else{
		return NO_CONNECTION;
	}
	//return (response.length() > 12) ? response.substring(9, 12) : response;	//return API "error" code
	if(response.length() > 12){
		//probably reply
		return (AZURE_REP)atoi(response.substring(9, 12).c_str());
	}else{
		return UNKNOWN; //or success?
	}
}

AZURE_REP IoT::receive(String& methodName,String& payload){
	//retreive message with ID from Azure
	String response = "";
	if(useCentral&&((centralTimeout+(24*3600))<time(nullptr))){
		//one day has passed, reconnect with central
		Serial.println("AZURE:\tResetting connection with Central");
		if(!initAzure((String)scope,(String)deviceId,(String)key)){
			return NO_CONNECTION;
		}
	}
	if(azureInitialised){
		if(now()>sasExpiryTime){
			//have old key, so renew
			delete[] fullSas;
			fullSas = (char*)GetStringValue(createSas(key, sasUrl));
		}
		connectToAzure();
		//connected with hub
		String prevEndpoint = endPoint;
		endPoint = "/devices/" + (String)deviceId + "/messages/deviceBound?api-version=2020-03-13";
		String response = exeHttpRequest(buildHttpRequest("GET", ""));
		//Serial.println(response);
		methodName = splitStringByIndex(splitStringByIndex(response, ':', 14),'\r',0);
		methodName = methodName.substring(1, methodName.length());
		payload = "{" + splitStringByIndex(splitStringByIndex(response, '{', 1),'}',0);
		if(!useCentral){
			#if defined SWAP_HUB_KEYVALUE && SWAP_HUB_KEYVALUE == true
				//there is no method name, so place (non JSON) payload inside methodName
				methodName = splitStringByIndex(response, '\r', 17);
				payload = "";
			#endif
		}
		uint16_t errorCode = atoi(response.substring(9, 12).c_str());
		//IoT Central will keep command in queue if command is not acknowledged
		//Command should be removed by eTag (unique message tag)
		String eTag = splitStringByIndex(splitStringByIndex(response, '"', 1),'"',0);
		if(eTag!=""){
			//message has eTag, so acknowledge/remove it
			Serial.print("C2D:\tCommand from cloud queue received\r\n\teTag:");
			Serial.println(eTag);
			Serial.print("\tRemoving message from queue...");
			//https://{iotHub}.azure-devices.net/devices/{deviceId}/messages/deviceBound/{etag}?api-version={api}" + DELETE
			//https://{iotHub}.azure-devices.net/devices/{deviceId}/messages/deviceBound/{etag}/abandon?api-version={api}" + POST
			endPoint = "/devices/" + (String)deviceId + "/messages/deviceBound/" + eTag + "?api-version=2016-11-14";
			//if we want a dead letter, we can reject it by adding "&reject" after endPoint...
			String response = exeHttpRequest(buildHttpRequest("DELETE", ""));
			if(atoi(response.substring(9, 12).c_str())==204){
				Serial.println("OK");
			}else{
				Serial.println("failed");
			}
		}
		endPoint = prevEndpoint;
		//pass the data by reference
		return (AZURE_REP)errorCode;
	}else{
		return NO_CONNECTION;
	}
}

String IoT::exeHttpRequest(String http){
	//Serial.println(http);
	tlsClient.print(http);
	String response = "";
	String chunk = "";
	int limit = 1;
	do{
		if(tlsClient.connected()){
			yield();
			chunk = tlsClient.readStringUntil('\n');
			response += chunk;
		}
	}while(chunk.length()>0 && ++limit<100);
	return response;
}

String IoT::buildHttpRequest(String type, String data){
	String http = 	type + " " + endPoint + " HTTP/1.1\r\n" +
					"Host: " + host + "\r\n" +
					"Authorization: SharedAccessSignature " + fullSas + "\r\n";
	if(useCentral&&centralStep==2){
		//need additional header
		http = http + "iothub-to: /devices/" + deviceId + "/messages/events\r\n";
	}
	http = http + 	"User-Agent: ESP minimal C++\r\n" +
					"Content-Type: application/json\r\n" +
					"Content-Length: " + data.length() + "\r\n\r\n" + data;
	return http;
}


String IoT::createSas(char* key, String url){
	Serial.println("AZURE:\tCreating new SAS");
	sasExpiryTime = now() + sasExpiryPeriodInSeconds;
	String stringToSign = url + "\n" + sasExpiryTime;
	int keyLength = strlen(key);
	int decodedKeyLength = base64_dec_len(key, keyLength);
	char decodedKey[decodedKeyLength];  //allocate char array big enough for the base64 decoded key
	base64_decode(decodedKey, key, keyLength);  //decode key
	Sha256.initHmac((const uint8_t*)decodedKey, decodedKeyLength);
	Sha256.print(stringToSign);  
	char* sign = (char*) Sha256.resultHmac();
	int encodedSignLen = base64_enc_len(HASH_LENGTH);
	char encodedSign[encodedSignLen];
	base64_encode(encodedSign, sign, HASH_LENGTH); 
	//create shared acces key signature from newly encoded SAS
	return "sr=" + url + "&sig="+ urlEncode(encodedSign) + "&se=" + sasExpiryTime;
}

String IoT::urlEncode(const char* msg){
	const char *hex = "0123456789abcdef";
	String encodedMsg = "";
	while(*msg!='\0'){
		if( ('a' <= *msg && *msg <= 'z')			//between a until z
			|| ('A' <= *msg && *msg <= 'Z')			//or between A until Z
			|| ('0' <= *msg && *msg <= '9') ) {		//or between 0 until 9
			encodedMsg += *msg;						//nothing to encode, just copy
		}else{										//special character, need to encode
			encodedMsg += '%';						//prefix
			encodedMsg += hex[*msg >> 4];			//MSnibble to hex
			encodedMsg += hex[*msg & 15];			//LSnibble to hex
		}
		msg++;
	}
	return encodedMsg;
}
