/*
 * OLED Display Test Sketch for SSD1306 128x64
 * 
 * Tests the OLED display with various patterns and text
 * Compatible with U8g2 library
 * 
 * Hardware:
 * - ESP32 DevKit
 * - SSD1306 OLED 128x64 (I2C)
 * - I2C Address: 0x3C
 * - Connections:
 *   SDA -> GPIO21
 *   SCL -> GPIO22
 *   VCC -> 3.3V
 *   GND -> GND
 */

#include <Wire.h>
#include <U8g2lib.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 21
#define I2C_SCL 22

// Initialize display with 180° rotation (U8G2_R2)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

int testPhase = 0;
unsigned long lastUpdate = 0;
int counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("   OLED Display Test - SSD1306 128x64");
  Serial.println("========================================\n");
  
  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("I2C initialized on pins:");
  Serial.println("  SDA: GPIO21");
  Serial.println("  SCL: GPIO22");
  
  // Scan I2C bus
  Serial.println("\nScanning I2C bus...");
  scanI2C();
  
  // Initialize display
  Serial.println("\nInitializing display...");
  if (!u8g2.begin()) {
    Serial.println("❌ Display initialization FAILED!");
    Serial.println("\nTroubleshooting:");
    Serial.println("1. Check I2C connections (SDA/SCL)");
    Serial.println("2. Verify I2C address is 0x3C");
    Serial.println("3. Check power supply (3.3V)");
    Serial.println("4. Try different I2C pins");
    while(1) delay(1000);
  }
  
  Serial.println("✅ Display initialized successfully!");
  Serial.println("\nDisplay Info:");
  Serial.println("  Type: SSD1306");
  Serial.println("  Size: 128x64");
  Serial.println("  I2C Address: 0x3C");
  Serial.println("  Rotation: 180°");
  
  // Show startup message
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x13_tr);
  u8g2.drawStr(0, 15, "OLED Test");
  u8g2.drawStr(0, 30, "SSD1306");
  u8g2.drawStr(0, 45, "128x64");
  u8g2.drawStr(0, 60, "Ready!");
  u8g2.sendBuffer();
  
  Serial.println("\n========================================");
  Serial.println("Starting test sequence...");
  Serial.println("Tests will cycle every 3 seconds");
  Serial.println("========================================\n");
  
  delay(2000);
}

void loop() {
  // Cycle through tests every 3 seconds
  if (millis() - lastUpdate > 3000) {
    lastUpdate = millis();
    testPhase = (testPhase + 1) % 8;
    
    Serial.print("Test Phase ");
    Serial.print(testPhase + 1);
    Serial.print("/8: ");
    
    switch(testPhase) {
      case 0:
        Serial.println("Basic Text");
        testBasicText();
        break;
      case 1:
        Serial.println("Different Fonts");
        testFonts();
        break;
      case 2:
        Serial.println("Shapes & Lines");
        testShapes();
        break;
      case 3:
        Serial.println("Fill Patterns");
        testFillPatterns();
        break;
      case 4:
        Serial.println("Scrolling Text");
        testScrolling();
        break;
      case 5:
        Serial.println("Menu Simulation");
        testMenu();
        break;
      case 6:
        Serial.println("Status Display");
        testStatus();
        break;
      case 7:
        Serial.println("Animation");
        testAnimation();
        break;
    }
  }
  
  // Update animation if in animation phase
  if (testPhase == 7) {
    delay(100);
    counter++;
    testAnimation();
  }
}

void testBasicText() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "Basic Text Test");
  u8g2.drawStr(0, 25, "Line 2");
  u8g2.drawStr(0, 40, "Line 3");
  u8g2.drawStr(0, 55, "Line 4");
  u8g2.sendBuffer();
}

void testFonts() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(0, 8, "4x6 Font - Tiny");
  
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 20, "5x7 Font - Small");
  
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 35, "6x10 Font - Med");
  
  u8g2.setFont(u8g2_font_7x13_tr);
  u8g2.drawStr(0, 53, "7x13 - Large");
  
  u8g2.sendBuffer();
}

