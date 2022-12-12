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

enum DisplayContent { DISPLAY_MESSAGE, DISPLAY_SENSORS, DISPLAY_LOCKED };

IPAddress server(192, 168, 137, 1);
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

Ticker wifiTicker;
bool wifiLock;

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";

DisplayContent currentContent;
TFT_eSprite screen(&M5.Lcd);
TFT_eSprite viewPorts[] = {TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd),
                           TFT_eSprite(&M5.Lcd), TFT_eSprite(&M5.Lcd)};

typedef struct Sensor {
  String mac = "";
  long lastPingedAt;
  int dist;
  TFT_eSprite viewPort = TFT_eSprite(&M5.Lcd);
} Sensor;

Sensor sensors[6];
int sensorCount = 0;

void debug(const char* message) {
	mqttClient.publish("takwashnak/debug",message);
}

void pushViewPort(int index) {
  if (currentContent == DISPLAY_SENSORS) {
    int y = index < 3 ? SPRITE_PADDING : SPRITE_HEIGHT + SPRITE_PADDING * 3;
    int x = SPRITE_PADDING + (SPRITE_PADDING * 2 + SPRITE_WIDTH) * (index % 3);
    sensors[index].viewPort.pushSprite(x, y);
  }
}

void setDisplayMode(DisplayContent content) {
  if (content != currentContent) {
    currentContent = content;

    if (content == DISPLAY_SENSORS) {
      screen.fillScreen(TFT_LIGHTGREY);
      screen.pushSprite(0, 0);
      for (int i = 0; i < 6; i++) {
        pushViewPort(i);
      }
    }
  }
}

void displayMessage(const char *message) {
  setDisplayMode(DISPLAY_MESSAGE);
  screen.fillScreen(TFT_BLACK);
  screen.setCursor(0, 0);
  screen.setTextSize(2);
  screen.print(message);
  screen.pushSprite(0, 0);
}

void renderSensor(int index) {
  Sensor sensor = sensors[index];

  uint32_t background;

  if (sensor.lastPingedAt < millis() - 10000) {
    background = TFT_DARKGREY;
  } else if (sensor.dist > 100) {
    background = TFT_GREEN;
  } else if (sensor.dist > 50) {
    background = TFT_YELLOW;
  } else {
    background = TFT_RED;
  }

  sensor.viewPort.fillScreen(background);

  pushViewPort(index);
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

  if (!mqttClient.connected()) {
    displayMessage("Attempting to Connect to MQTT");
    if (mqttClient.connect("takwashnak/core2")) {
      displayMessage("Connected to MQTT");
      mqttClient.subscribe("CSC375/dist");
      return true;
    }
    return false;
  }

  return true;
}

void onMessageReceived(char *topic, byte *payload, unsigned int length) {
  if (strcmp(topic, "CSC375/dist") == 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    const char *mac = doc["MAC"];
    int distance = doc["dist"];

    int sensorIndex = -1;

    for (int i = 0; i < sensorCount; i++) {
      if (strcmp(sensors[i].mac.begin(),mac) == 0) {
        sensorIndex = i;
      }
    }

    if (sensorIndex == -1 && sensorCount < 6) {
      sensorIndex = sensorCount;
      sensorCount++;
    }

    if (sensorIndex == -1) {
      return;
    }

		sensors[sensorIndex].mac = mac;
		sensors[sensorIndex].lastPingedAt = millis();
		sensors[sensorIndex].dist = distance;
		renderSensor(sensorIndex);

    /*
    for(int i = 0; i < 6; i++) {
            if(sensors[i].mac.equals("") || sensors[i].mac.equals(mac)) {
                    sensors[i].mac = mac;
                    sensors[i].lastPingedAt = millis();
                    viewPorts[i].setCursor(0, 0);
                    viewPorts[i].printf("%d",distance);
                    pushViewPort(i);
                    break;
            }
    }q
    */
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
    sensors[i].viewPort.setColorDepth(SPRITE_COLOR_DEPTH);
    sensors[i].viewPort.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    sensors[i].viewPort.fillScreen(TFT_BLACK);
  }
  screen.setColorDepth(SPRITE_COLOR_DEPTH);
  screen.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void loop() {
  if (connectWiFi() && connectMQTT()) {
    setDisplayMode(DISPLAY_SENSORS);
    mqttClient.loop();
  }
}
