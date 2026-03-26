#pragma once
#include "config.h"

void initKeypad();
void handleKeypad();       // Called every loop()
void handleIDInput();
void handleDispense(char choice);
void checkIdleTimeout();   // NEW: resets state if user goes idle
