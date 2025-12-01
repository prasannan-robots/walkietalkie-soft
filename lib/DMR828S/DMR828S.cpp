#include "DMR828S.h"

DMR828S::DMR828S(HardwareSerial &port) : utils(port) {
}

void DMR828S::begin(uint32_t baud) {
    utils.begin(baud);
}

void DMR828S::enableDebug(bool enable) {
    utils.debug = enable;
}

void DMR828S::enableChecksum(bool enable) {
    utils.checksumEnabled = enable;
}

/********************************************************
 * 🔵 1. CORE RADIO OPERATION
 ********************************************************/

bool DMR828S::setChannel(uint8_t channel) {
    if (channel < 1 || channel > 16) return false;
    uint8_t data = channel;
    return sendCommand(DMR_CMD_SET_CHANNEL, 0x01, 0x01, &data, 1);
}

bool DMR828S::setVolume(uint8_t volume) {
    if (volume < 1 || volume > 9) return false;
    uint8_t data = volume;
    return sendCommand(DMR_CMD_SET_VOLUME, 0x01, 0x01, &data, 1);
}

DMRModuleStatus DMR828S::getModuleStatus() {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_CHECK_STATUS, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 1) {
            return (DMRModuleStatus)frame.data[0];
        }
    }
    return STATUS_STANDBY;
}

uint8_t DMR828S::getRSSI() {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_RSSI, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 1) {
            return frame.data[0];
        }
    }
    return 0;
}

/********************************************************
 * 🟢 2. VOICE CALLING
 ********************************************************/

bool DMR828S::startCall(DMRCallType type, uint32_t contactID) {
    uint8_t data[4];
    data[0] = (uint8_t)type;
    uint32ToBytes3(contactID, &data[1]);
    return sendCommand(DMR_CMD_CALL, 0x01, 0x01, data, 4);
}

bool DMR828S::stopCall() {
    uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};
    return sendCommand(DMR_CMD_CALL, 0x01, 0xFF, data, 4);
}

bool DMR828S::getCallInContact(DMRCallInfo &callInfo) {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_QUERY_CALL_CONTACT, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 4) {
            callInfo.type = (DMRCallType)frame.data[0];
            callInfo.contactID = bytes3ToUint32(&frame.data[1]);
            callInfo.active = true;
            return true;
        }
    }
    callInfo.active = false;
    return false;
}

/********************************************************
 * 🟣 3. SMS MESSAGING
 ********************************************************/

bool DMR828S::sendSMS(uint32_t targetID, const char* message, bool isGroup) {
    uint8_t msgLen = strlen(message);
    if (msgLen > 72) msgLen = 72; // Max length for UTF-16 (144 bytes / 2)
    
    // DMR828S expects UTF-16 format based on received frame analysis
    uint8_t data[150];
    uint8_t dataIndex = 0;
    
    // Add header bytes (based on sent frame pattern)
    data[dataIndex++] = isGroup ? 0x02 : 0x01; // SMS type: 0x01=Private, 0x02=Group
    uint32ToBytes3(targetID, &data[dataIndex]); // 3 bytes for target ID
    dataIndex += 3;
    
    // Convert message to UTF-16 format (char + 0x00)
    for (int i = 0; i < msgLen; i++) {
        data[dataIndex++] = message[i];
        data[dataIndex++] = 0x00; // UTF-16 null byte
    }
    
    if (utils.debug) {
        Serial.println("[DEBUG] Sending SMS in UTF-16 format:");
        Serial.print("  Target ID: 0x"); Serial.println(targetID, HEX);
        Serial.print("  Type: "); Serial.println(isGroup ? "Group" : "Private");
        Serial.print("  Message: '"); Serial.print(message); Serial.println("'");
        Serial.print("  Data bytes: ");
        for (int i = 0; i < dataIndex; i++) {
            if (data[i] < 0x10) Serial.print("0");
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    
    // Track SMS for status callback - no timeout, wait for actual response
    lastSMSTargetID = targetID;
    lastSMSSendTime = millis();
    
    if (utils.debug) {
        Serial.println("[DEBUG] SMS tracking started - listening for response...");
        Serial.println("[DEBUG] Will wait indefinitely until response received");
    }
    
    return sendCommand(DMR_CMD_SMS, 0x01, 0x01, data, dataIndex);
}

bool DMR828S::getLastSMS(DMRSMSMessage &sms) {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_QUERY_SMS, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 5) {
            sms.sourceID = bytes3ToUint32(&frame.data[0]);
            sms.type = frame.data[3];
            sms.length = frame.data[4];
            if (sms.length > 144) sms.length = 144;
            memcpy(sms.message, &frame.data[5], sms.length);
            sms.message[sms.length] = '\0';
            sms.valid = true;
            return true;
        }
    }
    sms.valid = false;
    return false;
}

