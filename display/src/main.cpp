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

// Connectivity Configuration

#define RSSI_CONNECT -50
#define RSSI_DISCONNECT -60

// Screen Configuration
#define COLOR_DEPTH 4
#define SCREEN_WIDTH 315
#define SCREEN_HEIGHT 240
#define SPRITE_WIDTH 103
#define SPRITE_HEIGHT 118
#define SPRITE_PADDING 1

// SENSOR CONNECTION

#define SENSOR_UPDATE_MS 1500
#define SENSOR_DISCONNECT_MS 5000

// MQTT Server Configuration
IPAddress mqttServer(192, 168, 137, 1);
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// WIFI Configuration

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";

Ticker wifiTicker;
bool wifiLock;

// Sensors
typedef struct Sensor {
  String mac;
  long lastPingedAt;
  int dist;
  TFT_eSprite viewPort = TFT_eSprite(&M5.Lcd);	
} Sensor;

Sensor sensors[6];
int sensorCount = 0;
Ticker sensorTicker;

// Misc.

TFT_eSprite screen(&M5.Lcd);
String connectionStatus;

char packet[1024];

bool connected, unlocked;

//"Exported" value from async code
bool exportUnlocked = false;

//// BEGIN ASYNC (CORE 0) CODE
TaskHandle_t asyncTaskHandler;

// Async Variables
bool asyncUnlocked, asyncUnlockedBuffer, asyncUpdated;
BLEScan *bleScan;

// Sets both the async and non-async unlocked value
void asyncSetUnlocked(bool value) {
  exportUnlocked = value;
  asyncUnlocked = value;
}

class AsyncAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) {
    if (String(device.getServiceData().c_str()).indexOf("takwashnak/key") !=
        -1) {
      if (asyncUnlocked && device.getRSSI() < RSSI_DISCONNECT) {
        if (!asyncUnlockedBuffer) {
          asyncSetUnlocked(false);
        }
        asyncUnlockedBuffer = false;
      } else if (device.getRSSI() > RSSI_CONNECT) {
        asyncUnlockedBuffer = true;
        asyncSetUnlocked(true);
      }
      asyncUpdated = true;
    }
  }
};

void asyncThread(void *params) {
  bleScan->setAdvertisedDeviceCallbacks(new AsyncAdvertisedDeviceCallbacks());
  for (;;) {
    asyncUpdated = false;
    BLEScanResults scanResults = bleScan->start(3, false);
    if (!asyncUpdated) {
      if (asyncUnlockedBuffer) {
        asyncUnlockedBuffer = false;
      } else {
        asyncSetUnlocked(false);
      }
    }
  }
}
//// END ASYNC (CORE 0) CODE

void pushConnectionStatus() {
  screen.fillScreen(TFT_BLACK);
  screen.setCursor(0, 0);
  screen.setTextSize(2);
  screen.print(connectionStatus);
  screen.pushSprite(0, 0);
}

void pushLockedScreen() {
  screen.fillScreen(TFT_BLACK);
  screen.setCursor(0, 0);
  screen.setTextSize(4);
  screen.print("LOCKED");
  screen.pushSprite(0, 0);
}

void pushSensorBackground() {
  screen.fillScreen(TFT_BLACK);
  screen.pushSprite(0, 0);
}

void pushSensor(int i) {
  int y = i < 3 ? SPRITE_PADDING : SPRITE_HEIGHT + SPRITE_PADDING * 3;
  int x = SPRITE_PADDING + (SPRITE_PADDING * 2 + SPRITE_WIDTH) * (i % 3);
  sensors[i].viewPort.pushSprite(x, y);
}

int getPositionIndex(int x, int y) {
	int index = 0;
	if(y >= SPRITE_HEIGHT + SPRITE_PADDING * 2) {
		index += 3;
	}
	if(x >= SPRITE_WIDTH + SPRITE_PADDING * 2) {
		index += 1;
	}
	if(x >= SPRITE_WIDTH * 2 + SPRITE_PADDING * 4) {
		index += 1;
	}
	return index;
}

void setConnectionStatus(const char *newConnectionStatus) {
  connectionStatus = newConnectionStatus;
  if (!connected && unlocked) {
    pushConnectionStatus();
  }
}

