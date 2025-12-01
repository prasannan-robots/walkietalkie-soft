#include <Arduino.h>
#include "DMR828S.h"

// Create DMR828S instance using Serial2
DMR828S dmr(Serial2);

void setup() {
    Serial.begin(115200);
    Serial.println("DMR828S Simple Example");
    
    // Initialize DMR communication
    // RX2=GPIO16, TX2=GPIO17 on ESP32
    Serial2.begin(57600, SERIAL_8N1, 16, 17);
    dmr.begin(57600);
    dmr.enableDebug(true);
    
    // Test basic functions
    Serial.println("Testing DMR828S...");
    
    // Set volume to 50%
    if (dmr.setVolume(50)) {
        Serial.println("✅ Volume set to 50%");
    }
    
    // Set channel to 1
    if (dmr.setChannel(1)) {
        Serial.println("✅ Channel set to 1");
    }
    
    Serial.println("Setup complete!");
}

void loop() {
    // Handle incoming data
    dmr.update();
    
    // Send test message every 10 seconds
    static unsigned long lastTx = 0;
    if (millis() - lastTx > 10000) {
        lastTx = millis();
        
        String msg = "Test from ESP32";
        if (dmr.transmitData((uint8_t*)msg.c_str(), msg.length())) {
            Serial.println("📤 Test message sent");
        }
    }
    
    delay(100);
}