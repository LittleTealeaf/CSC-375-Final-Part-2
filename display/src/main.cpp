#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Core2.h>
#include <WiFi.h>

#define SPRITE_COLOR_DEPTH 4
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 240

#define SPRITE_WIDTH 100
#define SPRITE_HEIGHT 140

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";

TFT_eSprite viewPorts[] = {TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd)};

int checkWiFi() {
  if (!WiFi.isConnected()) {

    int status = WiFi.scanComplete();

    if (status == -2) {
      WiFi.scanNetworks(true);
    } else if (status) {
      for (int i = 0; i < status; i++) {
        if (WiFi.SSID(i).indexOf(WiFiPrefix) == 0) {
          WiFi.begin(WiFi.SSID(i).begin(), WiFiPassword);
        }
      }
    }

    return 1;
  } else {
    return 0;
  }
}

void pushViewPort(int index) {
	int y = SPRITE_HEIGHT * index / 3;
	int x = SPRITE_WIDTH * (index % 3);
	viewPorts[index].pushSprite(x, y);
}

void setupViewports() {
	for(int i = 0; i < 6; i++) {
		viewPorts[i].setColorDepth(SPRITE_COLOR_DEPTH);
		viewPorts[i].createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
	}
}

void setup() {
  M5.begin();
	setupViewports();
  // Begin wifi and search
  WiFi.mode(WIFI_STA);
  WiFi.begin();
}

void loop() { checkWiFi(); }
