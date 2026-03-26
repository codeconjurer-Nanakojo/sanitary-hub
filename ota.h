#pragma once
#include "config.h"

// Call once from setup() after WiFi is connected
void initOTA();

// Call every loop() — handles OTA negotiation
// Returns true if an OTA update is currently in progress
// (so loop() can skip keypad/dispense logic during flash)
bool handleOTA();
