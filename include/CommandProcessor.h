#pragma once

#include <Arduino.h>

// Command processing functions
void processCommand(Stream* stream, String command);
void showCommands();
void showCommandsTo(Stream* stream);
void showStatus();
void showStatusTo(Stream* stream);
void showDeviceInfo();
void showDeviceInfoTo(Stream* stream);