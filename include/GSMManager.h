#pragma once

#include <Arduino.h>

// GSM state structure
struct GSMState {
    bool initialized = false;
    bool networkRegistered = false;
    String operatorName = "";
    int signalStrength = 0;
    String phoneNumber = "";
};

extern GSMState gsmState;

// GSM functions
void initializeGSM();
bool waitForGSMResponse(String expectedResponse, unsigned long timeout);
void checkGSMNetwork();
void getGSMSignalStrength();
void sendGSMFallbackSMS(String phoneNumber, String message);
void checkIncomingGSMSMS();
void readGSMSMS(int index);