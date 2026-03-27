/*
 * ============================================================
 *  SANITARY HUB — Modular ESP32 Firmware v7.1
 *  Target:  ESP32 + PCA9685 + 4x4 Keypad + I2C LCD 16x2 + Fingerprint
 * ============================================================
 *  Module layout:
 *    config.h        — constants, structs, extern declarations
 *    hardware.*      — servo/coil control, LCD helpers
 *    storage.*       — SPIFFS + NVS load/save
 *    usage.*         — user verification, daily/monthly limits
 *    user.*          — keypad state machine, dispense logic
 *    fingerprint.*   — fingerprint enrollment + verification
 *    web.*           — HTTP routes + admin dashboard
 *    cloud.*         — Supabase integration
 *    ota.*           — ArduinoOTA over-the-air firmware updates
 * ============================================================
 */

#include "../config.h"
#include "../hardware.h"
#include "../storage.h"
#include "../usage.h"
#include "../user.h"
#include "../fingerprint.h"
#include "../web.h"
#include "../cloud.h"
#include "../ota.h"

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
int    totalA = 0, totalB = 0, totalC = 0, totalD = 0;

String entityName    = "UMaT";
String locationName  = "Main";
String staSSID       = "";
String staPASS       = "";
String cloudLog      = "System Ready";
String adminPassword = "";

Activity     history[HISTORY_SIZE];
MachineState hubState           = WAITING_ID;
String       currentID          = "";
unsigned long lastKeyPressTime  = 0;
unsigned long productSelectTime = 0;
int          webResetChannel    = -1;
int          webDispenseChannel = -1;
bool         otaActive          = false;

bool           isEnrolling       = false;
int            enrollmentStep    = 0;
unsigned long  enrollmentTimer   = 0;
int            validatedFingerID = -1;
int            fingerTries       = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed! Halting.");
    while (true) delay(1000);
  }

  initHardware();
  initFingerprint();

  loadSystemData();
  loadStock();
  loadSlotTotals();
  loadHistory();

  initKeypad();

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  if (staSSID.length() > 1) {
    WiFi.begin(staSSID.c_str(), staPASS.c_str());
    Serial.print("Connecting to: ");
    Serial.println(staSSID);
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

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  initWebServer();
  initOTA();

  Serial.println("=== SANITARY HUB v7.1 READY ===");
  lcdReady();
}

void loop() {
  if (handleOTA()) return;

  server.handleClient();

  if (isEnrolling && enrollmentStep == 0) {
    if (millis() - enrollmentTimer > FINGERPRINT_ENROLL_TIMEOUT) {
      lcdMessage("Enroll Timeout", "");
      delay(2000);
      finishEnrollment(false);
    }
  }

  if (isEnrolling) {
    enrollmentSequence();
  }

  if (webResetChannel != -1) {
    char label = 'A' + webResetChannel;
    performReset(webResetChannel, label);
    webResetChannel = -1;
  }

  if (webDispenseChannel != -1) {
    char slot = 'A' + webDispenseChannel;
    int* stockPtr = (slot == 'A') ? &stA :
                    (slot == 'B') ? &stB :
                    (slot == 'C') ? &stC : &stD;

    if (*stockPtr > 0) {
      dispenseAction(slot);
      (*stockPtr)--;
      incrementSlotTotal(slot);
      saveStock();
      lcdMessage("Admin Dispense", ("Slot " + String(slot)).c_str());
      delay(1000);
    } else {
      lcdMessage("Slot Empty", ("Slot " + String(slot)).c_str());
      delay(1000);
    }

    lcdReady();
    webDispenseChannel = -1;
  }

  handleKeypad();

  if (hubState == WAITING_FINGER) {
    checkFingerprint();
  }

  checkIdleTimeout();
}
