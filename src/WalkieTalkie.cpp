#include "WalkieTalkie.h"
#include "GPSManager.h"
#include "GSMManager.h"
#include "DisplayManager.h"
#include "KeyboardManager.h"

// Global instances
DMR828S dmr(Serial2);
BluetoothSerial SerialBT;
WalkieTalkieState wtState;
DemoMode currentMode = MODE_WALKIE_FEATURES;

void initializeSystem() {
    // Initialize GSM module on Serial1
    Serial1.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    
    // Initialize Bluetooth
    SerialBT.begin("LittleBoyz"); // Bluetooth device name
    
    // Initialize GSM module
    initializeGSM();
    
    // Initialize GPS
    initializeGPS();
    
    // Initialize I2C Display and Keyboard
    initializeDisplay();
    initializeKeyboard();
    
    // Initialize DMR module
    Serial2.begin(57600, SERIAL_8N1, 16, 17); // RX2=GPIO16, TX2=GPIO17
    dmr.begin(57600);
    dmr.enableDebug(true);
    dmr.enableChecksum(false);
    
    // Set event callbacks
    dmr.setSMSReceivedCallback(onSMSReceived);
    dmr.setSMSSendStatusCallback(onSMSStatus);
    dmr.setCallReceivedCallback(onCallReceived);
    dmr.setCallEndedCallback(onCallEnded);
    dmr.setEmergencyCallback(onEmergency);
    
    delay(2000); // Wait for module to initialize
}

// DMR Event Callbacks
void onSMSReceived(const DMRSMSMessage& message) {
    String output = "\n📨 SMS Received:\n";
    output += "From: 0x" + String(message.sourceID, HEX) + "\n";
    output += "Message: " + String(message.message) + "\n";
    SerialBT.print(output);
}

void onCallReceived(const DMRCallInfo& callInfo) {
    String output = "\n📞 Incoming Call:\n";
    output += "From: 0x" + String(callInfo.contactID, HEX) + "\n";
    output += "Type: ";
    
    switch(callInfo.type) {
        case CALL_PRIVATE: output += "Private"; break;
        case CALL_GROUP: output += "Group"; break;
        case CALL_ALL: output += "All"; break;
        default: output += "Unknown"; break;
    }
    output += "\n";
    SerialBT.print(output);
}

void onCallEnded() {
    SerialBT.println("📞 Call Ended");
}

void onEmergency(uint32_t sourceID) {
    String output = "\n🚨 Emergency Alert!\n";
    output += "From: 0x" + String(sourceID, HEX) + "\n";
    SerialBT.print(output);
}

void onSMSStatus(uint32_t targetID, SMSSendStatus status) {
    String output = "\n📱 SMS Send Status:\n";
    output += "To: 0x" + String(targetID, HEX) + "\n";
    
    switch(status) {
        case SMS_SEND_SUCCESS:
            output += "Status: ✅ SUCCESS - Message delivered!\n";
            break;
        case SMS_SEND_FAILED:
            output += "Status: ❌ FAILED - Trying GSM fallback...\n";
            SerialBT.print(output);
            
            // GSM fallback - send to predefined emergency number
            if (gsmState.phoneNumber.length() > 0) {
                String fallbackMsg = "EMERGENCY: VHF Radio SMS failed. Target: 0x" + String(targetID, HEX);
                fallbackMsg += ". Last known position: " + String(gpsState.latitude, 6) + ", " + String(gpsState.longitude, 6);
                sendGSMFallbackSMS(gsmState.phoneNumber, fallbackMsg);
            } else {
                SerialBT.println("❌ No fallback phone number configured");
            }
            return;
        case SMS_SEND_TIMEOUT:
            output += "Status: ⏰ TIMEOUT - Trying GSM fallback...\n";
            SerialBT.print(output);
            
            // GSM fallback for timeout
            if (gsmState.phoneNumber.length() > 0) {
                String fallbackMsg = "TIMEOUT: VHF Radio SMS timeout. Target: 0x" + String(targetID, HEX);
                fallbackMsg += ". Last known position: " + String(gpsState.latitude, 6) + ", " + String(gpsState.longitude, 6);
                sendGSMFallbackSMS(gsmState.phoneNumber, fallbackMsg);
            } else {
                SerialBT.println("❌ No fallback phone number configured");
            }
            return;
    }
    
    // Send to Bluetooth only (Serial0 is used for GPS)
    SerialBT.print(output);
}

