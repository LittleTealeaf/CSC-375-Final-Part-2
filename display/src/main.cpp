#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Core2.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <WiFi.h>

#define SPRITE_COLOR_DEPTH 4
#define SCREEN_WIDTH 315
#define SCREEN_HEIGHT 240

#define SPRITE_WIDTH 103
#define SPRITE_HEIGHT 118
#define SPRITE_PADDING 1

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";
int wifiStatus;
long scanDelay;

Ticker wifiTicker;

TFT_eSprite screenSprite = TFT_eSprite(&M5.Lcd);
TFT_eSprite viewPorts[] = {TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd)};

void displayMessage(const char *message) {
  screenSprite.fillScreen(TFT_BLACK);
  screenSprite.setTextSize(1);
  screenSprite.setCursor(0, 0);
  screenSprite.println(message);
  screenSprite.pushSprite(0, 0);
}

void pushViewPort(int index) {
  int y = index < 3 ? SPRITE_PADDING : SPRITE_HEIGHT + SPRITE_PADDING * 3;
  int x = SPRITE_PADDING + (SPRITE_PADDING * 2 + SPRITE_WIDTH) * (index % 3);
  viewPorts[index].pushSprite(x, y);
}

void onWifiDisconnected() { displayMessage("Wifi Not Connected"); }

void onWiFiConnected() {
	screenSprite.fillScreen(TFT_BLACK);
	screenSprite.pushSprite(0, 0);
	for(int i = 0; i < 6; i++) {
		pushViewPort(i);
	}
  // setup MQTT, etc
}

bool checkWiFi() {
  int status = WiFi.status();

  if (status == WL_CONNECTED) {
    if (wifiStatus != status) {
      onWiFiConnected();
    }
  } else {
    if (status == WL_CONNECTED) {
      onWifiDisconnected();
    }

    // if (scanDelay <= millis()) {
    int scanResult = WiFi.scanComplete();

    if (scanResult == -2 && scanDelay <= millis()) {
      WiFi.scanNetworks(true);
    } else if (scanResult == -1) {
    } else if (scanResult >= 0) {
      for (int i = 0; i < scanResult; i++) {
        if (WiFi.SSID(i).indexOf(WiFiPrefix) == 0) {
          WiFi.begin(WiFi.SSID(i).begin(), WiFiPassword);
          scanDelay = millis() + 10000;
        }
				WiFi.scanDelete();
      }
      //}
    }
  }

  wifiStatus = status;

  return status == WL_CONNECTED;
}

void setup() {
  M5.begin();
  Serial.begin(115200);

  for (int i = 0; i < 6; i++) {
    viewPorts[i].setColorDepth(SPRITE_COLOR_DEPTH);
    viewPorts[i].createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    viewPorts[i].fillScreen(TFT_BLUE);
  }
  screenSprite.setColorDepth(SPRITE_COLOR_DEPTH);
  screenSprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  onWifiDisconnected();
}

void loop() { checkWiFi(); }

/*
void pushMessage(const char *message) {
        screenSprite.fillScreen(TFT_BLACK);
        screenSprite.setTextSize(3);
        screenSprite.setCursor(0, 0);
        screenSprite.println(message);
        screenSprite.pushSprite(0, 0);
}

void pushViewPort(int index) {
  int y = index < 3 ? 0 : SPRITE_HEIGHT;
  int x = SPRITE_WIDTH * (index % 3);
  viewPorts[index].pushSprite(x, y);
}

void checkWiFi() {
  int status = WiFi.status();
  if (status != WL_CONNECTED) {
        int scanResult = WiFi.scanComplete();

                if(scanResult == -2) {
                        WiFi.scanNetworks(true);
                } else if(scanResult >= 0) {

                        for(int i = 0; i < scanResult; i++) {
                                if(WiFi.SSID(i).startsWith(WiFiPrefix)) {
                                        WiFi.begin(WiFi.SSID(i).begin(),
WiFiPassword);
                                }
                        }

                        WiFi.scanDelete();
                }
        }
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
        screenSprite.setColorDepth(SPRITE_COLOR_DEPTH);
        screenSprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  // Begin wifi and search
  WiFi.mode(WIFI_STA);
        WiFi.disconnect();


        wifiTicker.attach_ms(1000,checkWiFi);
        priorWiFiStatus = WiFi.status();
}

int ind = 0;

void loop() {
        if(WiFi.status() != WL_CONNECTED) {
                pushMessage("Not Connected to WiFi");
        } else {
                if(priorWiFiStatus != WL_CONNECTED) {
                        for(int i = 0; i < 6; i++) {
                                pushViewPort(i);
                        }
                }



        }

        priorWiFiStatus = WiFi.status();
}
*/
