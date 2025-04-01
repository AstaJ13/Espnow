#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Wi-Fi credentials
const char* ssid = "Mitadtu_Staff";         // Replace with your Wi-Fi SSID
const char* password = "Mitadtu@2016";     // Replace with your Wi-Fi password

// MQTT broker details
const char* mqtt_server = "172.25.31.26";  // Replace with your MQTT broker IP
const int mqtt_port = 1883;                // Default MQTT port

// MQ135 sensor pin
#define MQ135_PIN A0   // Analog pin connected to MQ135

// Sensor parameters
#define RLOAD 10.0      // Load resistance on the sensor (in kΩ)
#define RZERO 76.63     // Pre-calibrated RZERO value (adjust as per your calibration)
#define PARA 116.6020682 // Pre-defined constant for CO2 curve
#define PARB 2.769034857 // Pre-defined constant for CO2 curve

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);

// Time tracking for publishing
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 5000; // Publish every 5 seconds

// Function to connect to Wi-Fi
void setup_wifi() {
  Serial.println("\nConnecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to reconnect to MQTT broker
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("NodeMCU_Client")) { // MQTT client ID
      Serial.println("Connected to MQTT broker.");
      client.subscribe("esp32/commands"); // Subscribe to a topic (if needed)
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

// Function to calculate resistance
float calculateResistance(int analogValue) {
  float voltage = analogValue * (5.0 / 1023.0); // Convert analog value to voltage
  float resistance = (5.0 - voltage) * RLOAD / voltage; // Calculate sensor resistance
  return resistance;
}

// Function to calculate CO2 PPM
float calculatePPM(float resistance) {
  return PARA * pow((resistance / RZERO), -PARB); // Using MQ135 CO2 curve equation
}

// Function to publish MQ135 data to MQTT
void publish_sensor_data() {
  int analogValue = analogRead(MQ135_PIN);
  float resistance = calculateResistance(analogValue);
  float ppm = calculatePPM(resistance);

  // Publish raw analog value
  char analogStr[8];
  dtostrf(analogValue, 6, 0, analogStr);
  if (client.publish("esp32/mq135/raw", analogStr)) {
    Serial.print("Raw analog value published: ");
    Serial.println(analogStr);
  } else {
    Serial.println("Raw analog value publish failed.");
  }

  // Publish CO2 PPM
  char ppmStr[8];
  dtostrf(ppm, 6, 2, ppmStr);
  if (client.publish("esp32/mq135/co2", ppmStr)) {
    Serial.print("CO2 PPM published: ");
    Serial.println(ppmStr);
  } else {
    Serial.println("CO2 PPM publish failed.");
  }
}

// Setup function
void setup() {
  Serial.begin(115200);

  // Setup Wi-Fi and MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
}

// Main loop
void loop() {
  // Ensure Wi-Fi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    setup_wifi();
  }

  // Ensure MQTT connection
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // Publish sensor data periodically
  unsigned long currentTime = millis();
  if (currentTime - lastPublishTime >= publishInterval) {
    lastPublishTime = currentTime;
    publish_sensor_data();
  }
}