/********************************************************
 * 🔴 4. EMERGENCY FEATURES
 ********************************************************/

bool DMR828S::sendEmergencyAlarm(uint32_t targetID) {
    uint8_t data[3];
    uint32ToBytes3(targetID, data);
    return sendCommand(DMR_CMD_EMERGENCY, 0x01, 0x01, data, 3);
}

/********************************************************
 * 🟠 5. AUDIO & HARDWARE ADJUSTMENT
 ********************************************************/

bool DMR828S::setMicGain(uint8_t gain) {
    if (gain > 15) return false; // Typical range 0-15
    return sendCommand(DMR_CMD_MIC_GAIN, 0x01, 0x01, &gain, 1);
}

bool DMR828S::setDutyMode(bool enable) {
    uint8_t data = enable ? 0x01 : 0x00;
    return sendCommand(DMR_CMD_DUTY_MODE, 0x01, 0x01, &data, 1);
}

bool DMR828S::setRepeaterMode(bool enable) {
    uint8_t data = enable ? 0x01 : 0x00;
    return sendCommand(DMR_CMD_REPEATER_MODE, 0x01, 0x01, &data, 1);
}

/********************************************************
 * 🟡 6. DMR CHANNEL CONFIGURATION
 ********************************************************/

bool DMR828S::setFrequency(uint32_t txFreq, uint32_t rxFreq) {
    uint8_t data[8];
    uint32ToBytes4(txFreq, &data[0]);
    uint32ToBytes4(rxFreq, &data[4]);
    return sendCommand(DMR_CMD_SET_FREQUENCY, 0x01, 0x01, data, 8);
}

bool DMR828S::setSQLLevel(uint8_t level) {
    if (level > 9) return false;
    return sendCommand(DMR_CMD_SQL_SETTING, 0x01, 0x01, &level, 1);
}

bool DMR828S::setCTCSSType(uint8_t type) {
    return sendCommand(DMR_CMD_CTCSS_TYPE, 0x01, 0x01, &type, 1);
}

bool DMR828S::setCTCSSCode(uint8_t code) {
    return sendCommand(DMR_CMD_CTCSS_CODE, 0x01, 0x01, &code, 1);
}

bool DMR828S::setTXPower(uint8_t power) {
    if (power > 3) return false; // Typical range 0-3
    return sendCommand(DMR_CMD_TX_POWER, 0x01, 0x01, &power, 1);
}

bool DMR828S::setContact(uint32_t contactID, DMRCallType type) {
    uint8_t data[4];
    uint32ToBytes3(contactID, data);
    data[3] = (uint8_t)type;
    return sendCommand(DMR_CMD_SET_CONTACT, 0x01, 0x01, data, 4);
}

