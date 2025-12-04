#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

// Menu structures
struct MenuItem {
    String title;
    String action;
    bool isSubmenu;
};

struct Menu {
    String title;
    MenuItem items[8];
    int itemCount;
    int selectedItem;
};

// Display state
struct DisplayState {
    bool initialized = false;
    String currentScreen = "menu";
    int currentLine = 0;
    unsigned long lastUpdate = 0;
    String statusLine = "";
    String messages[6]; // 6 lines for messages
    int messageIndex = 0;
    
    // Menu system
    Menu currentMenu;
    Menu menuStack[5]; // Stack for nested menus
    int menuStackDepth = 0;
    bool inMenu = true;
    
    // Input system for menu parameters
    bool inputMode = false;
    String inputPrompt = "";
    String inputValue = "";
    String pendingAction = "";
    
    // Multi-step input system
    bool multiStepInput = false;
    int inputStep = 0;
    String stepValues[3]; // Support up to 3 input steps
    String stepPrompts[3];
    
    // Command output capture
    String lastCommandOutput = "";
    bool captureOutput = false;
};

extern DisplayState displayState;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// Display functions
void initializeDisplay();
void updateDisplay();
void showMainScreen();
void showStatusScreen();
void showGPSScreen();
void showGSMScreen();
void addMessage(String message);
void showMessage(String message, int duration = 2000);
void displayError(String error);
void displaySuccess(String success);

// Menu system functions
void initializeMenus();
void showMenu();
void navigateUp();
void navigateDown();
void selectMenuItem();
void goBack();
void createMainMenu();
void createRadioMenu();
void createSettingsMenu();
void createGPSMenu();
void createGSMMenu();
void createEncryptionMenu();
void createDebugMenu();
void createRadioConfigMenu();
void createSMSMenu();
void executeMenuAction(String action);
void startInput(String prompt, String action);
void startMultiStepInput(String action, String prompts[], int stepCount);
void handleMultiStepInput(char c);
void nextInputStep();
void completeMultiStepInput();
void handleInput(char c);
void cancelInput();
void confirmInput();
void showInputScreen();
void captureCommandOutput(String command);
void displayCapturedOutput();