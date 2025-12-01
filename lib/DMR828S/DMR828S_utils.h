#pragma once
#include <Arduino.h>

// DMR Frame structure for protocol communication
struct DMRFrame {
    uint8_t cmd = 0;
    uint8_t rw = 0;
    uint8_t sr = 0;
    uint16_t checksum = 0;
    uint16_t length = 0;
    uint8_t data[256];
    bool valid = false;
};

// Low-level DMR828S protocol utilities class
class DMR828S_Utils {
public:
    DMR828S_Utils(HardwareSerial &port);
    
    // Basic setup
    void begin(uint32_t baud);
    
    // Configuration options
    bool checksumEnabled = true;
    bool debug = true;
    
    // Core protocol functions
    bool sendFrame(uint8_t cmd, uint8_t rw, uint8_t sr,
                   const uint8_t *data, uint16_t len);
    bool readFrame(DMRFrame &frame);
    
    // Utility functions
    uint16_t calcChecksum(const uint8_t *buf, uint16_t len);
    void printHexByte(uint8_t b);
    void printHexPacket(const char *prefix, const uint8_t *buf, uint16_t len);
    
    // Direct access to serial for advanced users
    HardwareSerial* getSerial() { return serial; }
    
private:
    HardwareSerial *serial;
};
