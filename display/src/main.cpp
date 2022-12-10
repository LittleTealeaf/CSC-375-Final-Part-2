#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Core2.h>
#include <WiFi.h>
#include <MQTT.h>

#define SPRITE_COLOR_DEPTH 4
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 240

#define SPRITE_WIDTH 100
#define SPRITE_HEIGHT 120

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";

TFT_eSprite viewPorts[] = {TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd)};

int checkWiFi() {
  int status = WiFi.status();
  if (status != WL_CONNECTED) {

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
  }
  return status;
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
  setupViewports();
  // Begin wifi and search
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  viewPorts[0].fillScreen(TFT_GREEN);
  viewPorts[1].fillScreen(TFT_BLUE);
  viewPorts[2].fillScreen(TFT_WHITE);
  viewPorts[3].fillScreen(TFT_PURPLE);
  viewPorts[4].fillScreen(TFT_ORANGE);
  viewPorts[5].fillScreen(TFT_NAVY);
}

int ind = 0;

void loop() {
  checkWiFi();

  if (ind < 6) {
    pushViewPort(ind);
    ind++;
    delay(100);
  }
}
