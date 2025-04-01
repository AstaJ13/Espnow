#include <ESP8266WiFi.h>
#include <espnow.h>

// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0x48, 0xE7, 0x29, 0x65, 0x81, 0x5E};

// Set your Board ID
#define BOARD_ID 2

// MQ135 Sensor Setup
#define MQ135_PIN A0  // Analog pin for MQ135 sensor
#define RL_VALUE 10.0  // Load resistance in kilo-ohms (typically 10KΩ)
#define R0 76.63  // Baseline resistance of sensor in clean air (you need to calibrate this)

// Structure to send data (Fixed 16 bytes)
typedef struct __attribute__((packed)) struct_message {
    int id;               // 4 bytes
    float air_quality_ppm; // 4 bytes
    uint8_t padding[8];   // 8 bytes padding to make it exactly 16 bytes
} struct_message;

// Create a struct_message instance
struct_message myData;

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

// Function to convert raw analog value to PPM
float getPPM(int raw_adc) {
    float sensor_voltage = (raw_adc / 1023.0) * 3.3;  // Convert ADC to voltage
    float rs = ((3.3 * RL_VALUE) / sensor_voltage) - RL_VALUE;  // Calculate sensor resistance
    float ratio = rs / R0;  // Ratio (RS/R0)
    float ppm = 116.602 * pow(ratio, -2.769);  // MQ135 equation for CO2 PPM
    return ppm;
}

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
    Serial.print("\r\nLast Packet Send Status: ");
    Serial.println(sendStatus == 0 ? "✅ Delivery success" : "❌ Delivery failed");
}

void setup() {
    Serial.begin(115200);
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
        // Read MQ135 raw value and convert to PPM
        int raw_adc = analogRead(MQ135_PIN);
        myData.id = BOARD_ID;
        myData.air_quality_ppm = getPPM(raw_adc);
        memset(myData.padding, 0, sizeof(myData.padding)); // Ensure padding bytes are zeroed

        Serial.printf("📡 Sending -> ID: %d, Air Quality: %.2f PPM\n", myData.id, myData.air_quality_ppm);

        // Send message via ESP-NOW (16 bytes)
        esp_now_send(0, (uint8_t *) &myData, sizeof(myData));

        lastTime = millis();
    }
}
