# DMR828S Library - Refactored Architecture

## Overview

The DMR828S library has been refactored to provide a clean separation between low-level protocol handling and high-level convenience functions. This architecture allows users to choose the appropriate level of abstraction for their needs.

## Architecture

### 1. DMR828S_Utils (Low-Level Protocol Layer)
- **File**: `DMR828S_utils.h/cpp`
- **Purpose**: Handles raw protocol communication
- **Features**:
  - Frame sending and receiving
  - Checksum calculation
  - Debug output
  - Direct serial access
  - Protocol-level error handling

### 2. DMR828S (High-Level API Layer)
- **File**: `DMR828S.h/cpp`
- **Purpose**: Provides convenient, user-friendly functions
- **Features**:
  - Configuration management (volume, channel, frequency, etc.)
  - Event callbacks for received data and status changes
  - Automatic data conversion
  - Bulk operations
  - Access to low-level layer when needed

## Usage Examples

### High-Level API (Recommended for most users)

```cpp
#include "DMR828S.h"

DMR828S dmr(Serial2);

void onDataReceived(const uint8_t *data, uint16_t length) {
    // Handle received data
}

void setup() {
    dmr.begin(57600);
    dmr.setDataReceivedCallback(onDataReceived);
    
    // Easy configuration
    dmr.setVolume(75);
    dmr.setChannel(5);
    dmr.setFrequency(446000000); // 446 MHz
}

void loop() {
    dmr.update(); // Handle events
    
    // Send data easily
    String msg = "Hello World";
    dmr.transmitData((uint8_t*)msg.c_str(), msg.length());
}
```

### Low-Level API (For protocol-level control)

```cpp
#include "DMR828S_utils.h"

DMR828S_Utils dmrUtils(Serial2);

void setup() {
    dmrUtils.begin(57600);
    dmrUtils.debug = true;
}

void loop() {
    // Send custom frames
    uint8_t data[] = {0x01, 0x02, 0x03};
    dmrUtils.sendFrame(0x24, 0x01, 0x01, data, sizeof(data));
    
    // Read raw frames
    DMRFrame frame;
    if (dmrUtils.readFrame(frame)) {
        // Process frame manually
    }
}
```

### Mixed API (Best of both worlds)

```cpp
#include "DMR828S.h"

DMR828S dmr(Serial2);

void setup() {
    dmr.begin(57600);
    
    // Use high-level functions
    dmr.setVolume(50);
    
    // Access low-level when needed
    DMR828S_Utils& lowLevel = dmr.getLowLevel();
    lowLevel.debug = true;
    
    uint8_t customData[] = {0xFF, 0xAA};
    lowLevel.sendFrame(0x99, 0x01, 0x00, customData, 2);
}
```

## API Reference

### DMR828S (High-Level)

#### Configuration Functions
- `bool setFrequency(uint32_t frequency)` - Set radio frequency
- `bool getFrequency(uint32_t &frequency)` - Get current frequency
- `bool setVolume(uint8_t volume)` - Set volume (0-100)
- `bool getVolume(uint8_t &volume)` - Get current volume
- `bool setChannel(uint8_t channel)` - Set channel number
- `bool getChannel(uint8_t &channel)` - Get current channel
- `bool setPower(uint8_t power)` - Set transmission power
- `bool getPower(uint8_t &power)` - Get current power level
- `bool setSquelch(uint8_t squelch)` - Set squelch level
- `bool getSquelch(uint8_t &squelch)` - Get current squelch

#### Communication Functions
- `bool transmitData(const uint8_t *data, uint16_t length)` - Send data
- `bool receiveData(uint8_t *data, uint16_t &length, uint32_t timeout_ms)` - Receive data with timeout
- `void update()` - Process incoming frames and trigger callbacks

#### Status Functions
- `bool isConnected()` - Check if device is responding
- `bool getStatus(uint8_t &status)` - Get device status

