#include "GSMManager.h"
#include "BluetoothSerial.h"

extern BluetoothSerial SerialBT;

GSMState gsmState;
SIM800L gsm;

void initializeGSM() {
    SerialBT.println("📱 Initializing GSM module...");
    
    // Initialize GSM module on Serial1
    if (gsm.begin(Serial1)) {
        SerialBT.println("✅ GSM module detected");
        gsmState.initialized = true;
        
        // Check network registration
        checkGSMNetwork();
        
        // Get signal strength
        getGSMSignalStrength();
        
        // Get operator name
        gsmState.operatorName = gsm.serviceProvider();
        if (gsmState.operatorName.length() > 0 && gsmState.operatorName != "No network") {
            SerialBT.print("Operator: ");
            SerialBT.println(gsmState.operatorName);
        }
        
        SerialBT.println("✅ GSM module initialized");
    } else {
        SerialBT.println("❌ GSM module not responding");
        gsmState.initialized = false;
    }
}

void checkGSMNetwork() {
    // Check network registration using library function
    bool registered = gsm.checkNetwork();
    
    if (registered) {
        gsmState.networkRegistered = true;
        SerialBT.println("✅ GSM network registered");
    } else {
        gsmState.networkRegistered = false;
        SerialBT.println("❌ GSM network not registered");
    }
}

void getGSMSignalStrength() {
    // Get signal strength (0-31, -1 for error)
    int8_t signal = gsm.signalStrength();
    
    if (signal >= 0) {
        gsmState.signalStrength = signal;
    } else {
        gsmState.signalStrength = 0;
    }
}

void sendGSMFallbackSMS(String phoneNumber, String message) {
    if (!gsmState.initialized || !gsmState.networkRegistered) {
        SerialBT.println("❌ GSM not ready for SMS");
        return;
    }
    
    SerialBT.println("📱 Sending fallback SMS via GSM...");
    
    // Convert String to char array for library
    char phone[20];
    char msg[160];
    phoneNumber.toCharArray(phone, sizeof(phone));
    message.toCharArray(msg, sizeof(msg));
    
    if (gsm.sendSMS(phone, msg)) {
        SerialBT.println("✅ Fallback SMS sent successfully");
    } else {
        SerialBT.println("❌ Failed to send fallback SMS");
    }
}

void checkIncomingGSMSMS() {
    // Check for incoming SMS using library available() function
    if (gsm.available()) {
        // Read SMS from index 1 (most recent)
        String smsContent = gsm.readSMS(1);
        
        if (smsContent.length() > 0) {
            SerialBT.println("\n📱 GSM SMS Received:");
            
            // Parse SMS content from library response
            // Format: +CMGR: "status","sender","","timestamp"\r\nMessage
            if (smsContent.indexOf("CMGR:") != -1) {
                int messageStart = smsContent.indexOf('\n', smsContent.indexOf("CMGR:"));
                if (messageStart != -1) {
                    String message = smsContent.substring(messageStart + 1);
                    message.trim();
                    
                    SerialBT.println("Message: " + message);
                    
                    // Check if this is a GPS message and parse it
                    if (message.startsWith("GPS ")) {
                        extern void parseIncomingGPS(String message, String commMode);
                        parseIncomingGPS(message, "GSM");
                    }
                }
            }
        }
    }
}

void readGSMSMS(int index) {
    // Read SMS at specified index using library
    String smsContent = gsm.readSMS(index);
    
    if (smsContent.length() > 0) {
        SerialBT.println("\n📱 GSM SMS Received:");
        
        // Parse SMS content from library response
        if (smsContent.indexOf("CMGR:") != -1) {
            int messageStart = smsContent.indexOf('\n', smsContent.indexOf("CMGR:"));
            if (messageStart != -1) {
                String message = smsContent.substring(messageStart + 1);
                message.trim();
                
                SerialBT.println("Message: " + message);
                
                // Check if this is a GPS message and parse it
                if (message.startsWith("GPS ")) {
                    extern void parseIncomingGPS(String message, String commMode);
                    parseIncomingGPS(message, "GSM");
                }
            }
        }
    }
}