bool DMR828S::setEncryption(bool enable, const uint8_t* encryptionKey) {
    if (enable) {
        // Encryption ON: Format = SWITCH (0x01) + KEY (8 bytes)
        uint8_t data[9];
        data[0] = 0x01; // SWITCH: Encryption on
        
        if (encryptionKey) {
            // Use provided key
            memcpy(&data[1], encryptionKey, 8);
        } else {
            // Use default key: 0x0102030405060708
            data[1] = 0x01; data[2] = 0x02; data[3] = 0x03; data[4] = 0x04;
            data[5] = 0x05; data[6] = 0x06; data[7] = 0x07; data[8] = 0x08;
        }
        
        if (utils.debug) {
            Serial.print("[DEBUG] Setting encryption ON with key: ");
            for (int i = 1; i < 9; i++) {
                if (data[i] < 0x10) Serial.print("0");
                Serial.print(data[i], HEX);
            }
            Serial.println();
        }
        
        return sendCommand(DMR_CMD_ENCRYPTION, 0x01, 0x01, data, 9);
    } else {
        // Encryption OFF: Format = SWITCH (0xFF)
        uint8_t data = 0xFF; // SWITCH: Encryption off
        
        if (utils.debug) {
            Serial.println("[DEBUG] Setting encryption OFF");
        }
        
        return sendCommand(DMR_CMD_ENCRYPTION, 0x01, 0x01, &data, 1);
    }
}

/********************************************************
 * 🟤 7. IDs & COLOR CODE SETUP
 ********************************************************/

bool DMR828S::setRadioID(uint32_t radioID) {
    uint8_t data[3];
    uint32ToBytes3(radioID, data);
    return sendCommand(DMR_CMD_SET_RADIO_ID, 0x01, 0x01, data, 3);
}

uint32_t DMR828S::getRadioID() {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_CHECK_RADIO_ID, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 3) {
            return bytes3ToUint32(frame.data);
        }
    }
    return 0;
}

uint32_t DMR828S::getContactID() {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_CHECK_CONTACT_ID, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 3) {
            return bytes3ToUint32(frame.data);
        }
    }
    return 0;
}

bool DMR828S::setColorCode(uint8_t colorCode) {
    if (colorCode > 15) return false;
    return sendCommand(DMR_CMD_COLOR_CODE, 0x01, 0x01, &colorCode, 1);
}

bool DMR828S::setTimeSlot(uint8_t timeSlot) {
    if (timeSlot < 1 || timeSlot > 2) return false;
    return sendCommand(DMR_CMD_TIME_SLOT, 0x01, 0x01, &timeSlot, 1);
}

/********************************************************
 * 🟣 8. RX GROUP LISTS
 ********************************************************/

bool DMR828S::addContactToRXGroup(uint8_t groupIndex, uint32_t contactID) {
    if (groupIndex < 1 || groupIndex > 32) return false;
    uint8_t data[4];
    data[0] = groupIndex;
    uint32ToBytes3(contactID, &data[1]);
    return sendCommand(DMR_CMD_ADD_RX_GROUP, 0x01, 0x01, data, 4);
}

bool DMR828S::clearRXGroup(uint8_t groupIndex) {
    if (groupIndex < 1 || groupIndex > 32) return false;
    return sendCommand(DMR_CMD_CLEAR_RX_GROUP, 0x01, 0x01, &groupIndex, 1);
}

/********************************************************
 * ⚪ 9. DIAGNOSTICS
 ********************************************************/

String DMR828S::getFirmwareVersion() {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_FIRMWARE_VERSION, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length > 0) {
            String version = "";
            for (int i = 0; i < frame.length; i++) {
                if (frame.data[i] >= 32 && frame.data[i] <= 126) {
                    version += (char)frame.data[i];
                }
            }
            return version;
        }
    }
    return "Unknown";
}

bool DMR828S::getEncryptionStatus() {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_ENCRYPTION_STATUS, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 1) {
            return frame.data[0] != 0;
        }
    }
    return false;
}

