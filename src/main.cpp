#include <Arduino.h>
#include "DMR828S.h"
#include "BluetoothSerial.h"

// GSM module connected to Serial1 (GPIO25 RX, GPIO26 TX)
#define GSM_RX_PIN 25
#define GSM_TX_PIN 26

// DMR828S instance using high-level API
DMR828S dmr(Serial2);

// Bluetooth Serial instance
BluetoothSerial SerialBT;

// Demo mode selector
enum DemoMode {
    MODE_BASIC_TEST,
    MODE_WALKIE_FEATURES,
    MODE_LOW_LEVEL
};

DemoMode currentMode = MODE_WALKIE_FEATURES;

// Walkie-talkie state
struct {
    uint32_t myRadioID = 0x000001;
    uint8_t currentChannel = 1;
    uint8_t volume = 5;
} wtState;

// GPS state
struct {
    double latitude = 29.938971327453903;
    double longitude = 77.56449807342506;
    double lastLatitude = 29.938971327453903;
    double lastLongitude = 77.56449807342506;
    bool hasValidFix = false;
    bool hasLastLocation = false;
    unsigned long lastGPSRead = 0;
    
    // Continuous GPS transmission
    bool continuousMode = false;
    uint32_t targetID = 0;
    unsigned long intervalMinutes = 5;
    unsigned long lastTransmission = 0;
} gpsState;

// GSM state
struct {
    bool initialized = false;
    bool networkRegistered = false;
    String operatorName = "";
    int signalStrength = 0;
    String phoneNumber = "";
} gsmState;

// Function declarations
void setupBasicTest();
void setupWalkieFeatures();
void setupLowLevel();
void loopBasicTest();
void loopWalkieFeatures();
void loopLowLevel();
void handleBluetoothCommands();
void processCommand(Stream* stream, String command);
void showCommands();
void showCommandsTo(Stream* stream);
void showStatus();
void showStatusTo(Stream* stream);
void showDeviceInfo();
void showDeviceInfoTo(Stream* stream);
void readGPS();
void parseNMEA(String sentence);
void sendGPSLocation(Stream* stream, uint32_t targetID);
void handleContinuousGPS();
void initializeGSM();
bool waitForGSMResponse(String expectedResponse, unsigned long timeout);
void checkGSMNetwork();
void getGSMSignalStrength();
void sendGSMFallbackSMS(String phoneNumber, String message);

// Event callbacks
void onSMSReceived(const DMRSMSMessage &sms) {
    String output = "\n📩 SMS Received!\n";
    output += "From: 0x" + String(sms.sourceID, HEX) + "\n";
    output += "Type: " + String(sms.type == 1 ? "Private" : "Group") + "\n";
    output += "Message: " + String(sms.message) + "\n";
    
    // Send to Bluetooth only (Serial0 is used for GPS)
    SerialBT.print(output);
}

