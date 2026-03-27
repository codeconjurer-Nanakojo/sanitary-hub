#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "PCA9685.h"

// =============================================================
//  SYSTEM CREDENTIALS
//  ADMIN_PASSWORD is no longer a hardcoded define.
//  It is stored in NVS (Preferences) under key "adminpw".
//  On first boot the machine enters setup mode and the admin
//  sets the password via the web dashboard before use.
//  AP_SSID / AP_PASS are the hotspot credentials for the
//  local admin Wi-Fi — change AP_PASS before deployment.
// =============================================================
#define AP_SSID  "Sanitary_Hub_Pro"
#define AP_PASS  "admin123"

// Default password written to NVS on very first boot only.
// The admin should change it immediately via the dashboard.
#define DEFAULT_ADMIN_PASSWORD  "changeme"

// =============================================================
//  SUPABASE CONFIG
//  Fill these in before flashing. Left blank intentionally.
// =============================================================
#define SUPABASE_URL    ""
#define SUPABASE_KEY    ""

// =============================================================
//  PCA9685 SERVO DRIVER
// =============================================================
#define SERVO_FREQ      50
#define STOP_PULSE      360
#define CW_PULSE        300
#define CCW_PULSE       400

#define CHAN_A          0
#define CHAN_B          1
#define CHAN_C          2
#define CHAN_D          3

#define TIME_PER_90_DEG 300   // ms per 90 degrees of coil rotation

// =============================================================
//  KEYPAD PINS
// =============================================================
#define KP_ROWS         4
#define KP_COLS         4

// =============================================================
//  STOCK & LIMITS
// =============================================================
#define LOW_STOCK_THRESHOLD  3   // LCD will warn below this level

// =============================================================
//  ACTIVITY LOG
// =============================================================
#define HISTORY_SIZE    5

// =============================================================
//  IDLE TIMEOUT
//  If a user enters their ID but doesn't pick a product within
//  this many milliseconds, the machine resets to WAITING_ID.
// =============================================================
#define IDLE_TIMEOUT_MS 30000UL   // 30 seconds

// =============================================================
//  ACTIVITY RECORD STRUCT
// =============================================================
struct Activity {
  String id;
  int    d;
  int    m;
  String timeStr;
  bool   active = false;
};

// =============================================================
//  FINGERPRINT SENSOR
// =============================================================
#define FINGER_RX               5   // Hardware Serial(2) RX
#define FINGER_TX              18   // Hardware Serial(2) TX
#define FINGERPRINT_ENROLL_TIMEOUT  60000UL  // 60 seconds

// =============================================================
//  MACHINE STATE
// =============================================================
enum MachineState { WAITING_ID, WAITING_FINGER, CHOOSING_PRODUCT };

// =============================================================
//  GLOBALLY SHARED OBJECTS  (defined in SanitaryHub.ino)
// =============================================================
extern PCA9685           driver;
extern LiquidCrystal_I2C lcd;
extern WebServer         server;
extern Preferences       settings;
extern Preferences       ledger;

// =============================================================
//  GLOBALLY SHARED VARIABLES  (defined in SanitaryHub.ino)
// =============================================================
extern int    stA, stB, stC, stD;
extern int    dailyLimit, monthlyLimit;
extern String entityName, locationName, staSSID, staPASS;
extern String cloudLog;
extern String adminPassword;          // Fix 1: loaded from NVS, not hardcoded

// Fix 5: cumulative per-slot dispense totals (persisted in NVS)
extern int    totalA, totalB, totalC, totalD;

extern Activity     history[HISTORY_SIZE];
extern MachineState hubState;
extern String       currentID;
extern unsigned long lastKeyPressTime;
extern unsigned long productSelectTime;
extern int          webResetChannel;
extern bool         otaActive;        // Fix 2: true while OTA is running

// =============================================================
//  FINGERPRINT GLOBALS  (defined in SanitaryHub.ino)
// =============================================================
extern bool    isEnrolling;
extern int     enrollmentStep;
extern unsigned long enrollmentTimer;
extern int     validatedFingerID;
extern int     fingerTries;
