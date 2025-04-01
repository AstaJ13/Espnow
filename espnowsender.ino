#include <ESP8266WiFi.h>
#include <espnow.h>
#include <DHT.h>

// Receiver MAC Address
uint8_t broadcastAddress[] = {0x48, 0xE7, 0x29, 0x65, 0x81, 0x5E};

// Set Board ID
#define BOARD_ID 1

// DHT11 Sensor Setup
#define DHTPIN 4  // GPIO4 (D2 on NodeMCU)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Structure to send data (16 bytes total)
typedef struct __attribute__((packed)) struct_message {
    int id;              // 4 bytes
    float temperature;   // 4 bytes
    float humidity;      // 4 bytes
    uint8_t padding[4];  // 4 bytes padding to make total size 16 bytes
} struct_message;

// Create a struct_message instance
struct_message myData;

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// Callback function for sending status
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    Serial.print("\r\nLast Packet Send Status: ");
    Serial.println(sendStatus == 0 ? "✅ Delivery success" : "❌ Delivery failed");
}

void setup() {
    Serial.begin(115200);
    dht.begin(); // Initialize DHT sensor

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Init ESP-NOW
    if (esp_now_init() != 0) {
        Serial.println("❌ Error initializing ESP-NOW");
        return;
    }

    // Set ESP-NOW role
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);

    // Register receiver peer
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
    if ((millis() - lastTime) > timerDelay) {
        // Read temperature and humidity
        myData.id = BOARD_ID;
        myData.temperature = dht.readTemperature();
        myData.humidity = dht.readHumidity();
        memset(myData.padding, 0, sizeof(myData.padding)); // Ensure padding bytes are zeroed

        // Check if readings are valid
        if (isnan(myData.temperature) || isnan(myData.humidity)) {
            Serial.println("❌ Failed to read from DHT sensor!");
        } else {
            Serial.printf("📡 Sending Data -> ID: %d, Temp: %.2f°C, Humidity: %.2f%%\n",
                          myData.id, myData.temperature, myData.humidity);

            // Send message via ESP-NOW (16 bytes)
            esp_now_send(0, (uint8_t *) &myData, sizeof(myData));
        }

        lastTime = millis();
    }
}