bool DMR828S::getCurrentChannelParams(DMRChannelParams &params) {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_CHANNEL_PARAMS, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 20) {
            // Parse channel parameters from response
            params.channel = frame.data[0];
            params.txFreq = bytes4ToUint32(&frame.data[1]);
            params.rxFreq = bytes4ToUint32(&frame.data[5]);
            params.power = frame.data[9];
            params.bandwidth = frame.data[10];
            params.colorCode = frame.data[11];
            params.timeSlot = frame.data[12];
            params.contactID = bytes3ToUint32(&frame.data[13]);
            params.encryptionOn = frame.data[16] != 0;
            return true;
        }
    }
    return false;
}

bool DMR828S::getInitializationStatus() {
    uint8_t data = 0x01;
    if (sendCommand(DMR_CMD_INIT_STATUS, 0x01, 0x01, &data, 1)) {
        DMRFrame frame;
        if (waitForResponse(frame) && frame.length >= 1) {
            return frame.data[0] == 0x00; // 0x00 = initialized
        }
    }
    return false;
}

/********************************************************
 * 🟤 10. SYSTEM
 ********************************************************/

bool DMR828S::resetToDefaults() {
    uint8_t data = 0x01;
    return sendCommand(DMR_CMD_RESET_DEFAULTS, 0x01, 0x01, &data, 1);
}

bool DMR828S::softwareReset() {
    uint8_t data = 0x01;
    return sendCommand(DMR_CMD_SOFTWARE_RESET, 0x01, 0x01, &data, 1);
}

/********************************************************
 * ADVANCED FEATURES
 ********************************************************/

bool DMR828S::setBandwidth(uint8_t bandwidth) {
    if (bandwidth > 1) return false; // 0=12.5K, 1=25K
    return sendCommand(DMR_CMD_BANDWIDTH, 0x01, 0x01, &bandwidth, 1);
}

bool DMR828S::setToneOnOff(bool enable) {
    uint8_t data = enable ? 0x01 : 0x00;
    return sendCommand(DMR_CMD_TONE_ONOFF, 0x01, 0x01, &data, 1);
}

/********************************************************
 * EVENT HANDLING
 ********************************************************/

void DMR828S::update() {
    DMRFrame frame;
    while (utils.readFrame(frame)) {
        if (utils.debug) {
            Serial.println("[DEBUG] Frame received in update()");
            Serial.print("  CMD: 0x"); Serial.println(frame.cmd, HEX);
            Serial.print("  R/W: 0x"); Serial.println(frame.rw, HEX);
            Serial.print("  SR: 0x"); Serial.println(frame.sr, HEX);
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
        }
        
        if (frame.valid) {
            processIncomingFrame(frame);
        } else if (utils.debug) {
            Serial.println("[DEBUG] Skipping invalid frame");
        }
    }
    
    // Optional: Safety timeout only for extreme cases (60 seconds)
    // This prevents infinite waiting if module completely fails
    if (smsStatusCallback && lastSMSSendTime > 0 && (millis() - lastSMSSendTime) > 60000) {
        if (utils.debug) {
            Serial.println("[DEBUG] SMS extreme timeout (60s) - module may have failed");
            Serial.println("[DEBUG] Triggering safety fallback");
        }
        smsStatusCallback(lastSMSTargetID, SMS_SEND_TIMEOUT);
        lastSMSSendTime = 0; // Reset to avoid multiple timeout calls
    }
}

