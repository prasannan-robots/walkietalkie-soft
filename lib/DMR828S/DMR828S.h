#pragma once
#include "DMR828S_utils.h"

// DMR828S Command Definitions
#define DMR_CMD_SET_CHANNEL         0x01
#define DMR_CMD_SET_VOLUME          0x02
#define DMR_CMD_CHECK_STATUS        0x04
#define DMR_CMD_RSSI                0x05
#define DMR_CMD_CALL                0x06
#define DMR_CMD_SMS                 0x07
#define DMR_CMD_EMERGENCY           0x09
#define DMR_CMD_MIC_GAIN            0x0B
#define DMR_CMD_DUTY_MODE           0x0C
#define DMR_CMD_SET_FREQUENCY       0x0D
#define DMR_CMD_REPEATER_MODE       0x0E
#define DMR_CMD_QUERY_CALL_CONTACT  0x10
#define DMR_CMD_QUERY_SMS           0x11
#define DMR_CMD_SQL_SETTING         0x12
#define DMR_CMD_CTCSS_TYPE          0x13
#define DMR_CMD_CTCSS_CODE          0x14
#define DMR_CMD_TX_POWER            0x17
#define DMR_CMD_SET_CONTACT         0x18
#define DMR_CMD_ENCRYPTION          0x19
#define DMR_CMD_INIT_STATUS         0x1A
#define DMR_CMD_SET_RADIO_ID        0x1B
#define DMR_CMD_TONE_ONOFF          0x1C
#define DMR_CMD_CHANNEL_PARAMS      0x1D
#define DMR_CMD_CHECK_CONTACT_ID    0x22
#define DMR_CMD_CHECK_RADIO_ID      0x24
#define DMR_CMD_FIRMWARE_VERSION    0x25
#define DMR_CMD_ENCRYPTION_STATUS   0x28
#define DMR_CMD_ADD_RX_GROUP        0x29
#define DMR_CMD_CLEAR_RX_GROUP      0x30
#define DMR_CMD_COLOR_CODE          0x31
#define DMR_CMD_BANDWIDTH           0x32
#define DMR_CMD_TIME_SLOT           0x33
#define DMR_CMD_RESET_DEFAULTS      0xF0
#define DMR_CMD_SOFTWARE_RESET      0xF2

// Call Types
enum DMRCallType {
    CALL_ANALOG = 0x00,
    CALL_PRIVATE = 0x01,
    CALL_GROUP = 0x02,
    CALL_ALL = 0x04
};

// Module Status
enum DMRModuleStatus {
    STATUS_RECEIVING = 0x01,
    STATUS_TRANSMITTING = 0x02,
    STATUS_STANDBY = 0x03
};

// Response Status
enum DMRResponseStatus {
    RESPONSE_OK = 0x00,
    RESPONSE_BUSY_FAIL = 0x01,
    RESPONSE_CHANNEL_ERROR = 0x02,
    RESPONSE_CHECKSUM_ERROR = 0x09
};

// SMS Send Status
enum SMSSendStatus {
    SMS_SEND_SUCCESS = 0x71,
    SMS_SEND_FAILED = 0x7E,
    SMS_SEND_TIMEOUT = 0xFF
};

// SMS Structure
struct DMRSMSMessage {
    uint8_t type;           // Private/Group SMS
    uint32_t sourceID;      // 3-byte source ID
    uint32_t targetID;      // 3-byte target ID  
    uint8_t length;         // Message length
    char message[144];      // SMS content (max 144 chars)
    bool valid;
};

// Call Information Structure
struct DMRCallInfo {
    DMRCallType type;
    uint32_t contactID;     // 3-byte contact ID
    bool active;
};

// Channel Parameters Structure
struct DMRChannelParams {
    uint32_t txFreq;        // TX frequency in Hz
    uint32_t rxFreq;        // RX frequency in Hz
    uint8_t channel;        // Channel number
    uint8_t bandwidth;      // 0=12.5K, 1=25K
    uint8_t power;          // TX power level
    uint8_t colorCode;      // DMR color code
    uint8_t timeSlot;       // DMR time slot
    uint32_t contactID;     // Contact ID
    bool encryptionOn;      // Encryption status
};

// High-level DMR828S Walkie-Talkie API
class DMR828S {
public:
    DMR828S(HardwareSerial &port);
    
    // Basic Setup
    void begin(uint32_t baud = 57600);
    void enableDebug(bool enable);
    void enableChecksum(bool enable);
    
    // Direct access to low-level utilities
    DMR828S_Utils& getLowLevel() { return utils; }
    
    // 🔵 1. CORE RADIO OPERATION
    bool setChannel(uint8_t channel);                    // 0x01
    bool setVolume(uint8_t volume);                      // 0x02 (1-9)
    DMRModuleStatus getModuleStatus();                   // 0x04
    uint8_t getRSSI();                                   // 0x05
    
    // 🟢 2. VOICE CALLING
    bool startCall(DMRCallType type, uint32_t contactID = 0);  // 0x06
    bool stopCall();                                     // 0x06
    bool getCallInContact(DMRCallInfo &callInfo);        // 0x10
    
