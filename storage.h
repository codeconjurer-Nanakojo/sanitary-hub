#pragma once
#include "config.h"

void loadSystemData();
void saveAdminPassword(String newPass);  // Fix 1: write new password to NVS

void loadStock();
void saveStock();

void loadSlotTotals();                   // Fix 5: cumulative dispense counts
void saveSlotTotals();
void incrementSlotTotal(char slot);

void loadHistory();
void saveHistory();
void logActivity(String id, int d, int m);
String getTimestamp();

// Fingerprint map persistence
void clearFingerprintMap();