#### Generic Register Access
- `bool readRegister(uint8_t reg, uint32_t &value)` - Read any register
- `bool writeRegister(uint8_t reg, uint32_t value)` - Write any register
- `bool readMultipleRegisters(uint8_t startReg, uint8_t count, uint32_t *values)` - Bulk read
- `bool writeMultipleRegisters(uint8_t startReg, uint8_t count, const uint32_t *values)` - Bulk write

#### Event Handling
- `setDataReceivedCallback(DataReceivedCallback callback)` - Set data received handler
- `setStatusChangedCallback(StatusChangedCallback callback)` - Set status change handler

#### Configuration
- `void enableDebug(bool enable)` - Enable/disable debug output
- `void enableChecksum(bool enable)` - Enable/disable checksum validation
- `DMR828S_Utils& getLowLevel()` - Get access to low-level API

### DMR828S_Utils (Low-Level)

#### Core Protocol Functions
- `bool sendFrame(uint8_t cmd, uint8_t rw, uint8_t sr, const uint8_t *data, uint16_t len)` - Send protocol frame
- `bool readFrame(DMRFrame &frame)` - Read protocol frame
- `uint16_t calcChecksum(const uint8_t *buf, uint16_t len)` - Calculate frame checksum

#### Utility Functions
- `void printHexByte(uint8_t b)` - Print byte in hex format
- `void printHexPacket(const char *prefix, const uint8_t *buf, uint16_t len)` - Print packet in hex
- `HardwareSerial* getSerial()` - Get direct serial access

#### Configuration
- `bool checksumEnabled` - Enable/disable checksum validation
- `bool debug` - Enable/disable debug output

## Frame Structure

The DMR828S protocol uses the following frame format:

```
[0x68] [CMD] [R/W] [SR] [CHECKSUM_H] [CHECKSUM_L] [LEN_H] [LEN_L] [DATA...] [0x10]
```

- **0x68**: Frame header
- **CMD**: Command byte
- **R/W**: Read/Write flag
- **SR**: Sub-register or parameter
- **CHECKSUM**: 16-bit checksum (optional)
- **LEN**: 16-bit payload length
- **DATA**: Payload data
- **0x10**: Frame footer

## Command Definitions

The library defines common command constants:

```cpp
#define DMR_CMD_READ_REG    0x01  // Read register
#define DMR_CMD_WRITE_REG   0x02  // Write register
#define DMR_CMD_GET_STATUS  0x03  // Get device status
#define DMR_CMD_SET_CONFIG  0x04  // Set configuration
#define DMR_CMD_TX_DATA     0x05  // Transmit data
#define DMR_CMD_RX_DATA     0x06  // Receive data
```

## Register Definitions

Common register addresses:

```cpp
#define DMR_REG_FREQUENCY   0x10  // Frequency register
#define DMR_REG_POWER       0x11  // Power level register
#define DMR_REG_VOLUME      0x12  // Volume register
#define DMR_REG_SQUELCH     0x13  // Squelch register
#define DMR_REG_CHANNEL     0x14  // Channel register
```

## Migration from Previous Version

If you were using the original library:

### Old Code:
```cpp
DMR828S dmr(Serial2);
dmr.debug = true;
dmr.sendFrame(0x24, 0x01, 0x01, data, len);
```

### New Code (Low-level equivalent):
```cpp
DMR828S_Utils dmr(Serial2);
dmr.debug = true;
dmr.sendFrame(0x24, 0x01, 0x01, data, len);
```

### New Code (High-level alternative):
```cpp
DMR828S dmr(Serial2);
dmr.enableDebug(true);
dmr.transmitData(data, len);  // Simpler interface
```

## Benefits of Refactored Architecture

1. **Separation of Concerns**: Protocol handling is separate from convenience functions
2. **Flexibility**: Choose appropriate abstraction level
3. **Maintainability**: Easier to add new features and fix bugs
4. **Testability**: Each layer can be tested independently
5. **Extensibility**: Easy to add new high-level functions without touching protocol code
6. **Backward Compatibility**: Low-level access still available for existing code

## Examples

The library includes several example sketches:

- **Simple_DMR**: Basic high-level usage
- **Low_Level_Protocol**: Direct protocol access
- **DMR_Demo**: Comprehensive demonstration of all features