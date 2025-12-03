#include "CommandProcessor.h"
#include "WalkieTalkie.h"
#include "GPSManager.h"
#include "GSMManager.h"
#include "KeyboardManager.h"
#include "BluetoothSerial.h"

extern DMR828S dmr;
extern WalkieTalkieState wtState;
extern BluetoothSerial SerialBT;
extern DemoMode currentMode;

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
        
        if (keyStr.length() == 16) {
            uint8_t key[8];
            bool validHex = true;
            
            for (int i = 0; i < 8; i++) {
                String byteStr = keyStr.substring(i*2, i*2+2);
                char* endptr;
                unsigned long val = strtoul(byteStr.c_str(), &endptr, 16);
                if (*endptr != '\0' || val > 255) {
                    validHex = false;
                    break;
                }
                key[i] = (uint8_t)val;
            }
            
            if (validHex) {
                if (dmr.setEncryption(true, key)) {
                    stream->print("🔒 Encryption: ON with custom key: ");
                    for (int i = 0; i < 8; i++) {
                        if (key[i] < 0x10) stream->print("0");
                        stream->print(key[i], HEX);
                    }
                    stream->println();
                } else {
                    stream->println("❌ Failed to set encryption key");
                }
            } else {
                stream->println("❌ Invalid hex key format. Use 16 hex digits (8 bytes)");
            }
        } else {
            stream->println("❌ Key must be 16 hex digits (8 bytes). Example: 0123456789ABCDEF");
        }
    }
    else if (command == "encrypt status") {
        // Get current encryption status
        stream->println("🔐 Encryption Status: [Feature available - check DMR module]");
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
        stream->println("\\n📊 SMS Status Info:");
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
            // Get GPS data
            double lat, lon;
            String status;
            
            if (gpsState.hasValidFix) {
                lat = gpsState.latitude;
                lon = gpsState.longitude;
                status = "CURRENT";
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
            unsigned long minutes = intervalStr.toInt();
            
            if (targetID > 0 && minutes >= 1 && minutes <= 1440) { // Max 24 hours
                gpsState.continuousMode = true;
                gpsState.targetID = targetID;
                gpsState.intervalMinutes = minutes;
                gpsState.lastTransmission = 0; // Send immediately on next check
                
                stream->print("📍 Auto-GPS enabled: 0x");
                stream->print(targetID, HEX);
                stream->print(" every ");
                stream->print(minutes);
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
        stream->println("📍 Auto-GPS transmission stopped");
    }
    else if (command == "gpsinfo") {
        stream->println("\\n📍 GPS Status:");
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
        stream->println("\\n📡 Raw GPS Data (2 seconds):");
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
        String hexStr = command.substring(4);
        hexStr.trim();
        hexStr.toUpperCase();
        
        if (hexStr.length() % 2 == 0 && hexStr.length() >= 2) {
            int dataLen = hexStr.length() / 2;
            uint8_t* rawData = new uint8_t[dataLen];
            bool validHex = true;
            
            for (int i = 0; i < dataLen; i++) {
                String byteStr = hexStr.substring(i*2, i*2+2);
                char* endptr;
                unsigned long val = strtoul(byteStr.c_str(), &endptr, 16);
                if (*endptr != '\0' || val > 255) {
                    validHex = false;
                    break;
                }
                rawData[i] = (uint8_t)val;
            }
            
            if (validHex) {
                DMR828S_Utils& lowLevel = dmr.getLowLevel();
                
                // Send raw frame - first byte is command, rest is data
                if (dataLen >= 1) {
                    uint8_t cmd = rawData[0];
                    uint8_t* frameData = (dataLen > 1) ? &rawData[1] : nullptr;
                    int frameDataLen = (dataLen > 1) ? dataLen - 1 : 0;
                    
                    if (lowLevel.sendFrame(cmd, 0x01, 0x01, frameData, frameDataLen)) {
                        stream->print("📡 Raw command sent: ");
                        for (int i = 0; i < dataLen; i++) {
                            if (rawData[i] < 0x10) stream->print("0");
                            stream->print(rawData[i], HEX);
                            stream->print(" ");
                        }
                        stream->println();
                    } else {
                        stream->println("❌ Failed to send raw command");
                    }
                } else {
                    stream->println("❌ Need at least command byte");
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
        stream->println("\\n📱 GSM Status:");
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
    else if (command == "i2cscan") {
        scanI2CDevices();
    }
    else if (command == "keytest") {
        testKeyboard();
    }
    else if (command == "keyscan") {
        stream->println("Keyboard scanning mode enabled. Watch for key presses...");
        for (int i = 0; i < 50; i++) { // Scan for 1 second (50 * 20ms)
            scanKeyboard();
            delay(20);
        }
        stream->println("Keyboard scanning test complete.");
    }
    else {
        stream->println("❓ Unknown command. Type 'help' for commands.");
    }
}

void showCommands() {
    showCommandsTo(&SerialBT);
}

void showCommandsTo(Stream* stream) {
    stream->println("\\n📖 Available Commands:");
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
    stream->println("Debug & Testing:");
    stream->println("  i2cscan                 - Scan for I2C devices");
    stream->println("  keytest                 - Test keyboard matrix");
    stream->println("  keyscan                 - Live keyboard scanning test");
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
    stream->println("  gsmphone +1234567890    - Set emergency fallback number");
    stream->println();
}

void showStatus() {
    showStatusTo(&SerialBT);
}

void showStatusTo(Stream* stream) {
    stream->println("\\n📊 Current Status:");
    stream->println("==================");
    stream->print("Mode: ");
    
    switch(currentMode) {
        case MODE_BASIC_TEST: stream->println("Basic Test"); break;
        case MODE_WALKIE_FEATURES: stream->println("Full Walkie-Talkie"); break;
        case MODE_LOW_LEVEL: stream->println("Low-Level Protocol"); break;
        default: stream->println("Unknown"); break;
    }
    
    stream->print("Radio ID: 0x"); stream->println(wtState.myRadioID, HEX);
    stream->print("Channel: "); stream->println(wtState.currentChannel);
    stream->print("Volume: "); stream->println(wtState.volume);
    stream->print("RSSI: "); stream->println(dmr.getRSSI());
    
    DMRModuleStatus status = dmr.getModuleStatus();
    stream->print("Module Status: ");
    switch(status) {
        case STATUS_RECEIVING: stream->println("Receiving"); break;
        case STATUS_TRANSMITTING: stream->println("Transmitting"); break;
        case STATUS_STANDBY: stream->println("Standby"); break;
        default: stream->println("Unknown"); break;
    }
    
    // GPS Status
    stream->println();
    stream->println("📍 GPS Status:");
    stream->print("  Position: ");
    stream->print(gpsState.latitude, 6);
    stream->print(", ");
    stream->println(gpsState.longitude, 6);
    stream->print("  Valid Fix: ");
    stream->println(gpsState.hasValidFix ? "YES" : "NO");
    stream->print("  Auto-transmission: ");
    stream->println(gpsState.continuousMode ? "ENABLED" : "DISABLED");
    
    // GSM Status
    stream->println();
    stream->println("📱 GSM Status:");
    stream->print("  Module: ");
    stream->println(gsmState.initialized ? "READY" : "NOT READY");
    stream->print("  Network: ");
    stream->println(gsmState.networkRegistered ? "REGISTERED" : "NOT REGISTERED");
    stream->print("  Fallback Phone: ");
    stream->println(gsmState.phoneNumber.length() > 0 ? gsmState.phoneNumber : "Not configured");
}

void showDeviceInfo() {
    showDeviceInfoTo(&SerialBT);
}

void showDeviceInfoTo(Stream* stream) {
    stream->println("\\n🔧 Device Information:");
    stream->println("========================");
    stream->println("Hardware: ESP32 DMR Walkie-Talkie");
    stream->println("Firmware: v2.1 - Modular Architecture");
    stream->println("DMR Module: DMR828S");
    stream->println("GPS: Serial0 (9600 baud)");
    stream->println("GSM: Serial1 (9600 baud, GPIO25/26)");
    stream->println("DMR Radio: Serial2 (57600 baud, GPIO16/17)");
    stream->println("Bluetooth: SerialBT (LittleBoyz)");
    stream->println();
    stream->println("Features:");
    stream->println("  ✅ DMR Radio Communication");
    stream->println("  ✅ GPS Location Tracking");
    stream->println("  ✅ GSM Fallback SMS");
    stream->println("  ✅ Bluetooth Wireless Control");
    stream->println("  ✅ Encryption Support");
    stream->println("  ✅ Emergency Alerts");
    stream->println("  ✅ Automatic Fallback System");
    stream->println();
    stream->print("Uptime: ");
    stream->print(millis() / 1000);
    stream->println(" seconds");
}