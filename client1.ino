#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <PubSubClient.h>
#include <DHT.h>

// Wi-Fi credentials
const char* ssid = "Mitadtu_Staff";         // Replace with your Wi-Fi SSID
const char* password = "Mitadtu@2016";      // Replace with your Wi-Fi password

// Firebase credentials
#define FIREBASE_HOST "wirless-sensor-network-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "qeyvl7tDbhUiXNQTavsJBy4usnYkHkz7hbeBjZRC"

FirebaseConfig config;  // Firebase configuration
FirebaseAuth auth;      // Firebase authentication
FirebaseData firebaseData; // Firebase data object

// MQTT broker details
const char* mqtt_server = "172.25.47.111";   // Replace with your Raspberry Pi's MQTT broker IP
const int mqtt_port = 1883;                 // Default MQTT port

// DHT sensor setup
#define DHTPIN D4    // GPIO2 (D4) on NodeMCU
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);

// Time tracking for publishing
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 5000; // Publish every 5 seconds

// Function to connect to Wi-Fi
void setup_wifi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Retry limit
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWi-Fi connection failed! Check credentials.");
  }
}

// Function to reconnect to MQTT broker
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("NodeMCU_Client")) { // MQTT client ID
      Serial.println("Connected to MQTT broker.");
      client.subscribe("esp8266/commands"); // Subscribe to a topic
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// MQTT callback function
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Function to publish DHT11 data to MQTT and Firebase
void publish_sensor_data() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Convert to strings
  char tempStr[8], humStr[8];
  dtostrf(temperature, 6, 2, tempStr);
  dtostrf(humidity, 6, 2, humStr);

  // Publish to MQTT
  client.publish("esp8266/temperature", tempStr);
  client.publish("esp8266/humidity", humStr);

  // Publish to Firebase
  if (Firebase.ready()) {
    String path = "/sensorData"; // Firebase path
    Firebase.setFloat(firebaseData, path + "/temperature", temperature);
    Firebase.setFloat(firebaseData, path + "/humidity", humidity);
    Serial.println("Data sent to Firebase.");
  } else {
    Serial.println("Firebase not ready.");
  }
}

// Setup function
void setup() {
  Serial.begin(115200);
  dht.begin();
  
  setup_wifi();

  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Configure MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
}

// Loop function
void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    lastPublishTime = currentMillis;
    publish_sensor_data();
  }
}


