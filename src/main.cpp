#include <Arduino.h>
#include "WalkieTalkie.h"
#include "GPSManager.h"
#include "GSMManager.h"
#include "CommandProcessor.h"
#include "DisplayManager.h"
#include "KeyboardManager.h"

void setup() {
    // Initialize all system components
    initializeSystem();
    
    // Setup based on current mode
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
    
    // Show available commands
    showCommands();
}

void loop() {
    // Always handle DMR events
    dmr.update();
    
    // Handle GPS data
    readGPS();
    
    // Handle continuous GPS transmission
    handleContinuousGPS();
    
    // Check for incoming GSM SMS
    checkIncomingGSMSMS();
    
    // Handle Bluetooth commands
    handleBluetoothCommands();
    
    // Handle keyboard input
    scanKeyboard();
    
    // Update display
    updateDisplay();
    
    // // Run mode-specific loop
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
    
    // Small delay to prevent overwhelming the system
    delay(10);
}