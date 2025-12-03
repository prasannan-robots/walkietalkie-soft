#include "DisplayManager.h"
#include "GPSManager.h"
#include "GSMManager.h"
#include "WalkieTalkie.h"

DisplayState displayState;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

void initializeDisplay() {
    // Initialize U8g2
    u8g2.begin();
    u8g2.enableUTF8Print();
    
    displayState.initialized = true;
    
    // Show startup screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "DMR Walkie-Talkie");
    u8g2.drawHLine(0, 12, 128);
    u8g2.drawStr(0, 25, "GPS: Initializing...");
    u8g2.drawStr(0, 35, "GSM: Initializing...");
    u8g2.drawStr(0, 45, "DMR: Initializing...");
    u8g2.drawStr(0, 60, "Use keypad to navigate");
    u8g2.sendBuffer();
    
    delay(2000);
    
    // Initialize menu system
    initializeMenus();
    showMenu();
}

void updateDisplay() {
    if (!displayState.initialized) return;
    
    // Update display every 500ms or when needed
    if (millis() - displayState.lastUpdate > 500) {
        displayState.lastUpdate = millis();
        
        if (displayState.inputMode) {
            showInputScreen();
        } else if (displayState.inMenu) {
            showMenu();
        } else if (displayState.currentScreen == "main") {
            showMainScreen();
        } else if (displayState.currentScreen == "status") {
            showStatusScreen();
        } else if (displayState.currentScreen == "gps") {
            showGPSScreen();
        } else if (displayState.currentScreen == "gsm") {
            showGSMScreen();
        }
    }
}

void showMainScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // Title
    u8g2.drawStr(0, 10, "DMR Radio System");
    u8g2.drawHLine(0, 12, 128);
    
    // Status indicators
    char statusLine[32];
    snprintf(statusLine, sizeof(statusLine), "CH:%d VOL:%d", wtState.currentChannel, wtState.volume);
    u8g2.drawStr(0, 25, statusLine);
    
    // GPS Status
    if (gpsState.hasValidFix) {
        u8g2.drawStr(0, 35, "GPS: LOCK");
    } else {
        u8g2.drawStr(0, 35, "GPS: SEARCH");
    }
    
    // GSM Status  
    char gsmLine[32];
    if (gsmState.networkRegistered) {
        snprintf(gsmLine, sizeof(gsmLine), "GSM: REG %d", gsmState.signalStrength);
    } else {
        snprintf(gsmLine, sizeof(gsmLine), "GSM: NO NET");
    }
    u8g2.drawStr(0, 45, gsmLine);
    
    // Menu options
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 60, "*=Menu #=Back A=Call");
    
    u8g2.sendBuffer();
}

void showStatusScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    u8g2.drawStr(0, 10, "System Status");
    u8g2.drawHLine(0, 12, 128);
    
    char line[32];
    snprintf(line, sizeof(line), "Radio ID: 0x%X", (unsigned int)wtState.myRadioID);
    u8g2.drawStr(0, 25, line);
    
    snprintf(line, sizeof(line), "Channel: %d", wtState.currentChannel);
    u8g2.drawStr(0, 35, line);
    
    snprintf(line, sizeof(line), "GPS Fix: %s", gpsState.hasValidFix ? "YES" : "NO");
    u8g2.drawStr(0, 45, line);
    
    snprintf(line, sizeof(line), "GSM Reg: %s", gsmState.networkRegistered ? "YES" : "NO");
    u8g2.drawStr(0, 55, line);
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 64, "#=Back");
    
    u8g2.sendBuffer();
}

void showGPSScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    u8g2.drawStr(0, 10, "GPS Information");
    u8g2.drawHLine(0, 12, 128);
    
    if (gpsState.hasValidFix) {
        u8g2.drawStr(0, 25, "Status: LOCKED");
        
        char latLine[32];
        snprintf(latLine, sizeof(latLine), "Lat: %.6f", gpsState.latitude);
        u8g2.drawStr(0, 35, latLine);
        
        char lonLine[32]; 
        snprintf(lonLine, sizeof(lonLine), "Lon: %.6f", gpsState.longitude);
        u8g2.drawStr(0, 45, lonLine);
    } else {
        u8g2.drawStr(0, 25, "Status: SEARCHING");
        u8g2.drawStr(0, 35, "Satellites: --");
        u8g2.drawStr(0, 45, "Waiting for fix...");
    }
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 64, "#=Back C=Send");
    
    u8g2.sendBuffer();
}

void showGSMScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    u8g2.drawStr(0, 10, "GSM Information");
    u8g2.drawHLine(0, 12, 128);
    
    char line[32];
    snprintf(line, sizeof(line), "Status: %s", gsmState.networkRegistered ? "REG" : "NO REG");
    u8g2.drawStr(0, 25, line);
    
    snprintf(line, sizeof(line), "Signal: %d/31", gsmState.signalStrength);
    u8g2.drawStr(0, 35, line);
    
    if (gsmState.phoneNumber.length() > 0) {
        snprintf(line, sizeof(line), "Phone: %s", gsmState.phoneNumber.c_str());
        u8g2.drawStr(0, 45, line);
    } else {
        u8g2.drawStr(0, 45, "Phone: Not set");
    }
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(0, 64, "#=Back B=SMS");
    
    u8g2.sendBuffer();
}

void addMessage(String message) {
    displayState.messages[displayState.messageIndex] = message;
    displayState.messageIndex = (displayState.messageIndex + 1) % 6;
}

void showMessage(String message, int duration) {
    if (!displayState.initialized) return;
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tf);  // Use smaller font for more text
    
    // Clean up message - remove emojis and excessive formatting
    message.replace("📤", ">");
    message.replace("📞", ">");
    message.replace("📻", ">");
    message.replace("🔊", ">");
    message.replace("🆔", ">");
    message.replace("🔒", "");
    message.replace("🔓", "");
    message.replace("❌", "X");
    message.replace("✅", "OK");
    message.replace("🔐", "");
    message.replace("📊", "");
    message.replace("📶", "");
    message.replace("📍", "");
    message.replace("📱", "");
    message.replace("🔧", "");
    message.replace("📖", "");
    message.replace("📡", "");
    message.replace("🚨", "!");
    message.replace("🔄", "");
    
    // Split message into lines that fit the screen
    int maxCharsPerLine = 21; // 128px / 6px per char ≈ 21 chars
    int maxLines = 8; // 64px / 8px per line ≈ 8 lines
    String lines[8];
    int lineCount = 0;
    
    int startPos = 0;
    while (startPos < message.length() && lineCount < maxLines) {
        // Find natural break point (newline or word boundary)
        int endPos = startPos + maxCharsPerLine;
        if (endPos >= message.length()) {
            endPos = message.length();
        } else {
            // Try to break at word boundary
            int lastSpace = message.lastIndexOf(' ', endPos);
            int newlinePos = message.indexOf('\n', startPos);
            
            if (newlinePos != -1 && newlinePos < endPos) {
                endPos = newlinePos;
            } else if (lastSpace > startPos && lastSpace < endPos) {
                endPos = lastSpace;
            }
        }
        
        String line = message.substring(startPos, endPos);
        line.trim();
        
        if (line.length() > 0) {
            lines[lineCount] = line;
            lineCount++;
        }
        
        startPos = endPos + 1;
    }
    
    // Display lines with proper spacing
    int lineHeight = 8;
    int startY = 8;
    
    for (int i = 0; i < lineCount; i++) {
        u8g2.drawStr(0, startY + (i * lineHeight), lines[i].c_str());
    }
    
    // Add scroll indicator if message was truncated
    if (startPos < message.length()) {
        u8g2.drawStr(115, 62, "...");
    }
    
    // Add back indicator
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0, 62, "#=Back");
    
    u8g2.sendBuffer();
    delay(duration);
    
    // Return to main screen after showing message
    if (!displayState.inMenu) {
        showMainScreen();
    }
}

