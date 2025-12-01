#include "DMR828S_utils.h"

DMR828S_Utils::DMR828S_Utils(HardwareSerial &port) {
    serial = &port;
}

void DMR828S_Utils::begin(uint32_t baud) {
    serial->begin(baud);
}



/********************************************************
 * HEX PRINTING HELPERS
 ********************************************************/
void DMR828S_Utils::printHexByte(uint8_t b) {
    if (b < 0x10) Serial.print("0");
    Serial.print(b, HEX);
}

void DMR828S_Utils::printHexPacket(const char *prefix, const uint8_t *buf, uint16_t len) {
    if (!debug) return;
    Serial.print(prefix);
    Serial.print(" ");

    for (int i = 0; i < len; i++) {
        printHexByte(buf[i]);
        Serial.print(" ");
    }
    Serial.println();
}



/********************************************************
 * CHECKSUM
 ********************************************************/
uint16_t DMR828S_Utils::calcChecksum(const uint8_t *buf, uint16_t len)
{
    uint32_t sum = 0;

    while (len > 1) {
        sum += ((buf[0] << 8) | buf[1]);
        buf += 2;
        len -= 2;
    }

    if (len == 1) {
        sum += (buf[0] << 8);
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(sum ^ 0xFFFF);
}



/********************************************************
 * SEND FRAME
 ********************************************************/
bool DMR828S_Utils::sendFrame(uint8_t cmd, uint8_t rw, uint8_t sr,
                        const uint8_t *data, uint16_t len)
{
    uint8_t buf[300];
    uint16_t pos = 0;

    // Header
    buf[pos++] = 0x68;
    buf[pos++] = cmd;
    buf[pos++] = rw;
    buf[pos++] = sr;

    // Checksum (or zeroes if disabled)
    uint16_t chk = checksumEnabled ? calcChecksum(buf, 4) : 0x0000;
    buf[pos++] = (chk >> 8);
    buf[pos++] = (chk & 0xFF);

    // Payload length
    buf[pos++] = (len >> 8);
    buf[pos++] = (len & 0xFF);

    // Payload
    for (int i = 0; i < len; i++)
        buf[pos++] = data[i];

    // Tail
    buf[pos++] = 0x10;

    // Debug output
    printHexPacket("TX →", buf, pos);

    // Send
    serial->write(buf, pos);
    serial->flush();

    return true;
}



/********************************************************
 * READ FRAME
 ********************************************************/
bool DMR828S_Utils::readFrame(DMRFrame &f)
{
    static uint8_t buf[300];
    static uint16_t idx = 0;

    while (serial->available()) {
        uint8_t b = serial->read();
        buf[idx++] = b;

        // First byte must be 0x68
        if (idx == 1 && b != 0x68) {
            idx = 0;
            continue;
        }

        if (idx >= 8) {
            uint16_t payloadLen = (buf[6] << 8) | buf[7];
            uint16_t expectedLen = 1 + 3 + 2 + 2 + payloadLen + 1;

            if (idx == expectedLen) {
                if (buf[idx - 1] != 0x10) {
                    idx = 0;
                    return false;
                }

                // Fill struct
                f.cmd      = buf[1];
                f.rw       = buf[2];
                f.sr       = buf[3];
                f.checksum = (buf[4] << 8) | buf[5];
                f.length   = payloadLen;

                for (int i = 0; i < payloadLen; i++)
                    f.data[i] = buf[8 + i];

                // Verify checksum if enabled
                if (checksumEnabled) {
                    uint16_t chk = calcChecksum(buf, 4);
                    f.valid = (chk == f.checksum);
                } else {
                    f.valid = true;  // accept all packets
                }

                // Debug RX dump
                printHexPacket("RX ←", buf, idx);

                idx = 0;
                return true;
            }
        }

        if (idx >= 299)
            idx = 0;
    }

    return false;
}
