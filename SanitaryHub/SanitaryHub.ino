/*
 * ============================================================
 *  SANITARY HUB — Modular ESP32 Firmware
 *  Version: 7.0
 *  Target:  ESP32 + PCA9685 + 4x4 Keypad + I2C LCD 16x2
 * ============================================================
 *  Module layout:
 *    config.h      — constants, structs, extern declarations
 *    hardware.*    — servo/coil control, LCD helpers
 *    storage.*     — SPIFFS + NVS load/save
 *    usage.*       — user verification, daily/monthly limits
 *    user.*        — keypad state machine, dispense logic
 *    web.*         — HTTP routes + admin dashboard
 *    cloud.*       — Supabase integration
 *    ota.*         — ArduinoOTA over-the-air firmware updates
 *
 *  Changes in v7.0:
 *    Fix 1 — Admin password stored in NVS, changeable via dashboard
 *    Fix 2 — OTA firmware updates via ArduinoOTA
 *    Fix 5 — Per-slot cumulative dispense totals on dashboard
 * ============================================================
 */

#include "config.h"
#include "hardware.h"
#include "storage.h"
#include "usage.h"
#include "user.h"
#include "web.h"
#include "cloud.h"
#include "ota.h"

// ============================================================
//  GLOBAL OBJECT & VARIABLE DEFINITIONS
//  (declared extern in config.h, defined exactly once here)
// ============================================================
PCA9685           driver;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
WebServer         server(80);
Preferences       settings;
Preferences       ledger;

int    stA = 0, stB = 0, stC = 0, stD = 0;
int    dailyLimit   = 2;
int    monthlyLimit = 10;

// Fix 5: cumulative slot dispense totals
int    totalA = 0, totalB = 0, totalC = 0, totalD = 0;

String entityName    = "UMaT";
String locationName  = "Main";
String staSSID       = "";
String staPASS       = "";
String cloudLog      = "System Ready";
String adminPassword = "";       // Fix 1: loaded from NVS in loadSystemData()

Activity     history[HISTORY_SIZE];
MachineState hubState           = WAITING_ID;
String       currentID          = "";
unsigned long lastKeyPressTime  = 0;
unsigned long productSelectTime = 0;
int          webResetChannel    = -1;
bool         otaActive          = false;  // Fix 2: set true during OTA flash

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // --- Filesystem ---
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed! Halting.");
    while (true) delay(1000);
  }

  // --- Hardware (LCD + PCA9685) ---
  initHardware();

  // --- Load persisted data ---
  loadSystemData();    // Also loads adminPassword from NVS (Fix 1)
  loadStock();
  loadSlotTotals();    // Fix 5: load cumulative totals
  loadHistory();

  // --- Keypad ---
  initKeypad();

  // --- WiFi ---
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (staSSID.length() > 1) {
    WiFi.begin(staSSID.c_str(), staPASS.c_str());
    Serial.print("Connecting to: ");
    Serial.println(staSSID);
    // Wait up to 8 seconds for STA connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 16) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nSTA connected. IP: " + WiFi.localIP().toString());
    } else {
      Serial.println("\nSTA connection failed — running AP-only.");
    }
  }

  // --- Time (NTP) — Ghana is UTC+0 ---
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // --- Web server ---
  initWebServer();

  // --- OTA (Fix 2) — only starts if STA WiFi connected ---
  initOTA();

  Serial.println("=== SANITARY HUB v7.0 READY ===");
  lcdReady();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  // Fix 2: handle OTA — skip all other logic while flashing
  if (handleOTA()) return;

  // Handle incoming HTTP requests
  server.handleClient();

  // Execute any coil reset queued from the web dashboard
  if (webResetChannel != -1) {
    char label = 'A' + webResetChannel;
    performReset(webResetChannel, label);
    webResetChannel = -1;
  }

  // Process keypad input
  handleKeypad();

  // Check for idle session timeout
  checkIdleTimeout();
}