void displayError(String error) {
    showMessage("ERROR: " + error, 3000);
}

void displaySuccess(String success) {
    showMessage("OK: " + success, 2000);
}

// =============== MENU SYSTEM ===============

void initializeMenus() {
    createMainMenu();
}

void createMainMenu() {
    displayState.currentMenu.title = "Main Menu";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 8;
    
    displayState.currentMenu.items[0] = {"Radio Control", "radio_menu", true};
    displayState.currentMenu.items[1] = {"Radio Config", "radio_config_menu", true};
    displayState.currentMenu.items[2] = {"Encryption", "encryption_menu", true};
    displayState.currentMenu.items[3] = {"SMS & GSM", "sms_menu", true};
    displayState.currentMenu.items[4] = {"GPS Control", "gps_menu", true};
    displayState.currentMenu.items[5] = {"Debug Tools", "debug_menu", true};
    displayState.currentMenu.items[6] = {"System Info", "system_info", false};
    displayState.currentMenu.items[7] = {"Exit Menu", "exit_menu", false};
}

void createRadioMenu() {
    displayState.currentMenu.title = "Radio Control";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 8;
    
    displayState.currentMenu.items[0] = {"Call Radio ID", "input_call", false};
    displayState.currentMenu.items[1] = {"Group Call", "input_group", false};
    displayState.currentMenu.items[2] = {"Emergency Call", "emergency", false};
    displayState.currentMenu.items[3] = {"Stop Call", "stop", false};
    displayState.currentMenu.items[4] = {"Send Position", "send_position", false};
    displayState.currentMenu.items[5] = {"Radio Status", "status", false};
    displayState.currentMenu.items[6] = {"Fallback Mode", "fallback", false};
    displayState.currentMenu.items[7] = {"Back", "back", false};
}

void createGPSMenu() {
    displayState.currentMenu.title = "GPS Control";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 6;
    
    displayState.currentMenu.items[0] = {"GPS Status", "gps_position", false};
    displayState.currentMenu.items[1] = {"Send GPS Once", "input_gps", false};
    displayState.currentMenu.items[2] = {"Auto GPS Send", "input_gpsauto", false};
    displayState.currentMenu.items[3] = {"Stop Auto GPS", "gpsstop", false};
    displayState.currentMenu.items[4] = {"Detailed Info", "gps_satellites", false};
    displayState.currentMenu.items[5] = {"Back", "back", false};
}

void createGSMMenu() {
    displayState.currentMenu.title = "GSM Control";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 4;
    
    displayState.currentMenu.items[0] = {"Network Status", "gsmstatus", false};
    displayState.currentMenu.items[1] = {"Signal Strength", "gsmstatus", false};
    displayState.currentMenu.items[2] = {"Send SMS Test", "input_gsmsms", false};
    displayState.currentMenu.items[3] = {"Back", "back", false};
}

void createRadioConfigMenu() {
    displayState.currentMenu.title = "Radio Config";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 6;
    
    displayState.currentMenu.items[0] = {"Set Radio ID", "input_radioid", false};
    displayState.currentMenu.items[1] = {"Set Channel", "input_channel", false};
    displayState.currentMenu.items[2] = {"Set Volume", "input_volume", false};
    displayState.currentMenu.items[3] = {"Radio Info", "info", false};
    displayState.currentMenu.items[4] = {"Send Raw CMD", "input_raw", false};
    displayState.currentMenu.items[5] = {"Back", "back", false};
}

void createEncryptionMenu() {
    displayState.currentMenu.title = "Encryption";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 5;
    
    displayState.currentMenu.items[0] = {"Enable Encrypt", "encrypt on", false};
    displayState.currentMenu.items[1] = {"Disable Encrypt", "encrypt off", false};
    displayState.currentMenu.items[2] = {"Set Encrypt Key", "input_encryptkey", false};
    displayState.currentMenu.items[3] = {"Encrypt Status", "encrypt status", false};
    displayState.currentMenu.items[4] = {"Back", "back", false};
}

