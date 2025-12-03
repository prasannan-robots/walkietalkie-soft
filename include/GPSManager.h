#pragma once

#include <Arduino.h>

// GPS state structure
struct GPSState {
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
};

extern GPSState gpsState;

// GPS functions
void initializeGPS();
void readGPS();
void parseNMEA(String sentence);
void sendGPSLocation(Stream* stream, uint32_t targetID);
void handleContinuousGPS();