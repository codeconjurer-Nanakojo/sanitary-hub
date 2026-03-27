#pragma once
#include "config.h"

void initKeypad();
void handleKeypad();       // Called every loop()
void handleIDInput();
void handleDispense(char choice);
void checkIdleTimeout();   // Resets state if user goes idle

