#include "KeyboardManager.h"
#include "DisplayManager.h"
#include "CommandProcessor.h"
#include "BluetoothSerial.h"

extern BluetoothSerial SerialBT;

KeyboardState keyboardState;

// Direct I2C communication for keyboard
#define PCF8574_ADDR 0x20

// Key matrix mapping (4x4)
const KeyAction keyMatrix[ROWS][COLS] = {
    {KEY_1, KEY_2, KEY_3, KEY_A},
    {KEY_4, KEY_5, KEY_6, KEY_B},
    {KEY_7, KEY_8, KEY_9, KEY_C},
    {KEY_STAR, KEY_0, KEY_HASH, KEY_D}
};

void initializeKeyboard() {
    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);
    
    // Initialize PCF8574 pins to all HIGH (rows and columns)
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(0xFF);
    if (Wire.endTransmission() == 0) {
        keyboardState.initialized = true;
        Serial.println("Keyboard initialized successfully!");
    } else {
        keyboardState.initialized = false;
        Serial.println("Failed to initialize keyboard!");
        return;
    }
    
    keyboardState.inputMode = 0; // Start in numeric mode
}

void scanKeyboard() {
    if (!keyboardState.initialized) return;
    
    static unsigned long lastScan = 0;
    if (millis() - lastScan < 50) return; // Debouncing
    lastScan = millis();
    
    // Scan the 4x4 matrix using working method
    for (byte col = 0; col < COLS; col++) {
        // Drive one column LOW (0), rest HIGH (1)
        byte col_mask = 0x0F;             // Lower 4 bits columns all HIGH initially
        col_mask &= ~(1 << col);          // Set current column LOW 
        
        // Rows (bits 4-7) are inputs with pull-ups, so set HIGH (1)
        byte output = (0xF0) | col_mask;  // Rows high (bits 4-7), columns as above

        Wire.beginTransmission(PCF8574_ADDR);
        Wire.write(output);
        Wire.endTransmission();
        delayMicroseconds(50);

        Wire.requestFrom(PCF8574_ADDR, 1);
        if (Wire.available()) {
            byte data = Wire.read();

            // Check rows (bits 4-7). Active low on row means pressed key.
            for (byte row = 0; row < ROWS; row++) {
                if (!(data & (1 << (row + 4)))) {
                    // Key at row, col is pressed
                    int keyIndex = row * COLS + col;
                    
                    if (!keyboardState.keyPressed[keyIndex]) {
                        // Key just pressed
                        keyboardState.keyPressed[keyIndex] = true;
                        keyboardState.lastKey = keyMatrix[row][col];
                        keyboardState.lastKeyTime = millis();
                        keyboardState.keyHoldTime = 0;
                        
                        // Debug output
                        Serial.print("Key pressed: ");
                        Serial.print((char)keyMatrix[row][col]);
                        Serial.print(" (row: ");
                        Serial.print(row);
                        Serial.print(", col: ");
                        Serial.print(col);
                        Serial.println(")");
                        
                        SerialBT.print("Key pressed: ");
                        SerialBT.print(row);
                        SerialBT.print(",");
                        SerialBT.println(col);
                    } else {
                        // Key held down - check for long press
                        keyboardState.keyHoldTime = millis() - keyboardState.lastKeyTime;
                        
                        // Long press detection (1 second)
                        if (keyboardState.keyHoldTime > 1000) {
                            KeyAction longPressKey = KEY_NONE;
                            
                            switch (keyMatrix[row][col]) {
                                case KEY_STAR: longPressKey = KEY_MENU; break;
                                case KEY_HASH: longPressKey = KEY_BACK; break;
                                case KEY_5: longPressKey = KEY_SELECT; break;
                                case KEY_2: longPressKey = KEY_UP; break;
                                case KEY_8: longPressKey = KEY_DOWN; break;
                                case KEY_4: longPressKey = KEY_LEFT; break;
                                case KEY_6: longPressKey = KEY_RIGHT; break;
                            }
                            
                            if (longPressKey != KEY_NONE) {
                                handleKeyPress(longPressKey);
                                keyboardState.keyPressed[keyIndex] = false; // Prevent repeat
                                keyboardState.lastKey = KEY_NONE;
                            }
                        }
                    }
                } else {
                    // Key not pressed
                    int keyIndex = row * COLS + col;
                    if (keyboardState.keyPressed[keyIndex]) {
                        // Key just released
                        keyboardState.keyPressed[keyIndex] = false;
                        
                        // Process the key press (only if it wasn't a long press)
                        if (keyboardState.lastKey == keyMatrix[row][col]) {
                            handleKeyPress(keyboardState.lastKey);
                            keyboardState.lastKey = KEY_NONE;
                        }
                    }
                }
            }
        }
    }
    
    // Reset all pins HIGH
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
}

