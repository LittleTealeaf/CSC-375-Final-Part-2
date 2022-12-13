#include <Arduino.h>
#include <ArduinoJson.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
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

#define SENSOR_UPDATE_MS 2000
#define SENSOR_DISCONNECT_MS 5000

IPAddress server(192, 168, 137, 1);
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

Ticker wifiTicker;
bool wifiLock;

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";

TFT_eSprite screen(&M5.Lcd);

TaskHandle_t asyncTaskHandler;

BLEScan *bleScan;
Ticker bleTicker;

typedef struct Sensor {
  String mac = "";
  long lastPingedAt;
  int dist;
  TFT_eSprite viewPort = TFT_eSprite(&M5.Lcd);
} Sensor;

Sensor sensors[6];
int sensorCount = 0;
Ticker sensorTicker;

bool connected = false, unlocked = false, priorUnlocked;

String message;

void debug(const char *message) {
  mqttClient.publish("takwashnak/debug", message);
}

void pushViewPort(int index) {
  if (connected && unlocked) {
    int y = index < 3 ? SPRITE_PADDING : SPRITE_HEIGHT + SPRITE_PADDING * 3;
    int x = SPRITE_PADDING + (SPRITE_PADDING * 2 + SPRITE_WIDTH) * (index % 3);
    sensors[index].viewPort.pushSprite(x, y);
  }
}


void pushMessage() {
	screen.fillScreen(TFT_BLACK);
	screen.setCursor(0, 0);
	screen.setTextSize(2);
	screen.print(message);
	screen.pushSprite(0, 0);
}

void setMessage(const char *newMessage) {
	message = newMessage;
	if(unlocked) {
		pushMessage();
	}
}

void pushLockScreen() {
	screen.fillScreen(TFT_BLACK);
	screen.setCursor(0, 0);
	screen.setTextSize(3);
	screen.print("LOCKED");
	screen.pushSprite(0, 0);
}


void renderSensor(int index) {
  Sensor sensor = sensors[index];

  uint32_t background;

  if (sensor.lastPingedAt < millis() - SENSOR_DISCONNECT_MS) {
    background = TFT_DARKGREY;
  } else if (sensor.dist > 100) {
    background = TFT_GREEN;
  } else if (sensor.dist > 50) {
    background = TFT_YELLOW;
  } else {
    background = TFT_RED;
  }

  sensor.viewPort.fillScreen(background);

  sensor.viewPort.setCursor(1, SPRITE_HEIGHT - 10);
  sensor.viewPort.print(sensor.mac);

  pushViewPort(index);
}

void pushSensorScreen() {
	screen.fillScreen(TFT_BLACK);
	screen.pushSprite(0, 0);
	for(int i = 0; i < 6; i++) {
		renderSensor(i);
	}
}

void clearWifiLock() { wifiLock = false; }

bool connectWiFi() {
  int status = WiFi.status();

  if (status != WL_CONNECTED && !wifiLock) {
    int scanResult = WiFi.scanComplete();

    if (scanResult == -2) {
      setMessage("Scanning Wifi Networks");
      WiFi.scanNetworks(true);
    } else if (scanResult >= 0) {
      for (int i = 0; i < scanResult; i++) {
        if (WiFi.SSID(i).indexOf(WiFiPrefix) == 0) {
          setMessage("Connecting to Network");
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
    setMessage("Connecting to MQTT");

		if(!mqttClient.connect("takwashnak/core2")) {
			return false;
		}
		mqttClient.subscribe("CSC375/dist");
  }

  return true;
}

bool connectBluetooth() {
	BLEScanResults results = bleScan->start(1);
	int count = results.getCount();
	for(int i = 0; i < count; i++) {
		BLEAdvertisedDevice device = results.getDevice(i);
		if(String(device.getServiceData().c_str()).indexOf("takwashnak/key") != -1) {
			return true;
		}
	}
	return false;
}

void onMessageReceived(char *topic, byte *payload, unsigned int length) {
  if (strcmp(topic, "CSC375/dist") == 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    const char *mac = doc["MAC"];
    int distance = doc["dist"];

    int sensorIndex = -1;

    for (int i = 0; i < sensorCount; i++) {
      if (strcmp(sensors[i].mac.begin(), mac) == 0) {
        sensorIndex = i;
      }
    }

    if (sensorIndex == -1) {
      if (sensorCount >= 6) {
        return;
      }
      sensorIndex = sensorCount;
      sensorCount++;
    }

    sensors[sensorIndex].mac = mac;
    sensors[sensorIndex].lastPingedAt = millis();
    sensors[sensorIndex].dist = distance;
    renderSensor(sensorIndex);
  }
}

void updateSensors() {
  if (connected && unlocked) {
    for (int i = 0; i < sensorCount; i++) {
      if (sensors[i].lastPingedAt < millis() - SENSOR_UPDATE_MS) {
        renderSensor(i);
      }
    }
  }
}

void asyncCode(void *pvParameters) {
	for(;;) {
		unlocked = connectBluetooth();	
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

  sensorTicker.attach_ms(SENSOR_UPDATE_MS, updateSensors);

  BLEDevice::init("");
  bleScan = BLEDevice::getScan();
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(99);

	xTaskCreatePinnedToCore(asyncCode, "async", 10000, NULL, 1, &asyncTaskHandler, 0);
}

void loop() {
	bool oldConnected = connected;
	connected = connectWiFi() && connectMQTT();
  if (connected) {
    mqttClient.loop();

		if(unlocked && !oldConnected) {
			pushLockScreen();
		}
  }

	if(!priorUnlocked && unlocked) {
		debug("UNLOCKED");
		if(connected) {
			pushSensorScreen();
		} else {
			pushMessage();
		}
	} else if(priorUnlocked && !unlocked) {
		debug("LOCKED");
		pushLockScreen();
	}
	priorUnlocked = unlocked;
}
