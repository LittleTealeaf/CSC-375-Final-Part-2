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

IPAddress server(192, 168, 4, 2);
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

Ticker wifiTicker;
bool wifiLock;

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";

TFT_eSprite screen(&M5.Lcd);
TFT_eSprite viewPorts[] = {TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd)};

typedef struct Sensor {
	String mac = "";
	long lastPingedAt;
} Sensor;

Sensor sensors[6];

void displayMessage(const char *message) {
	screen.fillScreen(TFT_BLACK);
	screen.setCursor(0, 0);
	screen.print(message);
	screen.pushSprite(0, 0);
}

void pushViewPort(int index) {
  int x = index < 3 ? SPRITE_PADDING : SPRITE_HEIGHT + SPRITE_PADDING * 3;
  int y = SPRITE_PADDING + (SPRITE_PADDING * 2 + SPRITE_WIDTH) * (index % 3);
	viewPorts[index].setCursor(0, 0);
  viewPorts[index].pushSprite(x, y);
}

void clearWifiLock() { wifiLock = false; }

bool connectWiFi() {
  int status = WiFi.status();

  if (status != WL_CONNECTED && !wifiLock) {
    int scanResult = WiFi.scanComplete();

    if (scanResult == -2) {
			displayMessage("Scanning Wifi Networks");
      WiFi.scanNetworks(true);
    } else if (scanResult >= 0) {
      for (int i = 0; i < scanResult; i++) {
        if (WiFi.SSID(i).indexOf(WiFiPrefix) == 0) {
					displayMessage("Connecting to Network");
          WiFi.begin(WiFi.SSID(i).begin(), WiFiPassword);
          wifiLock = true;
          wifiTicker.once(5, clearWifiLock);
        }
      }
			WiFi.scanDelete();
    }
  }

  return status == WL_CONNECTED;
}

bool connectMQTT() {

	if(!mqttClient.connected()) {
		displayMessage("Attempting to Connect to MQTT");
		if(mqttClient.connect("takwashnak")) {
			displayMessage("Connected to MQTT");
			mqttClient.subscribe("CSC375/dist");
			return true;
		}
		return false;
	}

	return true;
}

void onMessageReceived(char *topic, byte *payload, unsigned int length) {
	DynamicJsonDocument doc(1024);
	deserializeJson(doc, payload);

	const char* mac = doc["MAC"];
	int distance = doc["dist"];

	for(int i = 0; i < 6; i++) {
		if(sensors[i].mac.equals("") || sensors[i].mac.equals(mac)) {
			sensors[i].mac = mac;
			sensors[i].lastPingedAt = millis();
			viewPorts[i].setCursor(0, 0);
			viewPorts[i].printf("%d",distance);
			pushViewPort(i);
			break;
		}
	}

}

void setup() {
  M5.begin();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  mqttClient.setServer(server, port);
  mqttClient.setCallback(onMessageReceived);

  wifiLock = false;

  for (int i = 0; i < 6; i++) {
    viewPorts[i].setColorDepth(SPRITE_COLOR_DEPTH);
    viewPorts[i].createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    viewPorts[i].fillScreen(TFT_DARKGREY);
  }
  screen.setColorDepth(SPRITE_COLOR_DEPTH);
  screen.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void loop() {
  if (connectWiFi() && connectMQTT()) {
		mqttClient.loop();
  }
}
