#include <Arduino.h>
#include <WiFi.h>
#include "DMR828S.h"

// WiFi credentials (optional - for network features)
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// LED pin (built-in LED on most ESP32 boards)
const int LED_PIN = 2;

// DMR828S instances - both high-level and low-level
DMR828S dmrHighLevel(Serial2);          // High-level API
DMR828S_Utils dmrLowLevel(Serial2);     // Low-level API (for direct protocol access)

// Example mode selector
enum ExampleMode {
    MODE_LOW_LEVEL,     // Use low-level API directly
    MODE_HIGH_LEVEL,    // Use high-level convenience functions
    MODE_MIXED          // Use both APIs
};

ExampleMode currentMode = MODE_MIXED;

// Callback function for received data (high-level API)
void onDataReceived(const uint8_t *data, uint16_t length) {
    Serial.print("📡 Data received via callback: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)data[i]);
    }
    Serial.println();
}

// Callback function for status changes (high-level API)
void onStatusChanged(uint8_t status) {
    Serial.print("📊 Status changed: 0x");
    Serial.println(status, HEX);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("🚀 ESP32 DMR828S Refactored Library Demo");
    Serial.println("=========================================");
    
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize Serial2 for DMR communication
    Serial2.begin(57600, SERIAL_8N1, 16, 17); // RX2=GPIO16, TX2=GPIO17
    
    // Setup based on example mode
    switch (currentMode) {
        case MODE_LOW_LEVEL:
            setupLowLevel();
            break;
            
        case MODE_HIGH_LEVEL:
            setupHighLevel();
            break;
            
        case MODE_MIXED:
            setupMixed();
            break;
    }
    
    Serial.println("✅ DMR828S setup complete");
    
    // Optional: Connect to WiFi for network features
    connectToWiFi();
    
    Serial.println("🔄 Entering main loop...");
}

void setupLowLevel() {
    Serial.println("🔧 Configuring LOW-LEVEL mode");
    
    dmrLowLevel.begin(57600);
    dmrLowLevel.debug = true;
    dmrLowLevel.checksumEnabled = false;
    
    Serial.println("   - Direct protocol access enabled");
    Serial.println("   - Debug output enabled");
    Serial.println("   - Checksum validation disabled");
}

void setupHighLevel() {
    Serial.println("🔧 Configuring HIGH-LEVEL mode");
    
    dmrHighLevel.begin(57600);
    dmrHighLevel.enableDebug(true);
    dmrHighLevel.enableChecksum(false);
    dmrHighLevel.setDataReceivedCallback(onDataReceived);
    dmrHighLevel.setStatusChangedCallback(onStatusChanged);
    
    Serial.println("   - High-level API enabled");
    Serial.println("   - Event callbacks registered");
    
    // Test some high-level functions
    testHighLevelFunctions();
}

void setupMixed() {
    Serial.println("🔧 Configuring MIXED mode (both APIs available)");
    
    // Setup high-level API
    dmrHighLevel.begin(57600);
    dmrHighLevel.enableDebug(true);
    dmrHighLevel.enableChecksum(false);
    dmrHighLevel.setDataReceivedCallback(onDataReceived);
    dmrHighLevel.setStatusChangedCallback(onStatusChanged);
    
    Serial.println("   - High-level API ready");
    Serial.println("   - Low-level API accessible via dmrHighLevel.getLowLevel()");
}

void testHighLevelFunctions() {
    Serial.println("🧪 Testing high-level functions...");
    
    // Test connection status
    if (dmrHighLevel.isConnected()) {
        Serial.println("   ✅ DMR device is connected");
    } else {
        Serial.println("   ⚠️  DMR device connection status unknown");
    }
    
    // Test volume control
    Serial.println("   🔊 Testing volume control...");
    if (dmrHighLevel.setVolume(75)) {
        Serial.println("      Volume set to 75%");
        
        uint8_t volume;
        if (dmrHighLevel.getVolume(volume)) {
            Serial.print("      Current volume: ");
            Serial.print(volume);
            Serial.println("%");
        }
    }
    
    // Test channel selection
    Serial.println("   📻 Testing channel control...");
    if (dmrHighLevel.setChannel(5)) {
        Serial.println("      Channel set to 5");
        
        uint8_t channel;
        if (dmrHighLevel.getChannel(channel)) {
            Serial.print("      Current channel: ");
            Serial.println(channel);
        }
    }
}

