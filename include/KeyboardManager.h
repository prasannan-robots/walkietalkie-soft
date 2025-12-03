#pragma once

#include <Arduino.h>
#include <Wire.h>

// GPIO Extender Configuration (PCF8574)
#define PCF8574_ADDRESS 0x20
#define I2C_SDA 21
#define I2C_SCL 22

// Keyboard Layout (4x4 matrix using PCF8574)
// PCF8574 pins: P0-P3 = Rows, P4-P7 = Columns
#define ROWS 4
#define COLS 4

// Key mappings
enum KeyAction {
    KEY_NONE = 0,
    KEY_1, KEY_2, KEY_3, KEY_A,
    KEY_4, KEY_5, KEY_6, KEY_B, 
    KEY_7, KEY_8, KEY_9, KEY_C,
    KEY_STAR, KEY_0, KEY_HASH, KEY_D,
    
    // Special function keys
    KEY_MENU,     // Long press *
    KEY_BACK,     // Long press #
    KEY_SELECT,   // Long press 5
    KEY_UP,       // 2
    KEY_DOWN,     // 8  
    KEY_LEFT,     // 4
    KEY_RIGHT,    // 6
    KEY_CALL,     // A
    KEY_SMS,      // B
    KEY_GPS,      // C
    KEY_SETTINGS  // D
};

// Keyboard state
struct KeyboardState {
    bool initialized = false;
    KeyAction lastKey = KEY_NONE;
    unsigned long lastKeyTime = 0;
    unsigned long keyHoldTime = 0;
    bool keyPressed[16] = {false};
    String inputBuffer = "";
    int cursorPosition = 0;
    bool capsLock = false;
    int inputMode = 0; // 0=numeric, 1=text, 2=hex
};

extern KeyboardState keyboardState;

// Keyboard functions
void initializeKeyboard();
void scanKeyboard();
KeyAction getKeyPress();
void handleKeyPress(KeyAction key);
void processInput();
char keyToChar(KeyAction key);
char getT9Char(KeyAction key, int pressCount);
void clearInput();
void backspace();
void addToInput(char c);
void testKeyboard();
void scanI2CDevices();