void DMR828S::processIncomingFrame(const DMRFrame &frame) {
    if (utils.debug) {
        Serial.println("[DEBUG] Processing incoming frame");
        Serial.print("  CMD: 0x"); Serial.print(frame.cmd, HEX);
        Serial.print(" R/W: 0x"); Serial.print(frame.rw, HEX);
        Serial.print(" SR: 0x"); Serial.println(frame.sr, HEX);
    }
    
    switch (frame.cmd) {
        case DMR_CMD_SMS:
            if (utils.debug) {
                Serial.println("[DEBUG] SMS command detected!");
            }
            
            if (frame.rw == 0x02) { // Initiative sending (incoming SMS)
                if (utils.debug) {
                    Serial.println("[DEBUG] Processing incoming SMS (RW=0x02)");
                }
                processSMSEvent(frame);
            } 
            else if (frame.rw == 0x00) { // Response frame (SMS send result)
                if (utils.debug) {
                    Serial.println("[DEBUG] Processing SMS response (RW=0x00)");
                    Serial.print("  Response code: 0x"); Serial.println(frame.sr, HEX);
                    
                    if (frame.sr == 0x71) {
                        Serial.println("  ✅ SMS sent successfully!");
                    } else if (frame.sr == 0x7E) {
                        Serial.println("  ❌ SMS send failed!");
                    } else {
                        Serial.println("  ❓ Unknown SMS response code");
                    }
                }
                
                // Process SMS send status
                processSMSStatusEvent(frame);
            } 
            else if (utils.debug) {
                Serial.print("[DEBUG] Ignoring SMS frame - unhandled R/W: 0x");
                Serial.println(frame.rw, HEX);
            }
            break;
            
        case DMR_CMD_CALL:
            if (utils.debug) {
                Serial.println("[DEBUG] Call command detected!");
            }
            if (frame.rw == 0x02) { // Initiative sending (call events)
                processCallEvent(frame);
            } else if (frame.rw == 0x00) { // Call response
                if (utils.debug) {
                    Serial.println("[DEBUG] Call response received");
                }
            }
            break;
            
        case DMR_CMD_EMERGENCY:
            if (utils.debug) {
                Serial.println("[DEBUG] Emergency command detected!");
            }
            if (frame.rw == 0x02) { // Initiative sending (emergency)
                processEmergencyEvent(frame);
            }
            break;
            
        default:
            if (utils.debug) {
                Serial.print("[DEBUG] Unknown/unhandled command: 0x");
                Serial.println(frame.cmd, HEX);
            }
            break;
    }
}

void DMR828S::processSMSEvent(const DMRFrame &frame) {
    if (utils.debug) {
        Serial.println("\n[DEBUG] ===== SMS EVENT PROCESSING =====");
        Serial.print("  Frame length: "); Serial.println(frame.length);
        Serial.print("  SMS callback set: "); Serial.println(smsCallback ? "YES" : "NO");
        
        Serial.println("  Raw frame data analysis:");
        for (int i = 0; i < frame.length && i < 20; i++) {
            Serial.print("    [" + String(i) + "]: 0x");
            if (frame.data[i] < 0x10) Serial.print("0");
            Serial.print(frame.data[i], HEX);
            Serial.print(" (" + String(frame.data[i]) + ")");
            if (frame.data[i] >= 32 && frame.data[i] <= 126) {
                Serial.print(" '" + String((char)frame.data[i]) + "'");
            }
            Serial.println();
        }
    }
    
    if (!smsCallback) {
        if (utils.debug) {
            Serial.println("[DEBUG] SMS callback not set - ignoring SMS");
        }
        return;
    }
    
    // DMR828S SMS format analysis based on received data:
    // Data: 00 00 02 31 00 32 00 33 00
    // This appears to be: [Header bytes] + UTF-16 message data
    
    DMRSMSMessage sms;
    
    // Determine source ID and message format
    // Based on the pattern, it looks like:
    // - First few bytes might be header/control info
    // - Message is in UTF-16 format (alternating character + 0x00)
    
    if (frame.length >= 3) {
        // Extract source ID - using the 3rd byte as it seems to contain ID info
        sms.sourceID = frame.data[2]; // Simple ID from 3rd byte
        if (utils.debug) {
            Serial.print("[DEBUG] Extracted Source ID: 0x"); Serial.println(sms.sourceID, HEX);
        }
    } else {
        sms.sourceID = 0;
    }
    
    // Assume Group SMS for now (based on your output showing "Type: Group")
    sms.type = 2; // Group SMS
    
    if (utils.debug) {
        Serial.print("[DEBUG] SMS Type: "); Serial.println(sms.type == 1 ? "Private" : "Group");
    }
    
    // Extract UTF-16 message - look for pattern of char + 0x00
    sms.length = 0;
    memset(sms.message, 0, sizeof(sms.message));
    
    if (utils.debug) {
        Serial.println("[DEBUG] Attempting UTF-16 message extraction:");
    }
    
    // Start looking for UTF-16 data (character followed by 0x00)
    for (int i = 3; i < frame.length - 1 && sms.length < 144; i += 2) {
        if (frame.data[i] != 0x00 && frame.data[i+1] == 0x00) {
            // This looks like UTF-16: character + null byte
            sms.message[sms.length] = frame.data[i];
            if (utils.debug) {
                Serial.print("  Found UTF-16 char at [" + String(i) + "]: 0x");
                if (frame.data[i] < 0x10) Serial.print("0");
                Serial.print(frame.data[i], HEX);
                Serial.println(" '" + String((char)frame.data[i]) + "'");
            }
            sms.length++;
        } else if (frame.data[i] != 0x00) {
            // Non-UTF-16 character, add it anyway
            sms.message[sms.length] = frame.data[i];
            if (utils.debug) {
                Serial.print("  Found regular char at [" + String(i) + "]: 0x");
                if (frame.data[i] < 0x10) Serial.print("0");
                Serial.print(frame.data[i], HEX);
                Serial.println(" '" + String((char)frame.data[i]) + "'");
            }
            sms.length++;
        }
    }
    
    sms.message[sms.length] = '\0';
    sms.valid = true;
    
    if (utils.debug) {
        Serial.print("[DEBUG] Extracted message length: "); Serial.println(sms.length);
        Serial.print("[DEBUG] Final message: '"); Serial.print(sms.message); Serial.println("'");
        Serial.println("[DEBUG] Calling SMS callback...");
    }
    
    smsCallback(sms);
    
    if (utils.debug) {
        Serial.println("[DEBUG] SMS callback completed");
        Serial.println("[DEBUG] ===== SMS PROCESSING DONE =====\n");
    }
}

