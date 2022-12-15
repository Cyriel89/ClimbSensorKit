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

const int DHTPIN = 14; // what digital pin we're connected to
const int DHTTYPE = DHT22; // DHT 22 (AM2302), AM2321
const int LEDPIN = 19; // what digital pin we're connected to
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

// Init LED
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

String getGyroReadings() {
  mpu.getEvent(&a, &g, &temp);
  gx = g.gyro.x;
  gy = g.gyro.y;
  gz = g.gyro.z;
  docReadings["gx"] = gx;
  docReadings["gy"] = gy;
  docReadings["gz"] = gz;
  String jsonString;
  serializeJson(docReadings, jsonString);
  return jsonString;
}

String getAccReadings() {
  mpu.getEvent(&a, &g, &temp);
  ax = a.acceleration.x;
  ay = a.acceleration.y;
  az = a.acceleration.z;
  docReadings["ax"] = ax;
  docReadings["ay"] = ay;
  docReadings["az"] = az;
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
  pinMode(LEDPIN, OUTPUT);
  initMPU6050();
  initDHT();
  initLED();
  initSPIFFS();
  initWiFi();
  initMQTT();

  // Send a GET request to <ESP_IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Serve static files
  server.serveStatic("/", SPIFFS, "/");

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }

    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
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
    client.publish("esp32/gyroX", docReadings["gx"].as<String>().c_str());
    // Publish GyroY
    client.publish("esp32/gyroY", docReadings["gy"].as<String>().c_str());
    // Publish GyroZ
    client.publish("esp32/gyroZ", docReadings["gz"].as<String>().c_str());
    // Publish AccX
    client.publish("esp32/accX", docReadings["ax"].as<String>().c_str());
    // Publish AccY
    client.publish("esp32/accY", docReadings["ay"].as<String>().c_str());
    // Publish AccZ
    client.publish("esp32/accZ", docReadings["az"].as<String>().c_str());
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