void onCallReceived(const DMRCallInfo &callInfo) {
    String output = "\n📞 Incoming Call!\n";
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

void onSMSStatus(uint32_t targetID, ::SMSSendStatus status) {
    String output = "\n📱 SMS Send Status:\n";
    output += "To: 0x" + String(targetID, HEX) + "\n";
    
    switch(status) {
        case ::SMS_SEND_SUCCESS:
            output += "Status: ✅ SUCCESS - Message delivered!\n";
            break;
        case ::SMS_SEND_FAILED:
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
        case ::SMS_SEND_TIMEOUT:
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

void setup() {
    // Initialize GPS on Serial0 (9600 baud is standard for most GPS modules)
    Serial.begin(9600); // GPS module baud rate
    delay(1000);
    
    // Initialize GSM module on Serial1
    Serial1.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    
    // Initialize Bluetooth
    SerialBT.begin("LittleBoyz"); // Bluetooth device name
    
    // Initialize GSM module
    initializeGSM();
    
    // Initialize GPS - Serial0 is now dedicated to GPS module
    // No USB debugging to avoid conflicts with GPS data
    
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
    
    switch (currentMode) {
        case MODE_BASIC_TEST:
            setupBasicTest();
            break;
        case MODE_WALKIE_FEATURES:
            setupWalkieFeatures();
            break;
        case MODE_LOW_LEVEL:
            setupLowLevel();
            break;
    }
    
    showCommands();
}

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
    Serial.println("🔧 Low-Level Protocol Mode");
    
    // Access low-level utilities for direct protocol control
    DMR828S_Utils& lowLevel = dmr.getLowLevel();
    lowLevel.debug = true;
    lowLevel.checksumEnabled = false;
    
    Serial.println("Low-level protocol access ready");
}

void loop() {
    // Always handle DMR events
    dmr.update();
    
    // switch (currentMode) {
    //     case MODE_BASIC_TEST:
    //         loopBasicTest();
    //         break;
    //     case MODE_WALKIE_FEATURES:
    //         loopWalkieFeatures();
    //         break;
    //     case MODE_LOW_LEVEL:
    //         loopLowLevel();
    //         break;
    // }
    
    // Handle GPS data
    readGPS();
    
    // Handle continuous GPS transmission
    handleContinuousGPS();
    
    // Handle Bluetooth commands only
    handleBluetoothCommands();
    
    delay(10);
}

void loopBasicTest() {
    static unsigned long lastTest = 0;
    if (millis() - lastTest > 2000) {
        lastTest = millis();
        
        // Test basic functions
        uint32_t radioID = dmr.getRadioID();
        uint8_t rssi = dmr.getRSSI();
        DMRModuleStatus status = dmr.getModuleStatus();
        
        Serial.print("📊 ID: 0x"); Serial.print(radioID, HEX);
        Serial.print(", RSSI: "); Serial.print(rssi);
        Serial.print(", Status: ");
        
        switch(status) {
            case STATUS_RECEIVING: Serial.println("RX"); break;
            case STATUS_TRANSMITTING: Serial.println("TX"); break;
            case STATUS_STANDBY: Serial.println("Standby"); break;
            default: Serial.println("Unknown"); break;
        }
    }
}

void loopWalkieFeatures() {
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        lastStatus = millis();
        
        // Show periodic status
        Serial.print("📊 Ch:"); Serial.print(wtState.currentChannel);
        Serial.print(", Vol:"); Serial.print(wtState.volume);
        Serial.print(", RSSI:"); Serial.print(dmr.getRSSI());
        
        DMRModuleStatus status = dmr.getModuleStatus();
        Serial.print(", Status:");
        switch(status) {
            case STATUS_RECEIVING: Serial.println("RX"); break;
            case STATUS_TRANSMITTING: Serial.println("TX"); break;
            case STATUS_STANDBY: Serial.println("Standby"); break;
            default: Serial.println("?"); break;
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
        
        Serial.println("🔍 Low-level ping sent");
    }
    
    // Read low-level responses
    DMRFrame frame;
    if (dmr.getLowLevel().readFrame(frame)) {
        Serial.println("📨 Low-level response:");
        Serial.print("  CMD: 0x"); Serial.println(frame.cmd, HEX);
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
        Serial.println("  ──────────");
    }
}

void handleBluetoothCommands() {
    if (SerialBT.available()) {
        String command = SerialBT.readStringUntil('\n');
        command.trim();
        processCommand(&SerialBT, command);
    }
}

void processCommand(Stream* stream, String command) {
    if (command.startsWith("sms ")) {
        // SMS command: "sms <hex_id> <message>"
        int firstSpace = command.indexOf(' ', 4);
        if (firstSpace > 0) {
            String idStr = command.substring(4, firstSpace);
            String message = command.substring(firstSpace + 1);
            uint32_t targetID = strtoul(idStr.c_str(), NULL, 16);
            
            if (dmr.sendSMS(targetID, message.c_str())) {
                stream->print("📤 SMS sent to 0x"); stream->print(targetID, HEX);
                stream->print(": "); stream->println(message);
            } else {
                stream->println("❌ SMS send failed");
            }
        }
    }
    else if (command.startsWith("call ")) {
        String idStr = command.substring(5);
        uint32_t targetID = strtoul(idStr.c_str(), NULL, 16);
        
        if (dmr.startCall(CALL_PRIVATE, targetID)) {
            stream->print("📞 Calling 0x"); stream->println(targetID, HEX);
        }
    }
    else if (command.startsWith("group ")) {
        String idStr = command.substring(6);
        uint32_t targetID = strtoul(idStr.c_str(), NULL, 16);
        
        if (dmr.startCall(CALL_GROUP, targetID)) {
            stream->print("📞 Group call to 0x"); stream->println(targetID, HEX);
        }
    }
    else if (command == "stop") {
        dmr.stopCall();
        stream->println("📞 Call stopped");
    }
    else if (command == "emergency") {
        if (dmr.sendEmergencyAlarm(0)) {
            stream->println("🚨 Emergency alert sent");
        }
    }
    else if (command.startsWith("channel ")) {
        int ch = command.substring(8).toInt();
        if (ch >= 1 && ch <= 16) {
            wtState.currentChannel = ch;
            if (dmr.setChannel(ch)) {
                stream->print("📻 Channel: "); stream->println(ch);
            }
        }
    }
    else if (command.startsWith("volume ")) {
        int vol = command.substring(7).toInt();
        if (vol >= 1 && vol <= 9) {
            wtState.volume = vol;
            if (dmr.setVolume(vol)) {
                stream->print("🔊 Volume: "); stream->println(vol);
            }
        }
    }
    else if (command.startsWith("radioid ")) {
        String idStr = command.substring(8);
        uint32_t radioID = strtoul(idStr.c_str(), NULL, 16);
        
        if (radioID > 0 && radioID <= 0xFFFFFF) {
            wtState.myRadioID = radioID;
            if (dmr.setRadioID(radioID)) {
                stream->print("🆔 Radio ID: 0x"); stream->println(radioID, HEX);
            } else {
                stream->println("❌ Failed to set Radio ID");
            }
        } else {
            stream->println("❌ Invalid Radio ID. Use hex format (1-FFFFFF)");
        }
    }
    else if (command == "encrypt on") {
        if (dmr.setEncryption(true)) {
            stream->println("🔒 Encryption: ON (using default key)");
        } else {
            stream->println("❌ Failed to enable encryption");
        }
    }
    else if (command == "encrypt off") {
        if (dmr.setEncryption(false)) {
            stream->println("🔓 Encryption: OFF");
        } else {
            stream->println("❌ Failed to disable encryption");
        }
    }
    else if (command.startsWith("encryptkey ")) {
        String keyStr = command.substring(11);
        keyStr.trim();
        
        if (keyStr.length() == 16) { // 8 bytes = 16 hex chars
            uint8_t encryptionKey[8];
            bool validKey = true;
            
            // Convert hex string to bytes
            for (int i = 0; i < 8; i++) {
                String byteStr = keyStr.substring(i * 2, i * 2 + 2);
                char* endPtr;
                long byteVal = strtol(byteStr.c_str(), &endPtr, 16);
                
                if (*endPtr != '\0' || byteVal < 0 || byteVal > 255) {
                    validKey = false;
                    break;
                }
                encryptionKey[i] = (uint8_t)byteVal;
            }
            
            if (validKey) {
                if (dmr.setEncryption(true, encryptionKey)) {
                    stream->print("🔐 Encryption: ON with custom key ");
                    for (int i = 0; i < 8; i++) {
                        if (encryptionKey[i] < 0x10) stream->print("0");
                        stream->print(encryptionKey[i], HEX);
                    }
                    stream->println();
                } else {
                    stream->println("❌ Failed to set encryption with custom key");
                }
            } else {
                stream->println("❌ Invalid encryption key. Use 16 hex digits (8 bytes)");
            }
        } else {
            stream->println("❌ Encryption key must be 16 hex digits (8 bytes)");
        }
    }
    else if (command == "encrypt status") {
        bool isEncrypted = dmr.getEncryptionStatus();
        stream->print("🔍 Encryption Status: ");
        stream->println(isEncrypted ? "ON 🔒" : "OFF 🔓");
    }
    else if (command == "status") {
        showStatusTo(stream);
    }
    else if (command == "info") {
        showDeviceInfoTo(stream);
    }
    else if (command == "help") {
        showCommandsTo(stream);
    }
    else if (command == "fallback") {
        stream->println("🔄 Manual fallback triggered - implement your backup SMS method here");
    }
    else if (command == "smsinfo") {
        stream->println("\n📊 SMS Status Info:");
        stream->println("Use this to check if waiting for SMS response");
    }
    else if (command == "bt") {
        // Bluetooth specific command
        stream->println("📶 Bluetooth Status: Connected");
        stream->println("Device Name: DMR828S-Walkie");
    }
    else if (command.startsWith("gps ")) {
        // GPS command: "gps <hex_id>"
        String idStr = command.substring(4);
        uint32_t targetID = strtoul(idStr.c_str(), NULL, 16);
        
        if (targetID > 0) {
            sendGPSLocation(stream, targetID);
        } else {
            stream->println("❌ Invalid target ID. Use: gps <hex_id>");
        }
    }
    else if (command.startsWith("gpsauto ")) {
        // Continuous GPS: "gpsauto <hex_id> <minutes>"
        int firstSpace = command.indexOf(' ', 8);
        if (firstSpace > 0) {
            String idStr = command.substring(8, firstSpace);
            String intervalStr = command.substring(firstSpace + 1);
            
            uint32_t targetID = strtoul(idStr.c_str(), NULL, 16);
            unsigned long intervalMin = intervalStr.toInt();
            
            if (targetID > 0 && intervalMin > 0 && intervalMin <= 1440) { // Max 24 hours
                gpsState.continuousMode = true;
                gpsState.targetID = targetID;
                gpsState.intervalMinutes = intervalMin;
                gpsState.lastTransmission = 0; // Send immediately
                
                stream->print("📍 GPS Auto-send started to 0x");
                stream->print(targetID, HEX);
                stream->print(" every ");
                stream->print(intervalMin);
                stream->println(" minutes");
            } else {
                stream->println("❌ Invalid parameters. Use: gpsauto <hex_id> <1-1440_minutes>");
            }
        } else {
            stream->println("❌ Format: gpsauto <hex_id> <minutes>");
        }
    }
    else if (command == "gpsstop") {
        gpsState.continuousMode = false;
        stream->println("📍 GPS Auto-send stopped");
    }
    else if (command == "gpsinfo") {
        stream->println("\n📍 GPS Status:");
        stream->print("Current: ");
        stream->print(gpsState.latitude, 6);
        stream->print(", ");
        stream->println(gpsState.longitude, 6);
        stream->print("Valid Fix: ");
        stream->println(gpsState.hasValidFix ? "YES" : "NO");
        stream->print("Auto-send: ");
        stream->println(gpsState.continuousMode ? "ON" : "OFF");
        if (gpsState.continuousMode) {
            stream->print("Target: 0x");
            stream->println(gpsState.targetID, HEX);
            stream->print("Interval: ");
            stream->print(gpsState.intervalMinutes);
            stream->println(" minutes");
        }
        
        // Show raw GPS data for 2 seconds
        stream->println("\n📡 Raw GPS Data (2 seconds):");
        stream->println("=============================");
        unsigned long startTime = millis();
        while (millis() - startTime < 2000) { // 2 seconds
            if (Serial.available()) {
                String rawData = Serial.readStringUntil('\n');
                rawData.trim();
                if (rawData.length() > 0) {
                    stream->println(rawData);
                }
            }
            delay(10); // Small delay to prevent blocking
        }
        stream->println("=============================");
    }
    else if (command.startsWith("raw ")) {
        // Raw DMR command: "raw <hex_bytes>"
        String hexData = command.substring(4);
        hexData.trim();
        
        if (hexData.length() > 0 && hexData.length() % 2 == 0) {
            // Convert hex string to bytes
            int dataLen = hexData.length() / 2;
            uint8_t* rawData = new uint8_t[dataLen];
            
            bool validHex = true;
            for (int i = 0; i < dataLen; i++) {
                String byteStr = hexData.substring(i * 2, i * 2 + 2);
                char* endPtr;
                long byteVal = strtol(byteStr.c_str(), &endPtr, 16);
                
                if (*endPtr != '\0' || byteVal < 0 || byteVal > 255) {
                    validHex = false;
                    break;
                }
                rawData[i] = (uint8_t)byteVal;
            }
            
            if (validHex) {
                stream->print("🔧 Sending raw command: ");
                for (int i = 0; i < dataLen; i++) {
                    if (rawData[i] < 0x10) stream->print("0");
                    stream->print(rawData[i], HEX);
                    stream->print(" ");
                }
                stream->println();
                
                // Send raw data and wait for response
                if (dmr.sendRawCommand(rawData, dataLen)) {
                    stream->println("✅ Raw command sent successfully");
                } else {
                    stream->println("❌ Raw command failed");
                }
            } else {
                stream->println("❌ Invalid hex format. Use: raw 68010000XX...");
            }
            
            delete[] rawData;
        } else {
            stream->println("❌ Invalid hex length. Must be even number of hex digits.");
        }
    }
    else if (command == "gsmstatus") {
        stream->println("\n📱 GSM Status:");
        stream->print("Initialized: ");
        stream->println(gsmState.initialized ? "YES" : "NO");
        stream->print("Network: ");
        stream->println(gsmState.networkRegistered ? "REGISTERED" : "NOT REGISTERED");
        stream->print("Signal: ");
        stream->print(gsmState.signalStrength);
        stream->println("/31");
        stream->print("Phone: ");
        stream->println(gsmState.phoneNumber.length() > 0 ? gsmState.phoneNumber : "Not set");
        if (gsmState.operatorName.length() > 0) {
            stream->print("Operator: ");
            stream->println(gsmState.operatorName);
        }
    }
    else if (command.startsWith("gsmphone ")) {
        String phone = command.substring(9);
        phone.trim();
        if (phone.length() > 0) {
            gsmState.phoneNumber = phone;
            stream->println("✅ Fallback phone number set: " + phone);
        } else {
            stream->println("❌ Invalid phone number");
        }
    }
    else if (command.startsWith("gsmsms ")) {
        int spaceIndex = command.indexOf(' ', 7);
        if (spaceIndex != -1) {
            String phone = command.substring(7, spaceIndex);
            String message = command.substring(spaceIndex + 1);
            phone.trim();
            message.trim();
            if (phone.length() > 0 && message.length() > 0) {
                sendGSMFallbackSMS(phone, message);
            } else {
                stream->println("❌ Invalid phone number or message");
            }
        } else {
            stream->println("❌ Format: gsmsms <number> <message>");
        }
    }
    else {
        stream->println("❓ Unknown command. Type 'help' for commands.");
    }
}

void showCommands() {
    showCommandsTo(&SerialBT);
}

void showCommandsTo(Stream* stream) {
    stream->println("\n📖 Available Commands:");
    stream->println("=======================");
    stream->println("Communication:");
    stream->println("  sms <hex_id> <message>  - Send SMS");
    stream->println("  call <hex_id>           - Private call");
    stream->println("  group <hex_id>          - Group call");
    stream->println("  stop                    - Stop current call");
    stream->println("  emergency               - Send SOS");
    stream->println();
    stream->println("Settings:");
    stream->println("  channel <1-16>          - Change channel");
    stream->println("  volume <1-9>            - Set volume");
    stream->println("  radioid <hex>           - Set radio ID");
    stream->println("  encrypt on/off          - Enable/disable encryption");
    stream->println("  encryptkey <16hex>      - Set encryption with custom key");
    stream->println("  encrypt status          - Check encryption status");
    stream->println();
    stream->println("GPS & Location:");
    stream->println("  gps <hex_id>            - Send GPS location once");
    stream->println("  gpsauto <hex_id> <min>  - Auto-send GPS every X minutes");
    stream->println("  gpsstop                 - Stop auto GPS transmission");
    stream->println("  gpsinfo                 - Show GPS status");
    stream->println();
    stream->println("Information:");
    stream->println("  status                  - Show status");
    stream->println("  info                    - Device info");
    stream->println("  smsinfo                 - SMS tracking status");
    stream->println("  fallback                - Manual fallback trigger");
    stream->println("  bt                      - Bluetooth status (BT only)");
    stream->println("  raw <hex>               - Send raw DMR command");
    stream->println("  help                    - Show commands");
    stream->println();
    stream->println("GSM Fallback:");
    stream->println("  gsmstatus               - Check GSM module status");
    stream->println("  gsmphone <number>       - Set fallback phone number");
    stream->println("  gsmsms <number> <msg>   - Send SMS via GSM directly");
    stream->println();
    stream->println("Examples:");
    stream->println("  sms 123 Hello World");
    stream->println("  call 456");
    stream->println("  channel 5");
    stream->println("  radioid 123456          - Set radio ID to 0x123456");
    stream->println("  gps 123                 - Send location to 0x123");
    stream->println("  gpsauto 123 10          - Auto-send to 0x123 every 10min");
    stream->println("  encrypt on              - Enable encryption");
    stream->println("  encryptkey 0102030405060708 - Custom encryption key");
    stream->println("  raw 68010101A9020110    - Send raw DMR frame");
    stream->println();
}

void showStatus() {
    showStatusTo(&Serial);
}

void showStatusTo(Stream* stream) {
    stream->println("\n📊 Current Status:");
    stream->println("==================");
    stream->print("Mode: ");
    switch(currentMode) {
        case MODE_BASIC_TEST: stream->println("Basic Test"); break;
        case MODE_WALKIE_FEATURES: stream->println("Full Features"); break;
        case MODE_LOW_LEVEL: stream->println("Low-Level"); break;
    }
    
    stream->print("Radio ID: 0x"); stream->println(wtState.myRadioID, HEX);
    stream->print("Channel: "); stream->println(wtState.currentChannel);
    stream->print("Volume: "); stream->println(wtState.volume);
    stream->print("RSSI: "); stream->println(dmr.getRSSI());
    
    DMRModuleStatus status = dmr.getModuleStatus();
    stream->print("Status: ");
    switch(status) {
        case STATUS_RECEIVING: stream->println("Receiving"); break;
        case STATUS_TRANSMITTING: stream->println("Transmitting"); break;
        case STATUS_STANDBY: stream->println("Standby"); break;
        default: stream->println("Unknown"); break;
    }
    stream->println();
}

void showDeviceInfo() {
    showDeviceInfoTo(&Serial);
}

void showDeviceInfoTo(Stream* stream) {
    stream->println("\n🔧 Device Information:");
    stream->println("=======================");
    
    String version = dmr.getFirmwareVersion();
    stream->print("Firmware: "); stream->println(version);
    
    uint32_t radioID = dmr.getRadioID();
    stream->print("Radio ID: 0x"); stream->println(radioID, HEX);
    
    bool encrypted = dmr.getEncryptionStatus();
    stream->print("Encryption: "); stream->println(encrypted ? "ON" : "OFF");
    
    bool initialized = dmr.getInitializationStatus();
    stream->print("Initialized: "); stream->println(initialized ? "YES" : "NO");
    
    DMRChannelParams params;
    if (dmr.getCurrentChannelParams(params)) {
        stream->println("\nChannel Parameters:");
        stream->print("  TX Freq: "); stream->print(params.txFreq); stream->println(" Hz");
        stream->print("  RX Freq: "); stream->print(params.rxFreq); stream->println(" Hz");
        stream->print("  Power: "); stream->println(params.power);
        stream->print("  Color Code: "); stream->println(params.colorCode);
        stream->print("  Time Slot: "); stream->println(params.timeSlot);
    }
    stream->println();
}

/********************************************************
 * GPS FUNCTIONS
 ********************************************************/

void readGPS() {
    // Read actual GPS data from Serial0
    while (Serial.available()) {
        String nmeaSentence = Serial.readStringUntil('\n');
        nmeaSentence.trim();
        
        if (nmeaSentence.length() > 0 && nmeaSentence.startsWith("$")) {
            parseNMEA(nmeaSentence);
            gpsState.lastGPSRead = millis();
        }
    }
    
    // Check for GPS timeout (no data for 30 seconds)
    if (millis() - gpsState.lastGPSRead > 30000) {
        gpsState.hasValidFix = false;
    }
}

void parseNMEA(String sentence) {
    // Parse NMEA sentences (GPGGA, GPRMC, etc.)
    sentence.trim();
    
    if (sentence.startsWith("$GPGGA") || sentence.startsWith("$GNGGA")) {
        // Example: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
        
        int commaIndex[15];
        int commaCount = 0;
        
        // Find all comma positions
        for (int i = 0; i < sentence.length() && commaCount < 15; i++) {
            if (sentence.charAt(i) == ',') {
                commaIndex[commaCount++] = i;
            }
        }
        
        if (commaCount >= 6) {
            // Extract fix quality (field 6, index 5)
            String fixQuality = sentence.substring(commaIndex[5] + 1, commaIndex[6]);
            
            if (fixQuality.toInt() > 0) {
                // Valid fix available
                gpsState.hasValidFix = true;
                
                // Extract latitude (field 2, index 1)
                String latStr = sentence.substring(commaIndex[1] + 1, commaIndex[2]);
                String latDir = sentence.substring(commaIndex[2] + 1, commaIndex[3]);
                
                // Extract longitude (field 4, index 3)  
                String lonStr = sentence.substring(commaIndex[3] + 1, commaIndex[4]);
                String lonDir = sentence.substring(commaIndex[4] + 1, commaIndex[5]);
                
                if (latStr.length() > 0 && lonStr.length() > 0) {
                    // Convert DDMM.MMMMM to decimal degrees
                    double lat = latStr.substring(0, 2).toDouble() + 
                                latStr.substring(2).toDouble() / 60.0;
                    if (latDir == "S") lat = -lat;
                    
                    double lon = lonStr.substring(0, 3).toDouble() + 
                                lonStr.substring(3).toDouble() / 60.0;
                    if (lonDir == "W") lon = -lon;
                    
                    // Update GPS coordinates
                    gpsState.lastLatitude = gpsState.latitude;
                    gpsState.lastLongitude = gpsState.longitude;
                    gpsState.latitude = lat;
                    gpsState.longitude = lon;
                    gpsState.hasLastLocation = true;
                }
            } else {
                gpsState.hasValidFix = false;
            }
        }
    }
}

void sendGPSLocation(Stream* stream, uint32_t targetID) {
    double lat, lon;
    String status;
    
    if (gpsState.hasValidFix) {
        lat = gpsState.latitude;
        lon = gpsState.longitude;
        status = "LIVE GPS";
    } else if (gpsState.hasLastLocation) {
        lat = gpsState.lastLatitude;
        lon = gpsState.lastLongitude;
        status = "LAST GPS";
    } else {
        lat = 29.938971327453903;
        lon = 77.56449807342506;
        status = "DEFAULT";
    }
    
    // Create GPS message
    String gpsMessage = "GPS " + status + ": ";
    gpsMessage += String(lat, 6) + "," + String(lon, 6);
    
    // Send via SMS
    if (dmr.sendSMS(targetID, gpsMessage.c_str())) {
        stream->print("📍 GPS sent to 0x");
        stream->print(targetID, HEX);
        stream->print(" (");
        stream->print(status);
        stream->print("): ");
        stream->print(lat, 6);
        stream->print(", ");
        stream->println(lon, 6);
    } else {
        stream->println("❌ GPS SMS send failed");
    }
}

void handleContinuousGPS() {
    if (!gpsState.continuousMode) return;
    
    unsigned long currentTime = millis();
    unsigned long intervalMs = gpsState.intervalMinutes * 60 * 1000; // Convert minutes to milliseconds
    
    // Check if it's time to send GPS
    if (gpsState.lastTransmission == 0 || 
        (currentTime - gpsState.lastTransmission) >= intervalMs) {
        
        gpsState.lastTransmission = currentTime;
        
        // Send GPS location
        double lat, lon;
        String status;
        
        if (gpsState.hasValidFix) {
            lat = gpsState.latitude;
            lon = gpsState.longitude;
            status = "AUTO-GPS";
        } else if (gpsState.hasLastLocation) {
            lat = gpsState.lastLatitude;
            lon = gpsState.lastLongitude;
            status = "AUTO-LAST";
        } else {
            lat = 29.938971327453903;
            lon = 77.56449807342506;
            status = "AUTO-DEFAULT";
        }
        
        // Create GPS message
        String gpsMessage = status + " " + String(gpsState.intervalMinutes) + "min: ";
        gpsMessage += String(lat, 6) + "," + String(lon, 6);
       
        // Send via SMS
        if (dmr.sendSMS(gpsState.targetID, gpsMessage.c_str())) {
            // Send confirmation to Bluetooth only (Serial0 is used for GPS)
            SerialBT.print("📍 Auto-GPS sent to 0x");
            SerialBT.print(gpsState.targetID, HEX);
            SerialBT.print(" (");
            SerialBT.print(status);
            SerialBT.print("): ");
            SerialBT.print(lat, 6);
            SerialBT.print(", ");
            SerialBT.println(lon, 6);
        }
    }
}

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