// Setup functions for different modes
void setupBasicTest() {
    // Basic configuration
    dmr.setRadioID(wtState.myRadioID);
    dmr.setChannel(wtState.currentChannel);
    dmr.setVolume(wtState.volume);
}

void setupWalkieFeatures() {
    // Complete walkie-talkie setup
    dmr.setRadioID(wtState.myRadioID);
    dmr.setChannel(wtState.currentChannel);
    dmr.setVolume(wtState.volume);
    
    // Additional settings
    dmr.setColorCode(1);
    dmr.setTimeSlot(1);
    dmr.setTXPower(3);
    dmr.setMicGain(8);
    dmr.setSQLLevel(5);
}

void setupLowLevel() {
    SerialBT.println("🔧 Low-Level Protocol Mode");
    
    // Access low-level utilities for direct protocol control
    DMR828S_Utils& lowLevel = dmr.getLowLevel();
    lowLevel.debug = true;
    lowLevel.checksumEnabled = false;
    
    SerialBT.println("Low-level protocol access ready");
}

// Loop functions for different modes
void loopBasicTest() {
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        lastStatus = millis();
        
        // Show periodic status
        SerialBT.print(", Ch:"); SerialBT.print(wtState.currentChannel);
        SerialBT.print(", Vol:"); SerialBT.print(wtState.volume);
        SerialBT.print(", RSSI:"); SerialBT.print(dmr.getRSSI());
        
        DMRModuleStatus status = dmr.getModuleStatus();
        SerialBT.print(", Status:");
        switch(status) {
            case STATUS_RECEIVING: SerialBT.println("RX"); break;
            case STATUS_TRANSMITTING: SerialBT.println("TX"); break;
            case STATUS_STANDBY: SerialBT.println("Standby"); break;
            default: SerialBT.println("?"); break;
        }
    }
}

void loopWalkieFeatures() {
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        lastStatus = millis();
        
        // Show periodic status
        SerialBT.print("📊 Ch:"); SerialBT.print(wtState.currentChannel);
        SerialBT.print(", Vol:"); SerialBT.print(wtState.volume);
        SerialBT.print(", RSSI:"); SerialBT.print(dmr.getRSSI());
        
        DMRModuleStatus status = dmr.getModuleStatus();
        SerialBT.print(", Status:");
        switch(status) {
            case STATUS_RECEIVING: SerialBT.println("RX"); break;
            case STATUS_TRANSMITTING: SerialBT.println("TX"); break;
            case STATUS_STANDBY: SerialBT.println("Standby"); break;
            default: SerialBT.println("?"); break;
        }
    }
}

void loopLowLevel() {
    static unsigned long lastPing = 0;
    if (millis() - lastPing > 2000) {
        lastPing = millis();
        
        // Send low-level ping using direct protocol access
        DMR828S_Utils& lowLevel = dmr.getLowLevel();
        uint8_t data[1] = { 1 };
        lowLevel.sendFrame(0x24, 0x01, 0x01, data, 1);  // Get Radio ID
        
        SerialBT.println("🔍 Low-level ping sent");
    }
    
    // Read low-level responses
    DMRFrame frame;
    if (dmr.getLowLevel().readFrame(frame)) {
        SerialBT.println("📨 Low-level response:");
        SerialBT.print("  CMD: 0x"); SerialBT.println(frame.cmd, HEX);
        SerialBT.print("  LEN: "); SerialBT.println(frame.length);
        SerialBT.print("  VALID: "); SerialBT.println(frame.valid ? "YES" : "NO");
        
        if (frame.length > 0) {
            SerialBT.print("  DATA: ");
            for (int i = 0; i < frame.length; i++) {
                if (frame.data[i] < 0x10) SerialBT.print("0");
                SerialBT.print(frame.data[i], HEX);
                SerialBT.print(" ");
            }
            SerialBT.println();
        }
        SerialBT.println("  ──────────");
    }
}

void handleBluetoothCommands() {
    if (SerialBT.available()) {
        String command = SerialBT.readStringUntil('\n');
        command.trim();
        processCommand(&SerialBT, command);
    }
}