void connectToWiFi() {
    Serial.println("📶 Connecting to WiFi (optional)...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Blink during connection
        attempts++;
        
        // Handle DMR communication during WiFi connection
        if (currentMode != MODE_LOW_LEVEL) {
            dmrHighLevel.update();
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n   ✅ WiFi connected!");
        Serial.print("   📍 IP address: ");
        Serial.println(WiFi.localIP());
        digitalWrite(LED_PIN, HIGH);
    } else {
        Serial.println("\n   ⚠️  WiFi connection failed (continuing without WiFi)");
        digitalWrite(LED_PIN, LOW);
    }
}

void loop() {
    switch (currentMode) {
        case MODE_LOW_LEVEL:
            loopLowLevel();
            break;
            
        case MODE_HIGH_LEVEL:
            loopHighLevel();
            break;
            
        case MODE_MIXED:
            loopMixed();
            break;
    }
    
    // Handle LED blinking
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 2000) {
        lastBlink = millis();
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    
    delay(10);
}

void loopLowLevel() {
    // Original low-level approach (similar to your existing code)
    static unsigned long lastPing = 0;
    if (millis() - lastPing > 2000) {
        lastPing = millis();
        
        // Send ping frame using low-level API
        uint8_t data[1] = { 1 };
        dmrLowLevel.sendFrame(0x24, 0x01, 0x01, data, 1);  // Get Radio ID
        
        Serial.println("🔍 Low-level ping sent");
    }
    
    // Read any response using low-level API
    DMRFrame frame;
    if (dmrLowLevel.readFrame(frame)) {
        Serial.println("📨 Low-level frame received:");
        Serial.print("   CMD: 0x"); Serial.println(frame.cmd, HEX);
        Serial.print("   LEN: "); Serial.println(frame.length);
        Serial.print("   VALID: "); Serial.println(frame.valid ? "✅" : "❌");
        Serial.print("   DATA: ");
        
        for (int i = 0; i < frame.length; i++) {
            if (frame.data[i] < 0x10) Serial.print("0");
            Serial.print(frame.data[i], HEX);
            Serial.print(" ");
        }
        Serial.println("\n   ──────────────────");
    }
}

void loopHighLevel() {
    // Handle incoming communication
    dmrHighLevel.update();
    
    // Periodic high-level operations
    static unsigned long lastOperation = 0;
    if (millis() - lastOperation > 5000) {
        lastOperation = millis();
        
        Serial.println("🔄 High-level periodic operations:");
        
        // Get device status
        uint8_t status;
        if (dmrHighLevel.getStatus(status)) {
            Serial.print("   📊 Device Status: 0x");
            Serial.println(status, HEX);
        }
        
        // Test data transmission
        String testMessage = "Hello from ESP32!";
        if (dmrHighLevel.transmitData((uint8_t*)testMessage.c_str(), testMessage.length())) {
            Serial.println("   📤 Test message transmitted");
        }
    }
}

void loopMixed() {
    // Use high-level API for most operations
    dmrHighLevel.update();
    
    static unsigned long lastMixedTest = 0;
    if (millis() - lastMixedTest > 8000) {
        lastMixedTest = millis();
        
        Serial.println("🔀 Mixed mode demonstration:");
        
        // High-level operation
        uint8_t channel;
        if (dmrHighLevel.getChannel(channel)) {
            Serial.print("   📻 Current channel (high-level): ");
            Serial.println(channel);
        }
        
        // Low-level operation using the same device
        DMR828S_Utils& lowLevel = dmrHighLevel.getLowLevel();
        uint8_t customData[] = {0xAA, 0xBB, 0xCC};
        
        Serial.println("   🔧 Sending custom frame (low-level):");
        if (lowLevel.sendFrame(0xFF, 0x01, 0x00, customData, sizeof(customData))) {
            Serial.println("      ✅ Custom frame sent successfully");
        }
        
        // Check for any responses
        DMRFrame response;
        if (lowLevel.readFrame(response)) {
            Serial.println("   📨 Low-level response received");
        }
    }
}