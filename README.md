# ESP32 PlatformIO Sample Project

This is a basic ESP32 project using PlatformIO that demonstrates:
- Serial communication
- WiFi connectivity
- Built-in LED control
- System monitoring

## Features

- **WiFi Connection**: Connects to your WiFi network and displays the IP address
- **LED Indicator**: 
  - Blinks while connecting to WiFi
  - Stays on when connected
  - Blinks every 5 seconds in the main loop
- **System Monitoring**: Displays free heap memory, WiFi signal strength, and uptime
- **Serial Output**: All information is printed to the serial monitor at 115200 baud

## Setup Instructions

1. **Install PlatformIO**: Make sure you have PlatformIO installed in VS Code
2. **Update WiFi Credentials**: Edit `src/main.cpp` and replace:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
   with your actual WiFi network name and password.

3. **Connect ESP32**: Connect your ESP32 board to your computer via USB

## Building and Uploading

### Using PlatformIO CLI:
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

### Using VS Code:
- Press `Ctrl+Shift+P` and search for "PlatformIO"
- Use "PlatformIO: Build" to compile
- Use "PlatformIO: Upload" to flash the ESP32
- Use "PlatformIO: Serial Monitor" to view output

## Hardware Requirements

- ESP32 development board (ESP32-DevKitC, NodeMCU-32S, or similar)
- USB cable for programming and power
- Computer with PlatformIO installed

## Expected Output

When running, you should see output like this in the serial monitor:
```
ESP32 Sample Project Starting...
Connecting to WiFi......
WiFi connected successfully!
IP address: 192.168.1.100
--- ESP32 Status ---
Free heap: 294524 bytes
WiFi RSSI: -45 dBm
Uptime: 5 seconds
-------------------
```

## Troubleshooting

1. **WiFi not connecting**: Check your SSID and password
2. **Upload fails**: Make sure the correct COM port is selected
3. **No serial output**: Verify baud rate is set to 115200
4. **LED not working**: Some boards use different pins for the built-in LED (try pin 2, 5, or 13)

## Next Steps

This sample provides a foundation for ESP32 development. You can extend it by adding:
- Sensors (temperature, humidity, etc.)
- Web server functionality
- MQTT communication
- Bluetooth connectivity
- Deep sleep modes
- Over-the-air (OTA) updates