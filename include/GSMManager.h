#pragma once

#include <Arduino.h>
#include "SIM800L.h"

// GSM state structure
struct GSMState {
    bool initialized = false;
    bool networkRegistered = false;
    String operatorName = "";
    int signalStrength = 0;
    String phoneNumber = "";
};

extern GSMState gsmState;
extern SIM800L gsm;

// GSM functions
void initializeGSM();
void checkGSMNetwork();
void getGSMSignalStrength();
void sendGSMFallbackSMS(String phoneNumber, String message);
void checkIncomingGSMSMS();
void readGSMSMS(int index);