void DMR828S::processSMSStatusEvent(const DMRFrame &frame) {
    if (utils.debug) {
        Serial.println("[DEBUG] ===== SMS STATUS EVENT PROCESSING =====");
        Serial.print("  Status code: 0x"); Serial.println(frame.sr, HEX);
        Serial.print("  Target ID: 0x"); Serial.println(lastSMSTargetID, HEX);
        Serial.print("  SMS status callback set: "); Serial.println(smsStatusCallback ? "YES" : "NO");
        Serial.print("  Timeout tracking active: "); Serial.println(lastSMSSendTime > 0 ? "YES" : "NO");
    }
    
    if (!smsStatusCallback) {
        if (utils.debug) {
            Serial.println("[DEBUG] SMS status callback not set - ignoring status");
        }
        return;
    }
    
    // Check if timeout already occurred (lastSMSSendTime would be 0)
    if (lastSMSSendTime == 0) {
        if (utils.debug) {
            Serial.println("[DEBUG] SMS timeout already occurred - ignoring late response");
        }
        return;
    }
    
    SMSSendStatus status;
    switch (frame.sr) {
        case 0x71:
            status = SMS_SEND_SUCCESS;
            if (utils.debug) {
                Serial.println("[DEBUG] SMS send status: SUCCESS");
            }
            break;
            
        case 0x7E:
            status = SMS_SEND_FAILED;
            if (utils.debug) {
                Serial.println("[DEBUG] SMS send status: FAILED");
            }
            break;
            
        default:
            if (utils.debug) {
                Serial.println("[DEBUG] Unknown SMS status code - treating as failed");
            }
            status = SMS_SEND_FAILED;
            break;
    }
    
    if (utils.debug) {
        Serial.println("[DEBUG] Calling SMS status callback...");
    }
    
    smsStatusCallback(lastSMSTargetID, status);
    
    // Reset timeout tracking to prevent duplicate timeout callbacks
    lastSMSSendTime = 0;
    
    if (utils.debug) {
        Serial.println("[DEBUG] SMS status callback completed");
        Serial.println("[DEBUG] Timeout tracking reset to prevent duplicates");
        Serial.println("[DEBUG] ===== SMS STATUS PROCESSING DONE =====\n");
    }
}