void createSMSMenu() {
    displayState.currentMenu.title = "SMS & GSM";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 7;
    
    displayState.currentMenu.items[0] = {"Send SMS", "input_sms", false};
    displayState.currentMenu.items[1] = {"Send GSM SMS", "input_gsmsms", false};
    displayState.currentMenu.items[2] = {"Set GSM Phone", "input_gsmphone", false};
    displayState.currentMenu.items[3] = {"GSM Status", "gsmstatus", false};
    displayState.currentMenu.items[4] = {"SMS Info", "smsinfo", false};
    displayState.currentMenu.items[5] = {"GSM Control", "gsm_menu", true};
    displayState.currentMenu.items[6] = {"Back", "back", false};
}

void createDebugMenu() {
    displayState.currentMenu.title = "Debug Tools";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 6;
    
    displayState.currentMenu.items[0] = {"I2C Scan", "i2cscan", false};
    displayState.currentMenu.items[1] = {"Key Test", "keytest", false};
    displayState.currentMenu.items[2] = {"Key Scan", "keyscan", false};
    displayState.currentMenu.items[3] = {"Bluetooth Info", "bt", false};
    displayState.currentMenu.items[4] = {"Help", "help", false};
    displayState.currentMenu.items[5] = {"Back", "back", false};
}

void createSettingsMenu() {
    displayState.currentMenu.title = "Settings";
    displayState.currentMenu.selectedItem = 0;
    displayState.currentMenu.itemCount = 4;
    
    displayState.currentMenu.items[0] = {"Display Settings", "display_settings", false};
    displayState.currentMenu.items[1] = {"Radio Settings", "radio_settings", false};
    displayState.currentMenu.items[2] = {"Debug Mode", "debug_mode", false};
    displayState.currentMenu.items[3] = {"Back", "back", false};
}

void showMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // Title
    u8g2.drawStr(0, 10, displayState.currentMenu.title.c_str());
    u8g2.drawHLine(0, 12, 128);
    
    // Menu items (show up to 5 items)
    int startItem = displayState.currentMenu.selectedItem > 2 ? displayState.currentMenu.selectedItem - 2 : 0;
    int endItem = min(startItem + 5, displayState.currentMenu.itemCount);
    
    for (int i = startItem; i < endItem; i++) {
        int y = 25 + (i - startItem) * 10;
        
        // Selection indicator
        if (i == displayState.currentMenu.selectedItem) {
            u8g2.drawBox(0, y - 8, 128, 9);
            u8g2.setColorIndex(0); // Invert color for selected item
        }
        
        // Menu item text
        String itemText = displayState.currentMenu.items[i].title;
        if (displayState.currentMenu.items[i].isSubmenu) {
            itemText += " >";
        }
        u8g2.drawStr(2, y, itemText.c_str());
        
        if (i == displayState.currentMenu.selectedItem) {
            u8g2.setColorIndex(1); // Reset color
        }
    }
    
    // Navigation help
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0, 62, "2/8:Up/Down 5:Select #:Back");
    
    u8g2.sendBuffer();
}

void navigateUp() {
    if (displayState.currentMenu.selectedItem > 0) {
        displayState.currentMenu.selectedItem--;
        showMenu();
    }
}

void navigateDown() {
    if (displayState.currentMenu.selectedItem < displayState.currentMenu.itemCount - 1) {
        displayState.currentMenu.selectedItem++;
        showMenu();
    }
}

