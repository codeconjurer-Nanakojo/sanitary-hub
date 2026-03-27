#include "storage.h"

// ---------------------------------------------------------------
//  loadSystemData
//  Reads persisted config from NVS (entity name, limits, WiFi).
//
//  Fix 1 — Admin password:
//  Stored in NVS under key "adminpw". On first boot the default
//  password is written in so the admin can log in and change it.
// ---------------------------------------------------------------
void loadSystemData() {
  settings.begin("config", false);  // false = read-write (needed for first-boot write)

  entityName   = settings.getString("en",   "UMaT");
  locationName = settings.getString("loc",  "Main");
  dailyLimit   = settings.getInt("dl",      2);
  monthlyLimit = settings.getInt("ml",      10);
  staSSID      = settings.getString("ssid", "");
  staPASS      = settings.getString("pw",   "");

  // First boot check: write default password if none exists
  if (!settings.isKey("adminpw")) {
    adminPassword = DEFAULT_ADMIN_PASSWORD;
    settings.putString("adminpw", adminPassword);
    Serial.println("First boot: default admin password written to NVS.");
    Serial.println(">>> CHANGE IT IN THE DASHBOARD IMMEDIATELY <<<");
  } else {
    adminPassword = settings.getString("adminpw", DEFAULT_ADMIN_PASSWORD);
  }

  settings.end();
}

// ---------------------------------------------------------------
//  saveAdminPassword  (Fix 1)
//  Called from the web dashboard when admin changes the password.
// ---------------------------------------------------------------
void saveAdminPassword(String newPass) {
  settings.begin("config", false);
  settings.putString("adminpw", newPass);
  settings.end();
  adminPassword = newPass;
  Serial.println("Admin password updated in NVS.");
}

// ---------------------------------------------------------------
//  loadStock  /  saveStock
//  Persists the four stock counters to SPIFFS as a simple CSV line.
// ---------------------------------------------------------------
void loadStock() {
  if (!SPIFFS.exists("/stock.txt")) return;
  File f = SPIFFS.open("/stock.txt", "r");
  if (!f) return;
  String s = f.readString();
  f.close();

  int c1 = s.indexOf(',');
  int c2 = s.indexOf(',', c1 + 1);
  int c3 = s.indexOf(',', c2 + 1);

  if (c1 != -1 && c2 != -1 && c3 != -1) {
    stA = s.substring(0, c1).toInt();
    stB = s.substring(c1 + 1, c2).toInt();
    stC = s.substring(c2 + 1, c3).toInt();
    stD = s.substring(c3 + 1).toInt();
  }
}

void saveStock() {
  File f = SPIFFS.open("/stock.txt", "w");
  if (!f) return;
  f.printf("%d,%d,%d,%d", stA, stB, stC, stD);
  f.close();
}

// ---------------------------------------------------------------
//  loadSlotTotals / saveSlotTotals / incrementSlotTotal  (Fix 5)
//  Cumulative count of how many pads each slot has ever dispensed,
//  stored in NVS so they survive reboots and factory resets of
//  session data. Reset only via factory reset.
// ---------------------------------------------------------------
void loadSlotTotals() {
  settings.begin("config", true);
  totalA = settings.getInt("totA", 0);
  totalB = settings.getInt("totB", 0);
  totalC = settings.getInt("totC", 0);
  totalD = settings.getInt("totD", 0);
  settings.end();
}

void saveSlotTotals() {
  settings.begin("config", false);
  settings.putInt("totA", totalA);
  settings.putInt("totB", totalB);
  settings.putInt("totC", totalC);
  settings.putInt("totD", totalD);
  settings.end();
}

void incrementSlotTotal(char slot) {
  if      (slot == 'A') totalA++;
  else if (slot == 'B') totalB++;
  else if (slot == 'C') totalC++;
  else if (slot == 'D') totalD++;
  saveSlotTotals();
}

// ---------------------------------------------------------------
//  getTimestamp
//  Returns current time as "HH:MM". Falls back to "??:??" if NTP
//  has not synced yet (no internet on first boot).
// ---------------------------------------------------------------
String getTimestamp() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "??:??";
  char buf[10];
  strftime(buf, sizeof(buf), "%H:%M", &ti);
  return String(buf);
}

// ---------------------------------------------------------------
//  resetUserUsage
//  Clears only one user's usage counters in NVS ledger.
// ---------------------------------------------------------------
bool resetUserUsage(String id) {
  id.trim();
  if (id.length() == 0) return false;

  ledger.begin("usage", false);
  bool changed = false;

  changed |= ledger.remove((id + "_d").c_str());
  changed |= ledger.remove((id + "_dc").c_str());
  changed |= ledger.remove((id + "_m_date").c_str());
  changed |= ledger.remove((id + "_mc").c_str());

  ledger.end();
  return changed;
}

// ---------------------------------------------------------------
//  logActivity
//  Pushes a new record to the front of the in-memory history
//  ring buffer, then persists it.
// ---------------------------------------------------------------
void logActivity(String id, int d, int m) {
  Activity nowAct = { id, d, m, getTimestamp(), true };
  for (int i = HISTORY_SIZE - 1; i > 0; i--) {
    history[i] = history[i - 1];
  }
  history[0] = nowAct;
  saveHistory();
}

// ---------------------------------------------------------------
//  saveHistory  /  loadHistory
//  Each activity is one line: id,day,month,time
// ---------------------------------------------------------------
void saveHistory() {
  File f = SPIFFS.open("/history.txt", "w");
  if (!f) return;
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (history[i].active) {
      f.printf("%s,%d,%d,%s\n",
               history[i].id.c_str(),
               history[i].d,
               history[i].m,
               history[i].timeStr.c_str());
    }
  }
  f.close();
}

void loadHistory() {
  if (!SPIFFS.exists("/history.txt")) return;
  File f = SPIFFS.open("/history.txt", "r");
  if (!f) return;

  int i = 0;
  while (f.available() && i < HISTORY_SIZE) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    int c3 = line.lastIndexOf(',');

    if (c1 != -1 && c2 != -1 && c3 != -1) {
      history[i] = {
        line.substring(0, c1),
        line.substring(c1 + 1, c2).toInt(),
        line.substring(c2 + 1, c3).toInt(),
        line.substring(c3 + 1),
        true
      };
      i++;
    }
  }
  f.close();
}

// ---------------------------------------------------------------
//  clearFingerprintMap
//  Wipes all stored fingerprint-to-ID mappings during factory reset
// ---------------------------------------------------------------
void clearFingerprintMap() {
  Preferences fp;
  fp.begin("fingers", false);
  fp.clear();
  fp.end();
  Serial.println("Fingerprint map cleared.");
}
