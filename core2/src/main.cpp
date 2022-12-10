#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Core2.h>
#include <WiFi.h>

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";


int checkWiFi() {
	if(!WiFi.isConnected()) {

		int status = WiFi.scanComplete();

		if(status == -2) {
			WiFi.scanNetworks(true);
		} else if(status) {
			for(int i = 0; i < status; i++) {
				if(WiFi.SSID(i).indexOf(WiFiPrefix) == 0) {
					WiFi.begin(WiFi.SSID(i).begin(), WiFiPassword);
				}
			}
		}

		return 1;
	} else {
		return 0;
	}
}


void setup() {
  M5.begin();

  // Begin wifi and search
  WiFi.mode(WIFI_STA);
  WiFi.begin();
}


void loop() {
	checkWiFi();
}
