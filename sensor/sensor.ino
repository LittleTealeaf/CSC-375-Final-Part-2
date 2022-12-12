//https://wiki.dfrobot.com/URM09_Ultrasonic_Sensor_Gravity_Trig_SKU_SEN0388
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#define    VELOCITY_TEMP(temp)       ( ( 331.5 + 0.6 * (float)( temp ) ) * 100 / 1000000.0 ) // The ultrasonic velocity (cm/us) compensated by temperature

String WiFiPrefix = "Omega";
char WiFiPassword[] = "123456789";
Ticker wifiTicker;
bool wifiLock;

IPAddress server(192, 168, 137, 1);
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int16_t sensorPin = D11;
#define DISTANCE_PIN D11
uint32_t pulseWidthUs;

char packet[1024];

bool connected, active;
Ticker sensorTicker;

void clearWifiLock() {
  wifiLock = false;
}

bool connectWiFi() {
  int status = WiFi.status();

  if (status != WL_CONNECTED && !wifiLock) {
    int scanResult = WiFi.scanComplete();

    if (scanResult == -2) {
      WiFi.scanNetworks(true);
    }
    else if (scanResult >= 0) {
      for (int i = 0; i < scanResult; i++) {
        if (WiFi.SSID(i).indexOf(WiFiPrefix) == 0) {
          WiFi.begin(WiFi.SSID(i).begin(), WiFiPassword);
          wifiLock = true;
          wifiTicker.once(5, clearWifiLock);
        }
      }
    }
  }
  return status == WL_CONNECTED;
}

bool connectMQTT() {
  if (!mqttClient.connected()) {
    if (mqttClient.connect("takwashnak/firebeetle")) {
      mqttClient.subscribe("CSC375/control");
      return true;
    }
    return false;
  }
  return true;
}


void onMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.println("Recieved Packet");
  Serial.println(topic);
  if (strcmp(topic, "CSC375/control") == 0) {
    Serial.println("Is control topic");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    Serial.println("Deserialized");

    const char* mac = doc["MAC"];
    int value = doc["control"];

    Serial.println("Extracted Values");

    if(strcmp(mac, WiFi.macAddress().begin()) == 0) {
      Serial.println("Setting Activitiy");
      active = value == 1;
    }

  }
}

void readSensor() {
  if(connected && active) {
    int16_t dist, temp;
    pinMode(DISTANCE_PIN,OUTPUT);
    digitalWrite(DISTANCE_PIN,LOW);
    digitalWrite(DISTANCE_PIN,HIGH);
    delayMicroseconds(10);
    digitalWrite(DISTANCE_PIN,LOW);
    pinMode(DISTANCE_PIN,INPUT);
    pulseWidthUs = pulseIn(DISTANCE_PIN,HIGH);

    DynamicJsonDocument doc(1024);
    doc["MAC"] = WiFi.macAddress();
    doc["dist"] = (int) (pulseWidthUs * VELOCITY_TEMP(20) / 2.0);

    serializeJson(doc,packet,1024);

    mqttClient.publish("CSC375/dist",packet);

  }
}


void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  mqttClient.setServer(server, port);
  mqttClient.setCallback(onMessageReceived);

  pinMode(LED_BUILTIN, OUTPUT);

  active = true;

  sensorTicker.attach(1,readSensor);
}

void loop() {
  connected = connectWiFi() && connectMQTT();
  if (connected) {
    mqttClient.loop();
  }
  digitalWrite(LED_BUILTIN, connected && active ? HIGH : LOW);
}



// void setup() {
//   Serial.begin(115200);
//   delay(100);
// }

// void loop() {
//   int16_t  dist, temp;
//   pinMode(trigechoPin,OUTPUT);
//   digitalWrite(trigechoPin,LOW);

//   digitalWrite(trigechoPin,HIGH);//Set the trig pin High
//   delayMicroseconds(10);     //Delay of 10 microseconds
//   digitalWrite(trigechoPin,LOW); //Set the trig pin Low

//   pinMode(trigechoPin,INPUT);//Set the pin to input mode
//   pulseWidthUs = pulseIn(trigechoPin,HIGH);//Detect the high level time on the echo pin, the output high level time represents the ultrasonic flight time (unit: us)

//   distance = pulseWidthUs * VELOCITY_TEMP(20) / 2.0;//The distance can be calculated according to the flight time of ultrasonic wave,/
//                                                     //and the ultrasonic sound speed can be compensated according to the actual ambient temperature
//   Serial.print(distance, DEC);
//   Serial.println("cm");
//   delay(500);
// }
