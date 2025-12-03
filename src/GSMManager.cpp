#include "GSMManager.h"
#include "BluetoothSerial.h"

extern BluetoothSerial SerialBT;

GSMState gsmState;

void initializeGSM() {
    SerialBT.println("📱 Initializing GSM module...");
    
    // Reset GSM module
    Serial1.println("ATZ");
    delay(1000);
    
    // Check if GSM module responds
    Serial1.println("AT");
    delay(500);
    if (waitForGSMResponse("OK", 2000)) {
        SerialBT.println("✅ GSM module detected");
        
        // Set text mode for SMS
        Serial1.println("AT+CMGF=1");
        waitForGSMResponse("OK", 1000);
        
        // Check network registration
        checkGSMNetwork();
        
        gsmState.initialized = true;
        SerialBT.println("✅ GSM module initialized");
    } else {
        SerialBT.println("❌ GSM module not responding");
        gsmState.initialized = false;
    }
}

bool waitForGSMResponse(String expectedResponse, unsigned long timeout) {
    unsigned long startTime = millis();
    String response = "";
    
    while (millis() - startTime < timeout) {
        if (Serial1.available()) {
            response += (char)Serial1.read();
            if (response.indexOf(expectedResponse) != -1) {
                return true;
            }
        }
        delay(10);
    }
    return false;
}

void checkGSMNetwork() {
    // Check network registration
    Serial1.println("AT+CREG?");
    delay(500);
    
    String response = "";
    unsigned long startTime = millis();
    while (millis() - startTime < 2000 && Serial1.available()) {
        response += (char)Serial1.read();
        delay(10);
    }
    
    if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1) {
        gsmState.networkRegistered = true;
        SerialBT.println("✅ GSM network registered");
        
        // Get operator name
        Serial1.println("AT+COPS?");
        delay(500);
        // Read operator response (simplified)
        
        // Get signal strength
        getGSMSignalStrength();
    } else {
        gsmState.networkRegistered = false;
        SerialBT.println("❌ GSM network not registered");
    }
}

void getGSMSignalStrength() {
    Serial1.println("AT+CSQ");
    delay(500);
    
    String response = "";
    unsigned long startTime = millis();
    while (millis() - startTime < 1000 && Serial1.available()) {
        response += (char)Serial1.read();
        delay(10);
    }
    
    int csqIndex = response.indexOf("+CSQ: ");
    if (csqIndex != -1) {
        int commaIndex = response.indexOf(",", csqIndex);
        if (commaIndex != -1) {
            String rssiStr = response.substring(csqIndex + 6, commaIndex);
            int rssi = rssiStr.toInt();
            if (rssi != 99) {
                gsmState.signalStrength = rssi;
            }
        }
    }
}

void sendGSMFallbackSMS(String phoneNumber, String message) {
    if (!gsmState.initialized || !gsmState.networkRegistered) {
        SerialBT.println("❌ GSM not ready for SMS");
        return;
    }
    
    SerialBT.println("📱 Sending fallback SMS via GSM...");
    
    // Set SMS recipient
    Serial1.print("AT+CMGS=\"");
    Serial1.print(phoneNumber);
    Serial1.println("\"");
    delay(500);
    
    // Send message content
    Serial1.print(message);
    Serial1.write(0x1A); // Ctrl+Z to send
    delay(2000);
    
    if (waitForGSMResponse("OK", 10000)) {
        SerialBT.println("✅ Fallback SMS sent successfully");
    } else {
        SerialBT.println("❌ Failed to send fallback SMS");
    }
}