void DMR828S::processCallEvent(const DMRFrame &frame) {
    if (frame.sr == 0x60) { // Being called starts
        if (callCallback && frame.length >= 4) {
            DMRCallInfo callInfo;
            callInfo.type = (DMRCallType)frame.data[0];
            callInfo.contactID = bytes3ToUint32(&frame.data[1]);
            callInfo.active = true;
            callCallback(callInfo);
        }
    } else if (frame.sr == 0x6F) { // Being called ends
        if (callEndCallback) {
            callEndCallback();
        }
    }
}

void DMR828S::processEmergencyEvent(const DMRFrame &frame) {
    if (emergencyCallback && frame.length >= 3) {
        uint32_t sourceID = bytes3ToUint32(frame.data);
        emergencyCallback(sourceID);
    }
}

/********************************************************
 * INTERNAL HELPER FUNCTIONS
 ********************************************************/

bool DMR828S::sendCommand(uint8_t cmd, uint8_t rw, uint8_t sr, const uint8_t *data, uint16_t len) {
    return utils.sendFrame(cmd, rw, sr, data, len);
}

bool DMR828S::waitForResponse(DMRFrame &frame, uint32_t timeout_ms) {
    uint32_t startTime = millis();
    
    while (millis() - startTime < timeout_ms) {
        if (utils.readFrame(frame)) {
            if (frame.valid && frame.rw == 0x00) { // Response frame
                return true;
            }
        }
        delay(1);
    }
    return false;
}

bool DMR828S::sendSimpleCommand(uint8_t cmd) {
    uint8_t data = 0x01;
    return sendCommand(cmd, 0x01, 0x01, &data, 1);
}

/********************************************************
 * DATA CONVERSION UTILITIES
 ********************************************************/

uint32_t DMR828S::bytes3ToUint32(const uint8_t *bytes) {
    uint32_t result = ((uint32_t)bytes[0] << 16) | ((uint32_t)bytes[1] << 8) | (uint32_t)bytes[2];
    
    if (utils.debug) {
        Serial.println("[DEBUG] bytes3ToUint32 conversion:");
        Serial.print("  Input bytes: 0x");
        if (bytes[0] < 0x10) Serial.print("0"); Serial.print(bytes[0], HEX);
        Serial.print(" 0x");
        if (bytes[1] < 0x10) Serial.print("0"); Serial.print(bytes[1], HEX);
        Serial.print(" 0x");
        if (bytes[2] < 0x10) Serial.print("0"); Serial.println(bytes[2], HEX);
        Serial.print("  Result: 0x"); Serial.println(result, HEX);
    }
    
    return result;
}

void DMR828S::uint32ToBytes3(uint32_t value, uint8_t *bytes) {
    bytes[0] = (value >> 16) & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = value & 0xFF;
}

uint32_t DMR828S::bytes4ToUint32(const uint8_t *bytes) {
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | 
           ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

void DMR828S::uint32ToBytes4(uint32_t value, uint8_t *bytes) {
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
}

bool DMR828S::sendRawCommand(const uint8_t *rawData, uint16_t length) {
    if (!rawData || length == 0) {
        return false;
    }
    
    // Send raw bytes directly to the serial port
    for (uint16_t i = 0; i < length; i++) {
        utils.getSerial()->write(rawData[i]);
    }
    
    if (utils.debug) {
        Serial.print("🔧 Raw command sent (");
        Serial.print(length);
        Serial.print(" bytes): ");
        for (uint16_t i = 0; i < length; i++) {
            if (rawData[i] < 0x10) Serial.print("0");
            Serial.print(rawData[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    
    return true;
}