void renderSensor(int index) {

  uint32_t background;

  if (sensors[index].lastPingedAt < millis() - SENSOR_DISCONNECT_MS) {
    background = TFT_DARKGREY;
  } else if (sensors[index].dist > 100) {
    background = TFT_GREEN;
  } else if (sensors[index].dist > 50) {
    background = TFT_YELLOW;
  } else {
    background = TFT_RED;
  }

  sensors[index].viewPort.fillScreen(background);

  sensors[index].viewPort.setCursor(1, SPRITE_HEIGHT - 10);
  sensors[index].viewPort.setTextSize(1);
  sensors[index].viewPort.print(sensors[index].mac);

  if (connected && unlocked) {
    pushSensor(index);
  }
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

void onTouch(Event& e) {
	if(connected && unlocked) {
		int index = getPositionIndex(e.to.x, e.to.y);
		if(index < sensorCount) {
			DynamicJsonDocument doc(1024);
			doc["MAC"] = sensors[index].mac;
			doc["control"] = sensors[index].lastPingedAt < millis() - SENSOR_DISCONNECT_MS ? 1 : 0;

			serializeJson(doc, packet);

			mqttClient.publish("CSC375/control", packet);
			
		}
	}
}

void updateSensors() {
  for (int i = 0; i < sensorCount; i++) {
    if (sensors[i].lastPingedAt < millis() - SENSOR_UPDATE_MS) {
      renderSensor(i);
    }
  }
}


void clearWiFiLock() { wifiLock = false; }

bool connectWiFi() {
  int status = WiFi.status();

  if (status != WL_CONNECTED && !wifiLock) {
    int scanResult = WiFi.scanComplete();
    if (scanResult == -2) {
      setConnectionStatus("Scanning WiFi Networks");
      WiFi.scanNetworks(true);
    } else if (scanResult >= 0) {
      for (int i = 0; i < scanResult; i++) {
        if (WiFi.SSID(i).indexOf(WiFiPrefix) == 0) {
          setConnectionStatus("Connecting to Network");
          WiFi.begin(WiFi.SSID(i).begin(), WiFiPassword);
          wifiLock = true;
          wifiTicker.once(5, clearWiFiLock);
        }
      }
      WiFi.scanDelete();
    }
  }
  return status == WL_CONNECTED;
}

bool connectMQTT() {
  if (!mqttClient.connected()) {
    setConnectionStatus("Connecting to MQTT");
    if (!mqttClient.connect("takwashnak/core2")) {
      setConnectionStatus("Failed Connecting MQTT");
      return false;
    }
    mqttClient.subscribe("CSC375/dist");
  }
  return true;
}

void setup() {
  M5.begin();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(onMessageReceived);

  wifiLock = false;

  for (int i = 0; i < 6; i++) {
    sensors[i].viewPort.setColorDepth(COLOR_DEPTH);
    sensors[i].viewPort.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    sensors[i].viewPort.fillScreen(TFT_BLACK);
  }
  screen.setColorDepth(COLOR_DEPTH);
  screen.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

  sensorTicker.attach_ms(SENSOR_UPDATE_MS, updateSensors);

  BLEDevice::init("");
  bleScan = BLEDevice::getScan();
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(99);

  xTaskCreatePinnedToCore(asyncThread, "async", 10000, NULL, 1,
                          &asyncTaskHandler, 0);
  connected = false;
  unlocked = true;
}

void loop() {
	M5.update();
  bool isConnected = connectWiFi() && connectMQTT();
  bool connectChange = isConnected != connected;
  bool unlockChanged = exportUnlocked != unlocked;
  connected = isConnected;
  unlocked = exportUnlocked;

  if (isConnected) {
    if (connectChange && unlocked) {
      pushSensorBackground();
      for (int i = 0; i < 6; i++) {
        pushSensor(i);
      }
    }
    mqttClient.loop();
  } else {
    if (connectChange && unlocked) {
      pushConnectionStatus();
    }
  }

  if (unlockChanged) {
    if (!unlocked) {
      pushLockedScreen();
    } else {
      if (connected) {
        pushSensorBackground();
        for (int i = 0; i < 6; i++) {
          pushSensor(i);
        }
      } else {
        pushConnectionStatus();
      }
    }
  }

	Event& e = M5.Buttons.event;
	if(e.type == E_RELEASE) {
		onTouch(e);
	}
}
