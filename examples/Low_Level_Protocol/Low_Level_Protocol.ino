#include <Arduino.h>
#include "DMR828S_utils.h"

// Create low-level DMR828S utilities instance
DMR828S_Utils dmrUtils(Serial2);

void setup() {
    Serial.begin(115200);
    Serial.println("DMR828S Low-Level Protocol Example");
    
    // Initialize DMR communication
    Serial2.begin(57600, SERIAL_8N1, 16, 17);
    dmrUtils.begin(57600);
    dmrUtils.debug = true;
    dmrUtils.checksumEnabled = false;
    
    Serial.println("Low-level protocol access ready!");
}

void loop() {
    // Send a ping frame every 2 seconds (original behavior)
    static unsigned long lastPing = 0;
    if (millis() - lastPing > 2000) {
        lastPing = millis();
        
        uint8_t data[1] = { 1 };
        dmrUtils.sendFrame(0x24, 0x01, 0x01, data, 1);  // Get Radio ID
        Serial.println("🔍 Ping frame sent");
    }
    
    // Read any incoming frames
    DMRFrame frame;
    if (dmrUtils.readFrame(frame)) {
        Serial.println("📨 Frame received:");
        Serial.print("  CMD: 0x"); Serial.println(frame.cmd, HEX);
        Serial.print("  R/W: 0x"); Serial.println(frame.rw, HEX);
        Serial.print("  SR:  0x"); Serial.println(frame.sr, HEX);
        Serial.print("  LEN: "); Serial.println(frame.length);
        Serial.print("  VALID: "); Serial.println(frame.valid ? "YES" : "NO");
        
        if (frame.length > 0) {
            Serial.print("  DATA: ");
            for (int i = 0; i < frame.length; i++) {
                if (frame.data[i] < 0x10) Serial.print("0");
                Serial.print(frame.data[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
        Serial.println("  ──────────────");
    }
    
    delay(10);
}