void testShapes() {
  u8g2.clearBuffer();
  
  // Rectangle
  u8g2.drawFrame(5, 5, 30, 20);
  
  // Filled rectangle
  u8g2.drawBox(40, 5, 30, 20);
  
  // Circle
  u8g2.drawCircle(20, 45, 15);
  
  // Filled circle
  u8g2.drawDisc(55, 45, 15);
  
  // Lines
  u8g2.drawLine(80, 5, 120, 25);
  u8g2.drawLine(120, 5, 80, 25);
  
  // Triangle
  u8g2.drawLine(85, 35, 100, 60);
  u8g2.drawLine(100, 60, 115, 35);
  u8g2.drawLine(115, 35, 85, 35);
  
  u8g2.sendBuffer();
}

void testFillPatterns() {
  u8g2.clearBuffer();
  
  // Checkerboard pattern
  for (int y = 0; y < 32; y += 4) {
    for (int x = 0; x < 64; x += 4) {
      if ((x / 4 + y / 4) % 2 == 0) {
        u8g2.drawBox(x, y, 4, 4);
      }
    }
  }
  
  // Gradient effect
  for (int x = 0; x < 128; x += 2) {
    int height = (x * 32) / 128;
    u8g2.drawLine(x, 32, x, 32 + height);
  }
  
  u8g2.sendBuffer();
}

void testScrolling() {
  static int scrollOffset = 0;
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tr);
  
  String text = ">>> Scrolling Text Test - ESP32 Walkie Talkie <<<";
  int textWidth = text.length() * 6;
  
  scrollOffset = (scrollOffset + 2) % (textWidth + 128);
  
  u8g2.drawStr(128 - scrollOffset, 20, text.c_str());
  
  u8g2.drawStr(0, 40, "SSD1306 OLED");
  u8g2.drawStr(0, 55, "128x64 Display");
  
  u8g2.sendBuffer();
}

void testMenu() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tr);
  
  // Title bar
  u8g2.drawBox(0, 0, 128, 12);
  u8g2.setDrawColor(0);
  u8g2.drawStr(25, 10, "Main Menu");
  u8g2.setDrawColor(1);
  
  // Menu items
  u8g2.drawStr(5, 25, "> Send Message");
  u8g2.drawStr(5, 38, "  View Status");
  u8g2.drawStr(5, 51, "  Settings");
  u8g2.drawStr(5, 64, "  GPS Location");
  
  u8g2.sendBuffer();
}

void testStatus() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_5x7_tr);
  
  // Status display like walkie-talkie
  u8g2.drawStr(0, 8, "Radio ID: 0x000001");
  u8g2.drawStr(0, 18, "Channel: 1");
  u8g2.drawStr(0, 28, "Volume: 5/9");
  u8g2.drawStr(0, 38, "Signal: ****");
  u8g2.drawStr(0, 48, "DMR: Connected");
  u8g2.drawStr(0, 58, "Battery: 87%");
  
  // Progress bar
  u8g2.drawFrame(80, 50, 40, 8);
  u8g2.drawBox(82, 52, 35, 4);
  
  u8g2.sendBuffer();
}

void testAnimation() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(10, 15, "Animation Test");
  
  // Bouncing ball
  int x = 64 + (int)(50 * sin(counter * 0.1));
  int y = 40 + (int)(15 * cos(counter * 0.15));
  u8g2.drawDisc(x, y, 5);
  
  // Rotating line
  int centerX = 64;
  int centerY = 45;
  int radius = 15;
  float angle = counter * 0.2;
  int endX = centerX + (int)(radius * cos(angle));
  int endY = centerY + (int)(radius * sin(angle));
  u8g2.drawLine(centerX, centerY, endX, endY);
  u8g2.drawCircle(centerX, centerY, radius);
  
  u8g2.sendBuffer();
}

void scanI2C() {
  byte error, address;
  int nDevices = 0;
  
  Serial.println("Scanning for I2C devices...");
  
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("✅ I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      
      if (address == 0x3C) {
        Serial.print(" (SSD1306 OLED - Correct!)");
      } else if (address == 0x3D) {
        Serial.print(" (SSD1306 OLED - Alt Address)");
      }
      Serial.println();
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    Serial.println("❌ No I2C devices found!");
    Serial.println("\nCheck your connections:");
    Serial.println("  - SDA to GPIO21");
    Serial.println("  - SCL to GPIO22");
    Serial.println("  - VCC to 3.3V");
    Serial.println("  - GND to GND");
  } else {
    Serial.print("Found ");
    Serial.print(nDevices);
    Serial.println(" device(s)");
  }
  Serial.println();
}
