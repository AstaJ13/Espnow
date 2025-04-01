#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FirebaseESP8266.h>

// WiFi Credentials
#define WIFI_SSID "MITADTU Hostel"
#define WIFI_PASSWORD "Adtu@321"

// Firebase Credentials
#define FIREBASE_HOST "https://wirless-sensor-network-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "1NBiOxvu8sldRPVWMnnIRU4XkeHuQg6RqCHfRAvj"

// Firebase and WiFi Objects
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// Web Server on port 80
ESP8266WebServer server(80);

// Sensor Data
float temperature, humidity, airQuality;

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("🔗 Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("🌐 ESP8266 IP: ");
    Serial.println(WiFi.localIP());  // Show IP for frontend

    // Firebase Setup
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    Serial.println("🔥 Firebase Initialized!");

    // Setup Web Server Routes
    server.on("/data", HTTP_GET, sendSensorData);  // Serve sensor data
    server.begin();
    Serial.println("🚀 Web Server Started!");
}

void loop() {
    checkWiFiConnection();

    if (!firebaseData.httpConnected()) {  
        resetFirebaseConnection();  
    }

    readSensors();
    sendDataToFirebase();
    server.handleClient();  // Handle incoming HTTP requests

    delay(5000);  // Prevent excessive Firebase calls
}

// Reconnect WiFi if disconnected
void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ WiFi Disconnected! Reconnecting...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 10) {
            delay(1000);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ WiFi Reconnected!");
        } else {
            Serial.println("\n❌ WiFi Connection Failed!");
        }
    }
}

// Reset Firebase connection
void resetFirebaseConnection() {
    Serial.println("🔄 Resetting Firebase Connection...");
    Firebase.reconnectWiFi(true);
}

// Simulated Sensor Readings
void readSensors() {
    temperature = random(20, 35) + random(0, 99) / 100.0;
    humidity = random(40, 80) + random(0, 99) / 100.0;
    airQuality = random(10, 100) + random(0, 99) / 100.0;
}

// Send Data to Firebase
void sendDataToFirebase() {
    Serial.println("🚀 Uploading data to Firebase...");

    FirebaseJson json;
    json.set("temperature", temperature);
    json.set("humidity", humidity);
    json.set("airQuality", airQuality);

    if (Firebase.setJSON(firebaseData, "/sensorData", json)) {
        Serial.println("✅ Data sent successfully!");
    } else {
        Serial.print("❌ Firebase Error: ");
        Serial.println(firebaseData.errorReason());
    }
}

// Serve JSON Data to JavaScript
void sendSensorData() {
    String json = "{";
    json += "\"temperature\": " + String(temperature, 1) + ",";
    json += "\"humidity\": " + String(humidity, 1) + ",";
    json += "\"airQuality\": " + String(airQuality, 1);
    json += "}";

    server.send(200, "application/json", json);
}
