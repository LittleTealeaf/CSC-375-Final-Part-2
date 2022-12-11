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

IPAddress server(192,168,4,3);
int port = 1883;

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";
int wifiStatus;
long scanDelay, mqttConnectDelay;

Ticker wifiTicker;

TFT_eSprite screenSprite = TFT_eSprite(&M5.Lcd);
TFT_eSprite viewPorts[] = {TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd)};

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

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

bool checkMQTT() {
	
	bool connected = mqttClient.connected();

	if(!connected && mqttConnectDelay <= millis()) {

		displayMessage("Connecting to MQTT Client");

		if(mqttClient.connect("arduinoClient")) {
			mqttClient.subscribe("CSC375/dist");
		} else {
			displayMessage("MQTT Connection Failed, Waiting 10 seconds");
			mqttConnectDelay = millis() + 10000;
		}
	}


	return false;	
}

void onMessageReceived(char* topic, byte* payload, unsigned int length) {
}

void setup() {
  M5.begin();
  Serial.begin(115200);

  for (int i = 0; i < 6; i++) {
    viewPorts[i].setColorDepth(SPRITE_COLOR_DEPTH);
    viewPorts[i].createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    viewPorts[i].fillScreen(TFT_DARKGREY);
  }
  screenSprite.setColorDepth(SPRITE_COLOR_DEPTH);
  screenSprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  onWifiDisconnected();
	mqttClient.setServer(server, port);
	mqttClient.setCallback(onMessageReceived);
}

void loop() {
	if(checkWiFi() && checkMQTT()) {
		mqttClient.loop();
	}
}
