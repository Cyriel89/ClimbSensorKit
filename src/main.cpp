#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include <PubSubClient.h>

const int DHTPIN = 14; // DHT sensor pin
const int DHTTYPE = DHT22; // DHT 22 (AM2302), AM2321
const int LEDPIN = 2; // Led pin
const int SHOCKLEDPIN = 19; // Shock Led pin
const int BUZZERPIN = 23; // Buzzer sensor pin
const int SHOCKPIN = 32; // Shock sensor pin
const char* ssid = "iPhone de Lucas";
const char* password = "babayaga";
const char* mqtt_server = "172.20.10.2";
//Create WiFiClient object
WiFiClient espClient;
// Create PubSubClient object
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
// Timer variables
unsigned long lastTime = 0;  
unsigned long lastTimeTemp = 0;
unsigned long lastTimeAcc = 0;
unsigned long gyroDelay = 10;
unsigned long tempDelay = 2000;
unsigned long accDelay = 200;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create an Event Source on /events
AsyncEventSource events("/events");
// Json variable to hold Sensor Readings
StaticJsonDocument<200> docReadings;
// Variable to store the HTTP request
String header;
// MPU6050 sensor
Adafruit_MPU6050 mpu;
// Event variables
sensors_event_t a, g, temp;
// MPU6050 sensor variables
float ax, ay, az;
float gx, gy, gz;
// DHT sensor
DHT dht(DHTPIN, DHTTYPE);
// DHT sensor variables
float h;
float t;

// Init MPU6050 sensor
void initMPU6050() {
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
}

// Init DHT sensor
void initDHT() {
  dht.begin();
  Serial.println("DHT22 Found!");
}

// Init Shock sensor
void initShock() {
  pinMode(SHOCKPIN, INPUT);
  pinMode(SHOCKLEDPIN, OUTPUT);
  digitalWrite(SHOCKLEDPIN, LOW);
}

// Init Buzzer sensor
void initBuzzer() {
  pinMode(BUZZERPIN, INPUT);
}

// Init buil-in LED
void initLED() {
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);
}

// Init SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
}

// Init WiFi
void initWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // Connect to Wi-Fi network with SSID and password
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LEDPIN, HIGH);
  }
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();
}

// Init MQTT
void initMQTT() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Reconnect to MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
// calculate radians to degrees
float radToDeg(float rad) {
  return rad * 180 / M_PI;
}


String getGyroReadings() {
  mpu.getEvent(&a, &g, &temp);
  gx = g.gyro.x;
  gy = g.gyro.y;
  gz = g.gyro.z;
  docReadings["gyroX"] = gx * 180 / M_PI;
  docReadings["gyroY"] = gy;
  docReadings["gyroZ"] = gz;
  String jsonString;
  serializeJson(docReadings, jsonString);
  return jsonString;
}

String getAccReadings() {
  mpu.getEvent(&a, &g, &temp);
  ax = a.acceleration.x;
  ay = a.acceleration.y;
  az = a.acceleration.z;
  docReadings["accX"] = ax;
  docReadings["accY"] = ay;
  docReadings["accZ"] = az;
  String jsonString;
  serializeJson(docReadings, jsonString);
  return jsonString;
}

String getTemperatureReadings() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  docReadings["hum"] = h;
  docReadings["temp"] = t;
  String jsonString;
  serializeJson(docReadings, jsonString);
  return jsonString;
}

void setup() {
  Serial.begin(115200);
  initMPU6050();
  initDHT();
  initShock();
  initBuzzer();
  initLED();
  initSPIFFS();
  initWiFi();
  initMQTT();

  File file = SPIFFS.open("/index.html");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("File Content:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
  // Send a GET request to <ESP_IP>/get?message=<message>
  server.on("/home", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Serves static files
  server.serveStatic("/", SPIFFS, "/");

  server.addHandler(&events);
  server.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long currentMillis = millis();
  if (currentMillis - lastMsg > 5000) {
    lastMsg = currentMillis;
    // Publish Temperature
    client.publish("esp32/temperature", docReadings["temp"].as<String>().c_str());
    // Publish Humidity
    client.publish("esp32/humidity", docReadings["hum"].as<String>().c_str());
    // Publish GyroX
    client.publish("esp32/gyroX", docReadings["gyroX"].as<String>().c_str());
    // Publish GyroY
    client.publish("esp32/gyroY", docReadings["gyroY"].as<String>().c_str());
    // Publish GyroZ
    client.publish("esp32/gyroZ", docReadings["gyroZ"].as<String>().c_str());
    // Publish AccX
    client.publish("esp32/accX", docReadings["accX"].as<String>().c_str());
    // Publish AccY
    client.publish("esp32/accY", docReadings["accY"].as<String>().c_str());
    // Publish AccZ
    client.publish("esp32/accZ", docReadings["accZ"].as<String>().c_str());
  }

  if((millis() - lastTime) > gyroDelay) {
    lastTime = millis();
    events.send(getGyroReadings().c_str(), "gyro_readings", millis());
    lastTime = millis();
  }

  if((millis() - lastTimeAcc) > accDelay) {
    lastTimeAcc = millis();
    events.send(getAccReadings().c_str(), "accelerometer_readings", millis());
    lastTimeAcc = millis();
  }

  if((millis() - lastTimeTemp) > tempDelay) {
    lastTimeTemp = millis();
    events.send(getTemperatureReadings().c_str(), "temperature_readings", millis());
    lastTimeTemp = millis();
  }
}