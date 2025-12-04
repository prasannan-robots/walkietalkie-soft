#pragma once

#include <Arduino.h>
#include "DMR828S.h"
#include "BluetoothSerial.h"

// Forward declarations from DMR828S library
using CallType = DMRCallType;

// Hardware pin definitions
#define GSM_RX_PIN 25
#define GSM_TX_PIN 26
#define I2C_SDA 21
#define I2C_SCL 22

// Walkie-talkie state structure
struct WalkieTalkieState {
    uint32_t myRadioID = 0x000001;
    uint8_t currentChannel = 1;
    uint8_t volume = 5;
    String soldierID = "BSF12345"; // Default soldier ID
};

// Demo mode selector
enum DemoMode {
    MODE_BASIC_TEST,
    MODE_WALKIE_FEATURES,
    MODE_LOW_LEVEL
};

// Global instances and state
extern DMR828S dmr;
extern BluetoothSerial SerialBT;
extern WalkieTalkieState wtState;
extern DemoMode currentMode;

// Main system functions
void initializeSystem();
void setupBasicTest();
void setupWalkieFeatures();
void setupLowLevel();
void loopBasicTest();
void loopWalkieFeatures();
void loopLowLevel();
void handleBluetoothCommands();

// DMR event callbacks
void onSMSReceived(const DMRSMSMessage& message);
void onCallReceived(const DMRCallInfo& callInfo);
void onCallEnded();
void onEmergency(uint32_t sourceID);

// GPS JSON formatting functions
String formatGPSToJSON(double lat, double lon, String soldierId, String commMode);
void parseIncomingGPS(String message, String commMode);
void processGPSData(double lat, double lon, String soldierId, String commMode);
void onSMSStatus(uint32_t targetID, SMSSendStatus status);

// Command processing
void processCommand(Stream* stream, String command);
void showCommands();
void showCommandsTo(Stream* stream);
void showStatus();
void showStatusTo(Stream* stream);
void showDeviceInfo();
void showDeviceInfoTo(Stream* stream);