#include <Arduino.h>
#include "DMR828S.h"
#include "BluetoothSerial.h"

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

// Function declarations
void setupBasicTest();
void setupWalkieFeatures();
void setupLowLevel();
void loopBasicTest();
void loopWalkieFeatures();
void loopLowLevel();
void handleSerialCommands();
void handleBluetoothCommands();
void processCommand(Stream* stream, String command);
void showCommands();
void showCommandsTo(Stream* stream);
void showStatus();
void showStatusTo(Stream* stream);
void showDeviceInfo();
void showDeviceInfoTo(Stream* stream);

// Event callbacks
void onSMSReceived(const DMRSMSMessage &sms) {
    String output = "\n📩 SMS Received!\n";
    output += "From: 0x" + String(sms.sourceID, HEX) + "\n";
    output += "Type: " + String(sms.type == 1 ? "Private" : "Group") + "\n";
    output += "Message: " + String(sms.message) + "\n";
    
    // Send to both Serial and Bluetooth
    Serial.print(output);
    SerialBT.print(output);
}

void onCallReceived(const DMRCallInfo &callInfo) {
    Serial.println("\n📞 Incoming Call!");
    Serial.print("From: 0x"); Serial.println(callInfo.contactID, HEX);
    Serial.print("Type: ");
    switch(callInfo.type) {
        case CALL_PRIVATE: Serial.println("Private"); break;
        case CALL_GROUP: Serial.println("Group"); break;
        case CALL_ALL: Serial.println("All"); break;
        default: Serial.println("Unknown"); break;
    }
}

void onCallEnded() {
    Serial.println("📞 Call Ended");
}

void onEmergency(uint32_t sourceID) {
    Serial.println("\n🚨 Emergency Alert!");
    Serial.print("From: 0x"); Serial.println(sourceID, HEX);
}

void onSMSStatus(uint32_t targetID, ::SMSSendStatus status) {
    String output = "\n📱 SMS Send Status:\n";
    output += "To: 0x" + String(targetID, HEX) + "\n";
    
    switch(status) {
        case ::SMS_SEND_SUCCESS:
            output += "Status: ✅ SUCCESS - Message delivered!\n";
            break;
        case ::SMS_SEND_FAILED:
            output += "Status: ❌ FAILED - Trying fallback method...\n";
            output += "🔄 Implementing fallback SMS delivery...\n";
            break;
        case ::SMS_SEND_TIMEOUT:
            output += "Status: ⏰ TIMEOUT - No response received\n";
            output += "🔄 Implementing fallback SMS delivery...\n";
            break;
    }
    
    // Send to both Serial and Bluetooth
    Serial.print(output);
    SerialBT.print(output);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("🎙️ DMR828S Comprehensive Walkie-Talkie Library");
    Serial.println("===============================================");
    
    // Initialize Bluetooth
    SerialBT.begin("FATMAN"); // Bluetooth device name
    Serial.println("📶 Bluetooth Started: DMR828S-Walkie");
    Serial.println("💡 Connect via Bluetooth to send SMS commands wirelessly");
    
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
    
    Serial.println("🔧 Configuring walkie-talkie...");
    
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
    
    Serial.println("✅ Setup complete!");
    showCommands();
}

void setupBasicTest() {
    Serial.println("📡 Basic Test Mode");
    
    // Basic configuration
    dmr.setRadioID(wtState.myRadioID);
    dmr.setChannel(wtState.currentChannel);
    dmr.setVolume(wtState.volume);
    
    Serial.println("Basic test ready - checking radio ID every 2 seconds");
}

void setupWalkieFeatures() {
    Serial.println("🎙️ Full Walkie-Talkie Feature Mode");
    
    // Complete walkie-talkie setup
    if (dmr.setRadioID(wtState.myRadioID)) {
        Serial.print("✅ Radio ID: 0x"); Serial.println(wtState.myRadioID, HEX);
    }
    
    if (dmr.setChannel(wtState.currentChannel)) {
        Serial.print("✅ Channel: "); Serial.println(wtState.currentChannel);
    }
    
    if (dmr.setVolume(wtState.volume)) {
        Serial.print("✅ Volume: "); Serial.println(wtState.volume);
    }
    
    // Additional settings
    dmr.setColorCode(1);
    dmr.setTimeSlot(1);
    dmr.setTXPower(3);
    dmr.setMicGain(8);
    dmr.setSQLLevel(5);
    
    Serial.println("Full walkie-talkie features enabled");
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
    
    // Handle serial commands (USB and Bluetooth)
    handleSerialCommands();
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

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        processCommand(&Serial, command);
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
    else {
        stream->println("❓ Unknown command. Type 'help' for commands.");
    }
}

void showCommands() {
    showCommandsTo(&Serial);
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
    stream->println("Information:");
    stream->println("  status                  - Show status");
    stream->println("  info                    - Device info");
    stream->println("  smsinfo                 - SMS tracking status");
    stream->println("  fallback                - Manual fallback trigger");
    stream->println("  bt                      - Bluetooth status (BT only)");
    stream->println("  raw <hex>               - Send raw DMR command");
    stream->println("  help                    - Show commands");
    stream->println();
    stream->println("Examples:");
    stream->println("  sms 123 Hello World");
    stream->println("  call 456");
    stream->println("  channel 5");
    stream->println("  radioid 123456          - Set radio ID to 0x123456");
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