void selectMenuItem() {
    MenuItem selectedItem = displayState.currentMenu.items[displayState.currentMenu.selectedItem];
    
    if (selectedItem.isSubmenu) {
        // Push current menu to stack
        if (displayState.menuStackDepth < 4) {
            displayState.menuStack[displayState.menuStackDepth] = displayState.currentMenu;
            displayState.menuStackDepth++;
        }
        
        // Load submenu
        if (selectedItem.action == "radio_menu") {
            createRadioMenu();
        } else if (selectedItem.action == "radio_config_menu") {
            createRadioConfigMenu();
        } else if (selectedItem.action == "encryption_menu") {
            createEncryptionMenu();
        } else if (selectedItem.action == "sms_menu") {
            createSMSMenu();
        } else if (selectedItem.action == "gps_menu") {
            createGPSMenu();
        } else if (selectedItem.action == "gsm_menu") {
            createGSMMenu();
        } else if (selectedItem.action == "debug_menu") {
            createDebugMenu();
        } else if (selectedItem.action == "settings_menu") {
            createSettingsMenu();
        }
        
        showMenu();
    } else {
        executeMenuAction(selectedItem.action);
    }
}

void goBack() {
    if (displayState.menuStackDepth > 0) {
        // Pop menu from stack
        displayState.menuStackDepth--;
        displayState.currentMenu = displayState.menuStack[displayState.menuStackDepth];
        showMenu();
    } else {
        // Exit menu system
        displayState.inMenu = false;
        showMainScreen();
    }
}