KeyAction getKeyPress() {
    KeyAction key = keyboardState.lastKey;
    keyboardState.lastKey = KEY_NONE;
    return key;
}

void handleKeyPress(KeyAction key) {
    // Check if we're in input mode
    if (displayState.inputMode) {
        char c = keyToChar(key);
        if (c != 0) {
            handleInput(c);
        } else if (key == KEY_HASH || key == KEY_BACK) {
            handleInput('#');
        } else if (key == KEY_STAR || key == KEY_SELECT) {
            handleInput('*');
        } else if (key == KEY_C) {
            handleInput('C');
        }
        return;
    }
    
    // Check if we're in menu mode
    if (displayState.inMenu) {
        switch (key) {
            case KEY_2:
            case KEY_UP:
                navigateUp();
                break;
                
            case KEY_8: 
            case KEY_DOWN:
                navigateDown();
                break;
                
            case KEY_5:
            case KEY_SELECT:
                selectMenuItem();
                break;
                
            case KEY_HASH:
            case KEY_BACK:
                goBack();
                break;
                
            case KEY_STAR:
            case KEY_MENU:
                // Exit menu system
                displayState.inMenu = false;
                showMainScreen();
                break;
                
            default:
                // Numeric keys for quick selection
                if (key >= KEY_1 && key <= KEY_9) {
                    int itemIndex = key - KEY_1;
                    if (itemIndex < displayState.currentMenu.itemCount) {
                        displayState.currentMenu.selectedItem = itemIndex;
                        selectMenuItem();
                    }
                }
                break;
        }
        return;
    }
    
    // Non-menu key handling
    switch (key) {
        case KEY_STAR:
        case KEY_MENU:
            // Enter menu system
            displayState.inMenu = true;
            createMainMenu();
            showMenu();
            break;
            
        case KEY_BACK:
            displayState.currentScreen = "main";
            clearInput();
            break;
            
        case KEY_A: // Call function
            if (keyboardState.inputBuffer.length() > 0) {
                String command = "call " + keyboardState.inputBuffer;
                processCommand(&SerialBT, command);
                displaySuccess("Calling " + keyboardState.inputBuffer);
                clearInput();
            } else {
                // Enter menu system
                displayState.inMenu = true;
                createMainMenu();
                showMenu();
            }
            break;
            
        case KEY_B: // SMS function
            displayState.currentScreen = "gsm";
            break;
            
        case KEY_C: // GPS function
            if (keyboardState.inputBuffer.length() > 0) {
                String command = "gps " + keyboardState.inputBuffer;
                processCommand(&SerialBT, command);
                displaySuccess("GPS sent to " + keyboardState.inputBuffer);
                clearInput();
            } else {
                displayState.currentScreen = "gps";
            }
            break;
            
        case KEY_D: // Settings
            displayState.currentScreen = "status";
            break;
            
        case KEY_UP:
        case KEY_2:
            // Navigate up in screens
            break;
            
        case KEY_DOWN:
        case KEY_8:
            // Navigate down in screens 
            break;
            
        case KEY_LEFT:
        case KEY_4:
            // Navigate left or backspace
            backspace();
            break;
            
        case KEY_RIGHT:
        case KEY_6:
            // Navigate right
            break;
            
        case KEY_SELECT:
        case KEY_5:
            // Select/Enter
            processInput();
            break;
            
        default:
            // Regular key press - add to input buffer
            char c = keyToChar(key);
            if (c != 0) {
                addToInput(c);
            }
            break;
    }
}

void processInput() {
    if (keyboardState.inputBuffer.length() == 0) return;
    
    // Process based on current screen/context
    if (displayState.currentScreen == "main") {
        // Channel change
        int channel = keyboardState.inputBuffer.toInt();
        if (channel >= 1 && channel <= 16) {
            String command = "channel " + keyboardState.inputBuffer;
            processCommand(&SerialBT, command);
            displaySuccess("Channel " + keyboardState.inputBuffer);
        }
    }
    
    clearInput();
}

char keyToChar(KeyAction key) {
    switch (key) {
        case KEY_0: return '0';
        case KEY_1: return '1';
        case KEY_2: return '2';
        case KEY_3: return '3';
        case KEY_4: return '4';
        case KEY_5: return '5';
        case KEY_6: return '6';
        case KEY_7: return '7';
        case KEY_8: return '8';
        case KEY_9: return '9';
        case KEY_STAR: return '*';
        case KEY_HASH: return '#';
        default: return 0;
    }
}

