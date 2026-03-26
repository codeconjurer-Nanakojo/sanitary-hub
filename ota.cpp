#include "ota.h"
#include "hardware.h"
#include <ArduinoOTA.h>

// ---------------------------------------------------------------
//  initOTA
//  Sets up ArduinoOTA with a hostname and the admin password
//  (loaded from NVS) so only authorised users can push firmware.
//
//  How to use from Arduino IDE:
//    1. Connect your laptop to the same WiFi as the ESP32
//       (either the router the hub joined, or the hub's own AP).
//    2. In Arduino IDE → Tools → Port, you should see
//       "sanitary-hub at <IP>" appear as a network port.
//    3. Select that port and upload as normal.
//    4. You will be prompted for the OTA password — enter the
//       same password you set in the admin dashboard.
//
//  NOTE: OTA is only available when WiFi (STA mode) is connected.
//        It does NOT work over the built-in AP hotspot because
//        the Arduino IDE needs to be on the same network segment.
// ---------------------------------------------------------------
void initOTA() {
  if (WiFi.status() != WL_CONNECTED) return;

  ArduinoOTA.setHostname("sanitary-hub");

  // Use the runtime admin password (from NVS) — not a hardcoded string
  ArduinoOTA.setPassword(adminPassword.c_str());

  ArduinoOTA.onStart([]() {
    otaActive = true;
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
    Serial.println("OTA Start: uploading " + type);
    lcdMessage("OTA Update...", "Do not power off");
  });

  ArduinoOTA.onEnd([]() {
    otaActive = false;
    Serial.println("OTA complete. Rebooting...");
    lcdMessage("OTA Done!", "Rebooting...");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pct = progress / (total / 100);
    Serial.printf("OTA Progress: %u%%\n", pct);
    // Update LCD every 10% so it doesn't flicker on every byte
    if (pct % 10 == 0) {
      lcd.setCursor(0, 1);
      lcd.print("Progress: " + String(pct) + "%   ");
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaActive = false;
    Serial.printf("OTA Error [%u]: ", error);
    String msg;
    if      (error == OTA_AUTH_ERROR)    msg = "Auth failed";
    else if (error == OTA_BEGIN_ERROR)   msg = "Begin failed";
    else if (error == OTA_CONNECT_ERROR) msg = "Connect failed";
    else if (error == OTA_RECEIVE_ERROR) msg = "Receive failed";
    else if (error == OTA_END_ERROR)     msg = "End failed";
    else                                  msg = "Unknown error";
    Serial.println(msg);
    lcdMessage("OTA Error!", msg.c_str());
    delay(3000);
    lcdReady();
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready. Hostname: sanitary-hub");
}

// ---------------------------------------------------------------
//  handleOTA
//  Must be called every loop() when WiFi is connected.
//  Returns true while an update is in progress so the main
//  loop can pause dispensing and keypad logic safely.
// ---------------------------------------------------------------
bool handleOTA() {
  if (WiFi.status() != WL_CONNECTED) return false;
  ArduinoOTA.handle();
  return otaActive;
}