    // 🟣 3. SMS MESSAGING
    bool sendSMS(uint32_t targetID, const char* message, bool isGroup = false); // 0x07
    bool getLastSMS(DMRSMSMessage &sms);                 // 0x11
    
    // 🔴 4. EMERGENCY FEATURES
    bool sendEmergencyAlarm(uint32_t targetID = 0);      // 0x09
    
    // 🟠 5. AUDIO & HARDWARE ADJUSTMENT
    bool setMicGain(uint8_t gain);                       // 0x0B
    bool setDutyMode(bool enable);                       // 0x0C
    bool setRepeaterMode(bool enable);                   // 0x0E
    
    // 🟡 6. DMR CHANNEL CONFIGURATION
    bool setFrequency(uint32_t txFreq, uint32_t rxFreq); // 0x0D
    bool setSQLLevel(uint8_t level);                     // 0x12
    bool setCTCSSType(uint8_t type);                     // 0x13 (analog only)
    bool setCTCSSCode(uint8_t code);                     // 0x14 (analog only)
    bool setTXPower(uint8_t power);                      // 0x17
    bool setContact(uint32_t contactID, DMRCallType type); // 0x18
    bool setEncryption(bool enable, const uint8_t* encryptionKey = nullptr); // 0x19
    
    // 🟤 7. IDs & COLOR CODE SETUP
    bool setRadioID(uint32_t radioID);                   // 0x1B
    uint32_t getRadioID();                               // 0x24
    uint32_t getContactID();                             // 0x22
    bool setColorCode(uint8_t colorCode);                // 0x31
    bool setTimeSlot(uint8_t timeSlot);                  // 0x33
    
    // 🟣 8. RX GROUP LISTS
    bool addContactToRXGroup(uint8_t groupIndex, uint32_t contactID); // 0x29
    bool clearRXGroup(uint8_t groupIndex);               // 0x30
    
    // ⚪ 9. DIAGNOSTICS
    String getFirmwareVersion();                         // 0x25
    bool getEncryptionStatus();                          // 0x28
    bool getCurrentChannelParams(DMRChannelParams &params); // 0x1D
    bool getInitializationStatus();                      // 0x1A
    
    // 🟤 10. SYSTEM
    bool resetToDefaults();                              // 0xF0
    bool softwareReset();                                // 0xF2
    
    // ADVANCED FEATURES
    bool setBandwidth(uint8_t bandwidth);                // 0x32
    bool setToneOnOff(bool enable);                      // 0x1C
    
    // EVENT HANDLING
    typedef void (*SMSReceivedCallback)(const DMRSMSMessage &sms);
    typedef void (*SMSSendStatusCallback)(uint32_t targetID, SMSSendStatus status);
    typedef void (*CallReceivedCallback)(const DMRCallInfo &callInfo);
    typedef void (*CallEndedCallback)();
    typedef void (*EmergencyCallback)(uint32_t sourceID);
    
    void setSMSReceivedCallback(SMSReceivedCallback callback) { smsCallback = callback; }
    void setSMSSendStatusCallback(SMSSendStatusCallback callback) { smsStatusCallback = callback; }
    void setCallReceivedCallback(CallReceivedCallback callback) { callCallback = callback; }
    void setCallEndedCallback(CallEndedCallback callback) { callEndCallback = callback; }
    void setEmergencyCallback(EmergencyCallback callback) { emergencyCallback = callback; }
    
    // Call this in your main loop to handle incoming events
    void update();
    
    // RAW COMMAND SUPPORT - Advanced users only
    bool sendRawCommand(const uint8_t *rawData, uint16_t length);
    
private:
    DMR828S_Utils utils;
    
    // Callbacks
    SMSReceivedCallback smsCallback = nullptr;
    SMSSendStatusCallback smsStatusCallback = nullptr;
    CallReceivedCallback callCallback = nullptr;
    CallEndedCallback callEndCallback = nullptr;
    EmergencyCallback emergencyCallback = nullptr;
    
    // SMS tracking for status callbacks
    uint32_t lastSMSTargetID = 0;
    unsigned long lastSMSSendTime = 0;
    uint32_t lastSMSTimeout = 10000;  // Dynamic timeout in milliseconds
    
    // Internal helper functions
    bool sendCommand(uint8_t cmd, uint8_t rw, uint8_t sr, const uint8_t *data = nullptr, uint16_t len = 0);
    bool waitForResponse(DMRFrame &frame, uint32_t timeout_ms = 1000);
    bool sendSimpleCommand(uint8_t cmd);
    
    // Data conversion utilities
    uint32_t bytes3ToUint32(const uint8_t *bytes);
    void uint32ToBytes3(uint32_t value, uint8_t *bytes);
    uint32_t bytes4ToUint32(const uint8_t *bytes);
    void uint32ToBytes4(uint32_t value, uint8_t *bytes);
    
    // Event processing
    void processIncomingFrame(const DMRFrame &frame);
    void processSMSEvent(const DMRFrame &frame);
    void processSMSStatusEvent(const DMRFrame &frame);
    void processCallEvent(const DMRFrame &frame);
    void processEmergencyEvent(const DMRFrame &frame);
};