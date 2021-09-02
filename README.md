# IoT_ESP_Azure
C-code based on different projects from around the web, adapted to work with an ESP8266 and ESP32. Straight forward code to send and receive (polling) message from an IoT Hub of Azure (direct or through Central)

## Innovet
The code was initialy developed and released as a research project, financed by [onderwijs.vlaanderen.be](https://onderwijs.vlaanderen.be/nl/innovet-projecten-schooljaar-2020-2021#IoT). The previous research project was mainly about interfacing sensors and actuators for the IoT scene, while this year the main task was communicating with the Microsoft Azure platform. The combination of the two projects gives a real life IoT application (e.g. monitoring digital power meter).

The first initial release does work on either the ESP8266 or the ESP32. Various options are available, but some are not fully implemented:
|Option| ESP8266 | ESP32|
|---|---|
|Azure HUB|X|X|
|Azure Central|X|X|
|WiFi|X|X|
|Ethernet (Wiz5500)|X|/|
|SSL store|/|/|

The code should work with only a few changes, mainly in the `config.h` file:
### &ensp;Global configuration
* &ensp;When using WiFi, you should ofcourse provide the SSID to connect to `#define WIFI_SSID "YOUR-SSID-HERE"`. When a password is needed to connect to the SSID, also provide `#define WIFI_PSK "YOUR-PSWD-HERE"`
* &ensp;When using Ethernet, choose for using Ethernet instead of WiFi by providing `#define USE_LAN true`. When set to another value then `true`, WiFi will be used. If this define is not provided, WiFi will be used.
* &ensp;The Ethernet relies on the Wiz5500 chip connected to the SPI bus. There are sufficient examples on how  to connect on the internet. An [example](https://esp8266hints.wordpress.com/2018/02/13/adding-an-ethernet-port-to-your-esp-revisited/) can be found here. The CS line can be choosen freely, but must be provided by `#define ETH_CS	x`, where x is the pin on the ESP8266 being used. The reset is optional for the ESP8266 but will be included in the ESP32 version. 

### &ensp;Azure configuration
&ensp;There are a few options which can be configured:
* &ensp;`#define IOT_CENTRAL false` will configure the code to use Azure HUB. Is this setting will be placed on true, then Azure CENTRAL will be used. See Azure credentials (further on) to provide the needed credentials to connect to Azure.
* &ensp;`#define SAS_RENEW	15` is the time given in minutes between SAS ([Shared Access Signatures](https://docs.microsoft.com/en-us/azure/storage/common/storage-sas-overview)) renewals. When your SAS is compromised, the attacker can only use it for the maximum of the given time. 
* &ensp;`#define IOT_UPDATE	300` is the time given in seconds between transmitting messages. The demo code does transmit a JSON based message with a given interval. 
* &ensp;`#define POLL_TIME	60` is the time given in seconds to check for new messages. Messages transmitted from the cloud to the device will be queued. The ESP will check if messages are enqueued every given interval.

### &ensp;Azure credentials
&ensp;Depending on using HUB or CENTRAL, the correct credentials should be given.
* &ensp;For HUB, only one connection string should be provided. This connection string does contain all needed settings (HUB connection point, HUB device name and HUB SAS). Provide this in the following way `const char* primaireVerbindingsreeks = "HostName=xyz.azure-devices.net;DeviceId=ESP8266;SharedAccessKey=XcDOTsMyDhfFrkKu/WuajID4nx5gdrhfc1InPrLuER8=";`
* &ensp;For CENTRAL, the three different settings should be provided seperately. `const char* SCOPE_ID = "0ne00241B52";`, `const char* DEVICE_ID = "1oikfewjvtb";` and `const char* DEVICE_KEY = "omvqlYa9x6+uZBGfNdfhghAjv8kRQMVaBzroOc0NDCes=";`.
All settings can be found in the [Azure Portal](https://azure.microsoft.com/nl-nl/features/azure-portal/). 

## Future releases
- [ ] Wired ethernet on the ESP32: The ESP32 has an inbuild MAC which does only need an external PHY. Using a MAC+PHY (as the Wiz5500) gives problems with the default libraries. An altered version of the code is currently being tested and will be merged with the tree if found working.
- [ ] SSL store: The given code uses http**s**. Every time a message thas to be transmitted or received through Azure, the certificate is checked. This certificate is saved in the header file of the azure code.
```
static const char rootCACertificateCentral[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFWjCCBEKgAwIBAgIQD6dHIsU9iMgPWJ77H51KOjANBgkqhkiG9w0BAQsFADBa
MQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl
clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTIw
MDcyMTIzMDAwMFoXDTI0MTAwODA3MDAwMFowTzELMAkGA1UEBhMCVVMxHjAcBgNV
BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEgMB4GA1UEAxMXTWljcm9zb2Z0IFJT
QSBUTFMgQ0EgMDIwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQD0wBlZ
qiokfAYhMdHuEvWBapTj9tFKL+NdsS4pFDi8zJVdKQfR+F039CDXtD9YOnqS7o88
+isKcgOeQNTri472mPnn8N3vPCX0bDOEVk+nkZNIBA3zApvGGg/40Thv78kAlxib
MipsKahdbuoHByOB4ZlYotcBhf/ObUf65kCRfXMRQqOKWkZLkilPPn3zkYM5GHxe
I4MNZ1SoKBEoHa2E/uDwBQVxadY4SRZWFxMd7ARyI4Cz1ik4N2Z6ALD3MfjAgEED
woknyw9TGvr4PubAZdqU511zNLBoavar2OAVTl0Tddj+RAhbnX1/zypqk+ifv+d3
CgiDa8Mbvo1u2Q8nuUBrKVUmR6EjkV/dDrIsUaU643v/Wp/uE7xLDdhC5rplK9si
NlYohMTMKLAkjxVeWBWbQj7REickISpc+yowi3yUrO5lCgNAKrCNYw+wAfAvhFkO
eqPm6kP41IHVXVtGNC/UogcdiKUiR/N59IfYB+o2v54GMW+ubSC3BohLFbho/oZZ
5XyulIZK75pwTHmauCIeE5clU9ivpLwPTx9b0Vno9+ApElrFgdY0/YKZ46GfjOC9
ta4G25VJ1WKsMmWLtzyrfgwbYopquZd724fFdpvsxfIvMG5m3VFkThOqzsOttDcU
fyMTqM2pan4txG58uxNJ0MjR03UCEULRU+qMnwIDAQABo4IBJTCCASEwHQYDVR0O
BBYEFP8vf+EG9DjzLe0ljZjC/g72bPz6MB8GA1UdIwQYMBaAFOWdWTCCR1jMrPoI
VDaGezq1BE3wMA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYI
KwYBBQUHAwIwEgYDVR0TAQH/BAgwBgEB/wIBADA0BggrBgEFBQcBAQQoMCYwJAYI
KwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTA6BgNVHR8EMzAxMC+g
LaArhilodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vT21uaXJvb3QyMDI1LmNybDAq
BgNVHSAEIzAhMAgGBmeBDAECATAIBgZngQwBAgIwCwYJKwYBBAGCNyoBMA0GCSqG
SIb3DQEBCwUAA4IBAQCg2d165dQ1tHS0IN83uOi4S5heLhsx+zXIOwtxnvwCWdOJ
3wFLQaFDcgaMtN79UjMIFVIUedDZBsvalKnx+6l2tM/VH4YAyNPx+u1LFR0joPYp
QYLbNYkedkNuhRmEBesPqj4aDz68ZDI6fJ92sj2q18QvJUJ5Qz728AvtFOat+Ajg
K0PFqPYEAviUKr162NB1XZJxf6uyIjUlnG4UEdHfUqdhl0R84mMtrYINksTzQ2sH
YM8fEhqICtTlcRLr/FErUaPUe9648nziSnA0qKH7rUZqP/Ifmbo+WNZSZG1BbgOh
lk+521W+Ncih3HRbvRBE0LWYT8vWKnfjgZKxwHwJ
-----END CERTIFICATE-----
)EOF";
```

```
static const char rootCACertificateHub[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFWjCCBEKgAwIBAgIQD6dHIsU9iMgPWJ77H51KOjANBgkqhkiG9w0BAQsFADBa
MQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl
clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTIw
MDcyMTIzMDAwMFoXDTI0MTAwODA3MDAwMFowTzELMAkGA1UEBhMCVVMxHjAcBgNV
BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEgMB4GA1UEAxMXTWljcm9zb2Z0IFJT
QSBUTFMgQ0EgMDIwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQD0wBlZ
qiokfAYhMdHuEvWBapTj9tFKL+NdsS4pFDi8zJVdKQfR+F039CDXtD9YOnqS7o88
+isKcgOeQNTri472mPnn8N3vPCX0bDOEVk+nkZNIBA3zApvGGg/40Thv78kAlxib
MipsKahdbuoHByOB4ZlYotcBhf/ObUf65kCRfXMRQqOKWkZLkilPPn3zkYM5GHxe
I4MNZ1SoKBEoHa2E/uDwBQVxadY4SRZWFxMd7ARyI4Cz1ik4N2Z6ALD3MfjAgEED
woknyw9TGvr4PubAZdqU511zNLBoavar2OAVTl0Tddj+RAhbnX1/zypqk+ifv+d3
CgiDa8Mbvo1u2Q8nuUBrKVUmR6EjkV/dDrIsUaU643v/Wp/uE7xLDdhC5rplK9si
NlYohMTMKLAkjxVeWBWbQj7REickISpc+yowi3yUrO5lCgNAKrCNYw+wAfAvhFkO
eqPm6kP41IHVXVtGNC/UogcdiKUiR/N59IfYB+o2v54GMW+ubSC3BohLFbho/oZZ
5XyulIZK75pwTHmauCIeE5clU9ivpLwPTx9b0Vno9+ApElrFgdY0/YKZ46GfjOC9
ta4G25VJ1WKsMmWLtzyrfgwbYopquZd724fFdpvsxfIvMG5m3VFkThOqzsOttDcU
fyMTqM2pan4txG58uxNJ0MjR03UCEULRU+qMnwIDAQABo4IBJTCCASEwHQYDVR0O
BBYEFP8vf+EG9DjzLe0ljZjC/g72bPz6MB8GA1UdIwQYMBaAFOWdWTCCR1jMrPoI
VDaGezq1BE3wMA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYI
KwYBBQUHAwIwEgYDVR0TAQH/BAgwBgEB/wIBADA0BggrBgEFBQcBAQQoMCYwJAYI
KwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTA6BgNVHR8EMzAxMC+g
LaArhilodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vT21uaXJvb3QyMDI1LmNybDAq
BgNVHSAEIzAhMAgGBmeBDAECATAIBgZngQwBAgIwCwYJKwYBBAGCNyoBMA0GCSqG
SIb3DQEBCwUAA4IBAQCg2d165dQ1tHS0IN83uOi4S5heLhsx+zXIOwtxnvwCWdOJ
3wFLQaFDcgaMtN79UjMIFVIUedDZBsvalKnx+6l2tM/VH4YAyNPx+u1LFR0joPYp
QYLbNYkedkNuhRmEBesPqj4aDz68ZDI6fJ92sj2q18QvJUJ5Qz728AvtFOat+Ajg
K0PFqPYEAviUKr162NB1XZJxf6uyIjUlnG4UEdHfUqdhl0R84mMtrYINksTzQ2sH
YM8fEhqICtTlcRLr/FErUaPUe9648nziSnA0qKH7rUZqP/Ifmbo+WNZSZG1BbgOh
lk+521W+Ncih3HRbvRBE0LWYT8vWKnfjgZKxwHwJ
-----END CERTIFICATE-----
)EOF";
```
These certificates do change over time, so they need to be replaced once in a while (which means the code should be recompiled). Using a certificate store and placing the certificates in a file space only need an update of the certificates flash file system. 
