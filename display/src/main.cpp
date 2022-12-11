#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Core2.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define SPRITE_COLOR_DEPTH 4
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 240

#define SPRITE_WIDTH 100
#define SPRITE_HEIGHT 120

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";

Ticker wifiTicker;


TFT_eSprite viewPorts[] = {TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd)};

void pushViewPort(int index);

void checkWiFi() {
  int status = WiFi.status();
  if (status != WL_CONNECTED) {
  	int scanResult = WiFi.scanComplete();
	
		if(scanResult == -2) {
			WiFi.scanNetworks(true);
		} else if(scanResult >= 0) {
			
			for(int i = 0; i < scanResult; i++) {
				if(WiFi.SSID(i).startsWith(WiFiPrefix)) {
					WiFi.begin(WiFi.SSID(i).begin(), WiFiPassword);
				}
			}

			WiFi.scanDelete();
		}
	}
}



void pushViewPort(int index) {
  int y = index < 3 ? 0 : SPRITE_HEIGHT;
  int x = SPRITE_WIDTH * (index % 3);
  viewPorts[index].pushSprite(x, y);
}

void setupViewports() {
  for (int i = 0; i < 6; i++) {
    viewPorts[i].setColorDepth(SPRITE_COLOR_DEPTH);
    viewPorts[i].createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
  }
}

void setup() {
  M5.begin();
	Serial.begin(115200);
  setupViewports();
  // Begin wifi and search
  WiFi.mode(WIFI_STA);
	WiFi.disconnect();


	wifiTicker.attach_ms(1000,checkWiFi);
}

int ind = 0;

void loop() {

}
