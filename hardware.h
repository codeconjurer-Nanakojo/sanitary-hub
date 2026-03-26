#pragma once
#include "config.h"

// --- Servo / Coil Control ---
void initHardware();
void dispenseAction(char choice);
void performReset(int chan, char label);
void stopAllCoils();

// --- LCD Helpers ---
void lcdMessage(const char* line1, const char* line2 = "");
void lcdReady();