char getT9Char(KeyAction key, int pressCount) {
    // T9 text input mapping
    const char* t9Map[] = {
        "",      // KEY_0
        "",      // KEY_1  
        "abc",   // KEY_2
        "def",   // KEY_3
        "ghi",   // KEY_4
        "jkl",   // KEY_5
        "mno",   // KEY_6
        "pqrs",  // KEY_7
        "tuv",   // KEY_8
        "wxyz"   // KEY_9
    };
    
    int keyNum = key - KEY_0;
    if (keyNum >= 0 && keyNum <= 9) {
        String chars = t9Map[keyNum];
        if (pressCount > 0 && pressCount <= chars.length()) {
            char c = chars[pressCount - 1];
            return keyboardState.capsLock ? toupper(c) : c;
        }
    }
    
    return 0;
}

void clearInput() {
    keyboardState.inputBuffer = "";
    keyboardState.cursorPosition = 0;
}

void backspace() {
    if (keyboardState.inputBuffer.length() > 0) {
        keyboardState.inputBuffer.remove(keyboardState.inputBuffer.length() - 1);
        keyboardState.cursorPosition--;
        if (keyboardState.cursorPosition < 0) keyboardState.cursorPosition = 0;
    }
}

void addToInput(char c) {
    if (keyboardState.inputBuffer.length() < 20) { // Limit input length
        keyboardState.inputBuffer += c;
        keyboardState.cursorPosition++;
    }
}

void scanI2CDevices() {
    SerialBT.println("Scanning I2C devices...");
    
    int deviceCount = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            SerialBT.print("I2C device found at address 0x");
            if (address < 16) SerialBT.print("0");
            SerialBT.print(address, HEX);
            
            // Identify common devices
            switch (address) {
                case 0x20: SerialBT.print(" (PCF8574 - GPIO Extender)"); break;
                case 0x3C: SerialBT.print(" (SSD1306 - OLED Display)"); break;
                case 0x3D: SerialBT.print(" (SSD1306 - OLED Display Alt)"); break;
                case 0x68: SerialBT.print(" (DS3231 - RTC)"); break;
                default: SerialBT.print(" (Unknown)"); break;
            }
            SerialBT.println();
            deviceCount++;
        }
    }
    
    if (deviceCount == 0) {
        SerialBT.println("No I2C devices found!");
    } else {
        SerialBT.print("Found ");
        SerialBT.print(deviceCount);
        SerialBT.println(" I2C device(s).");
    }
}

void testKeyboard() {
    SerialBT.println("Testing PCF8574 keyboard...");
    
    // Check if PCF8574 is responding
    Wire.beginTransmission(PCF8574_ADDR);
    byte error = Wire.endTransmission();
    
    if (error != 0) {
        SerialBT.print("PCF8574 not found at address 0x");
        SerialBT.println(PCF8574_ADDR, HEX);
        return;
    }
    
    SerialBT.println("PCF8574 found. Testing matrix...");
    
    // Test each row
    for (int row = 0; row < ROWS; row++) {
        SerialBT.print("Testing row ");
        SerialBT.print(row);
        SerialBT.print(": ");
        
        // Set all pins HIGH, then set current row LOW
        uint8_t portValue = 0xFF;
        portValue &= ~(1 << row);
        // Write port value
        Wire.beginTransmission(PCF8574_ADDR);
        Wire.write(portValue);
        Wire.endTransmission();
        delay(10);
        
        // Read port value
        Wire.requestFrom(PCF8574_ADDR, 1);
        uint8_t readValue = 0xFF;
        if (Wire.available()) {
            readValue = Wire.read();
        }
        
        SerialBT.print("Port value: 0x");
        SerialBT.print(readValue, HEX);
        SerialBT.print(" (Binary: ");
        for (int i = 7; i >= 0; i--) {
            SerialBT.print((readValue >> i) & 1);
        }
        SerialBT.print(") Columns: ");
        
        // Check column states
        for (int col = 0; col < COLS; col++) {
            bool pressed = !(readValue & (1 << (col + 4)));
            SerialBT.print(col);
            SerialBT.print(":");
            SerialBT.print(pressed ? "PRESS" : "FREE");
            if (col < COLS - 1) SerialBT.print(", ");
        }
        SerialBT.println();
    }
    
    // Reset all pins HIGH
    Wire.beginTransmission(PCF8574_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
    SerialBT.println("Test complete. Press any key and watch for detection...");
}