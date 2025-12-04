#include "GPSManager.h"

GPSState gpsState;

void initializeGPS() {
    // GPS module on Serial0 (9600 baud is standard for most GPS modules)
    Serial.begin(9600); // GPS module baud rate
    delay(1000);
}

void readGPS() {
    // Read NMEA sentences from GPS module on Serial0
    static String sentence = "";
    
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n') {
            // Process complete sentence
            if (sentence.length() > 0) {
                parseNMEA(sentence);
                sentence = "";
            }
        } else if (c != '\r') {
            sentence += c;
        }
    }
    
    // Update GPS read timestamp
    gpsState.lastGPSRead = millis();
}

void parseNMEA(String sentence) {
    // Parse GPGGA or GNGGA sentences for position data
    if (sentence.startsWith("$GPGGA") || sentence.startsWith("$GNGGA")) {
        // Split sentence by commas
        int commaCount = 0;
        String fields[15]; // GPGGA has up to 14 fields
        int startIndex = 0;
        
        for (int i = 0; i < sentence.length() && commaCount < 14; i++) {
            if (sentence.charAt(i) == ',') {
                fields[commaCount] = sentence.substring(startIndex, i);
                startIndex = i + 1;
                commaCount++;
            }
        }
        // Get last field
        if (commaCount < 14) {
            fields[commaCount] = sentence.substring(startIndex);
        }
        
        // Check if we have a valid fix (field 6)
        if (fields[6].toInt() > 0) {
            // Parse time (field 1) - format HHMMSS.sss
            if (fields[1].length() >= 6) {
                String timeStr = fields[1];
                gpsState.gpsHour = timeStr.substring(0, 2).toInt();
                gpsState.gpsMinute = timeStr.substring(2, 4).toInt();
                gpsState.gpsSecond = timeStr.substring(4, 6).toInt();
                gpsState.hasValidTime = true;
            }
            
            // Parse latitude (field 2) and direction (field 3)
            if (fields[2].length() > 0 && fields[4].length() > 0) {
                double lat = fields[2].toDouble();
                double lon = fields[4].toDouble();
                
                // Convert from DDMM.MMMM to decimal degrees
                int latDeg = (int)(lat / 100);
                double latMin = lat - (latDeg * 100);
                gpsState.latitude = latDeg + (latMin / 60.0);
                if (fields[3] == "S") gpsState.latitude = -gpsState.latitude;
                
                int lonDeg = (int)(lon / 100);
                double lonMin = lon - (lonDeg * 100);
                gpsState.longitude = lonDeg + (lonMin / 60.0);
                if (fields[5] == "W") gpsState.longitude = -gpsState.longitude;
                
                // Store as last known location
                gpsState.lastLatitude = gpsState.latitude;
                gpsState.lastLongitude = gpsState.longitude;
                
                gpsState.hasValidFix = true;
                gpsState.hasLastLocation = true;
            }
        } else {
            gpsState.hasValidFix = false;
        }
    }
    
    // Parse GPRMC or GNRMC sentences for date information
    if (sentence.startsWith("$GPRMC") || sentence.startsWith("$GNRMC")) {
        // Split sentence by commas
        int commaCount = 0;
        String fields[12]; // GPRMC has up to 11 fields
        int startIndex = 0;
        
        for (int i = 0; i < sentence.length() && commaCount < 11; i++) {
            if (sentence.charAt(i) == ',') {
                fields[commaCount] = sentence.substring(startIndex, i);
                startIndex = i + 1;
                commaCount++;
            }
        }
        // Get last field
        if (commaCount < 11) {
            fields[commaCount] = sentence.substring(startIndex);
        }
        
        // Check if data is valid (field 2)
        if (fields[2] == "A") {
            // Parse date (field 9) - format DDMMYY
            if (fields[9].length() >= 6) {
                String dateStr = fields[9];
                gpsState.gpsDay = dateStr.substring(0, 2).toInt();
                gpsState.gpsMonth = dateStr.substring(2, 4).toInt();
                int year = dateStr.substring(4, 6).toInt();
                gpsState.gpsYear = (year < 80) ? 2000 + year : 1900 + year; // Y2K handling
            }
        }
    }
}

void sendGPSLocation(Stream* stream, uint32_t targetID) {
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
    
    // Send via SMS - need to access DMR instance from main
    // This will be handled by the calling code
    stream->print("📍 GPS ready to send to 0x");
    stream->print(targetID, HEX);
    stream->print(" (");
    stream->print(status);
    stream->print("): ");
    stream->print(lat, 6);
    stream->print(", ");
    stream->println(lon, 6);
}

void handleContinuousGPS() {
    if (!gpsState.continuousMode) return;
    
    unsigned long currentTime = millis();
    unsigned long intervalMs = gpsState.intervalMinutes * 60 * 1000; // Convert minutes to milliseconds
    
    // Check if it's time to send GPS
    if (gpsState.lastTransmission == 0 || 
        (currentTime - gpsState.lastTransmission) >= intervalMs) {
        
        gpsState.lastTransmission = currentTime;
        
        // Send GPS location notification - actual sending handled by main
        // This function just manages timing
    }
}

String getGPSTimestamp() {
    if (gpsState.hasValidTime) {
        // Format: YYYY-MM-DDTHH:MM:SSZ
        String timestamp = String(gpsState.gpsYear) + "-";
        
        if (gpsState.gpsMonth < 10) timestamp += "0";
        timestamp += String(gpsState.gpsMonth) + "-";
        
        if (gpsState.gpsDay < 10) timestamp += "0";
        timestamp += String(gpsState.gpsDay) + "T";
        
        if (gpsState.gpsHour < 10) timestamp += "0";
        timestamp += String(gpsState.gpsHour) + ":";
        
        if (gpsState.gpsMinute < 10) timestamp += "0";
        timestamp += String(gpsState.gpsMinute) + ":";
        
        if (gpsState.gpsSecond < 10) timestamp += "0";
        timestamp += String(gpsState.gpsSecond) + "Z";
        
        return timestamp;
    } else {
        // Fallback to system time if GPS time not available
        String timestamp = "2025-11-20T";
        
        unsigned long currentTime = millis() / 1000;
        int hours = (currentTime / 3600) % 24;
        int minutes = (currentTime / 60) % 60;
        int seconds = currentTime % 60;
        
        if (hours < 10) timestamp += "0";
        timestamp += String(hours) + ":";
        if (minutes < 10) timestamp += "0";
        timestamp += String(minutes) + ":";
        if (seconds < 10) timestamp += "0";
        timestamp += String(seconds) + "Z";
        
        return timestamp;
    }
}