void executeMenuAction(String action) {
    // Handle input actions
    if (action.startsWith("input_")) {
        if (action == "input_call") {
            startInput("Enter Radio ID:", "call ");
        } else if (action == "input_group") {
            startInput("Enter Group ID:", "group ");
        } else if (action == "input_radioid") {
            startInput("Enter Radio ID:", "radioid ");
        } else if (action == "input_channel") {
            startInput("Enter Channel:", "channel ");
        } else if (action == "input_volume") {
            startInput("Enter Volume:", "volume ");
        } else if (action == "input_encryptkey") {
            startInput("Enter Key (32 hex):", "encryptkey ");
        } else if (action == "input_sms") {
            startInput("Enter ID:Message:", "sms ");
        } else if (action == "input_gsmsms") {
            startInput("Enter Phone:Msg:", "gsmsms ");
        } else if (action == "input_gsmphone") {
            startInput("Enter Phone #:", "gsmphone ");
        } else if (action == "input_raw") {
            startInput("Enter Raw CMD:", "raw ");
        } else if (action == "input_gps") {
            startInput("Enter Radio ID:", "gps ");
        } else if (action == "input_gpsauto") {
            startInput("Enter ID Minutes:", "gpsauto ");
        }
        return;
    }
    
    // Handle direct commands
    if (action == "exit_menu") {
        displayState.inMenu = false;
        showMainScreen();
    } else if (action == "back") {
        goBack();
    } else if (action == "send_position") {
        startInput("Enter Radio ID:", "gps ");
    } else if (action == "emergency") {
        processCommand(&SerialBT, "emergency");
        displaySuccess("Emergency signal sent");
    } else if (action == "stop") {
        processCommand(&SerialBT, "stop");
        displaySuccess("Call stopped");
    } else if (action == "fallback") {
        processCommand(&SerialBT, "fallback");
        displaySuccess("Fallback mode activated");
    } else if (action == "encrypt on") {
        processCommand(&SerialBT, "encrypt on");
        displaySuccess("Encryption enabled");
    } else if (action == "encrypt off") {
        processCommand(&SerialBT, "encrypt off");
        displaySuccess("Encryption disabled");
    } else if (action == "encrypt status") {
        captureCommandOutput("encrypt status");
        processCommand(&SerialBT, "encrypt status");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "status") {
        captureCommandOutput("status");
        processCommand(&SerialBT, "status");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "info") {
        captureCommandOutput("info");
        processCommand(&SerialBT, "info");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "help") {
        captureCommandOutput("help");
        processCommand(&SerialBT, "help");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "gsmstatus") {
        captureCommandOutput("gsmstatus");
        processCommand(&SerialBT, "gsmstatus");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "smsinfo") {
        captureCommandOutput("smsinfo");
        processCommand(&SerialBT, "smsinfo");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "bt") {
        captureCommandOutput("bt");
        processCommand(&SerialBT, "bt");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "i2cscan") {
        captureCommandOutput("i2cscan");
        processCommand(&SerialBT, "i2cscan");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "keytest") {
        captureCommandOutput("keytest");
        processCommand(&SerialBT, "keytest");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "keyscan") {
        captureCommandOutput("keyscan");
        processCommand(&SerialBT, "keyscan");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "gps_position") {
        captureCommandOutput("gpsinfo");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "gps_satellites") {
        captureCommandOutput("gpsinfo");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "gps_reset" || action == "gpsstop") {
        captureCommandOutput("gpsstop");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "gsm_signal") {
        captureCommandOutput("gsmstatus");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "system_info") {
        captureCommandOutput("info");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "display_settings") {
        captureCommandOutput("info");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "radio_settings") {
        captureCommandOutput("status");
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else if (action == "debug_mode") {
        displaySuccess("Debug mode toggled");
        displayState.inMenu = false;
        delay(500);
    }
}

// =============== INPUT SYSTEM ===============

void startInput(String prompt, String action) {
    displayState.inputMode = true;
    displayState.inputPrompt = prompt;
    displayState.inputValue = "";
    displayState.pendingAction = action;
    showInputScreen();
}

void handleInput(char c) {
    if (!displayState.inputMode) return;
    
    if (c == '#') {
        // Cancel input
        cancelInput();
    } else if (c == '*') {
        // Confirm input
        confirmInput();
    } else if (c == 'C') {
        // Backspace
        if (displayState.inputValue.length() > 0) {
            displayState.inputValue.remove(displayState.inputValue.length() - 1);
            showInputScreen();
        }
    } else if (c != 0) {
        // Add character
        displayState.inputValue += c;
        showInputScreen();
    }
}

void cancelInput() {
    displayState.inputMode = false;
    showMenu();
}

void confirmInput() {
    if (displayState.inputValue.length() > 0) {
        String fullCommand = displayState.pendingAction + displayState.inputValue;
        captureCommandOutput(fullCommand);
        processCommand(&SerialBT, fullCommand);
        displayState.inputMode = false;
        displayCapturedOutput();
        displayState.inMenu = false;
        delay(500);
    } else {
        displayState.inputMode = false;
        showMenu();
    }
}

void showInputScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // Title
    u8g2.drawStr(0, 10, "Input Required");
    u8g2.drawHLine(0, 12, 128);
    
    // Prompt
    u8g2.drawStr(0, 25, displayState.inputPrompt.c_str());
    
    // Input value with cursor
    String displayValue = displayState.inputValue + "_";
    u8g2.drawStr(0, 40, displayValue.c_str());
    
    // Help text
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0, 55, "Enter value, * to confirm");
    u8g2.drawStr(0, 62, "# to cancel, C to backspace");
    
    u8g2.sendBuffer();
}

// =============== OUTPUT CAPTURE SYSTEM ===============

class CaptureStream : public Stream {
public:
    String capturedText = "";
    
    // Required Stream methods
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    
    // Print methods to capture output
    size_t write(uint8_t c) override {
        if (c == '\r') return 1; // Skip carriage returns
        capturedText += (char)c;
        return 1;
    }
    
    size_t write(const uint8_t *buffer, size_t size) override {
        for (size_t i = 0; i < size; i++) {
            write(buffer[i]);
        }
        return size;
    }
    
    void clear() {
        capturedText = "";
    }
};

CaptureStream captureStream;

void captureCommandOutput(String command) {
    captureStream.clear();
    processCommand(&captureStream, command);
    displayState.lastCommandOutput = captureStream.capturedText;
}

void displayCapturedOutput() {
    if (displayState.lastCommandOutput.length() > 0) {
        showMessage(displayState.lastCommandOutput, 5000);
        displayState.lastCommandOutput = "";
    }
}