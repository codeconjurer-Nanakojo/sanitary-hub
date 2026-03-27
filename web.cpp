#include "web.h"
#include "hardware.h"
#include "storage.h"

// ---------------------------------------------------------------
//  initWebServer
// ---------------------------------------------------------------
void initWebServer() {
  server.on("/",                HTTP_GET,  handleRoot);
  server.on("/updateSettings",  HTTP_POST, handleUpdateSettings);
  server.on("/changePassword",  HTTP_POST, handleChangePassword);  // Fix 1
  server.on("/factoryReset",    HTTP_POST, handleFactoryReset);
  server.on("/executeReset",    HTTP_POST, handleExecuteReset);
  server.on("/remoteReset",     HTTP_GET,  handleRemoteReset);
  server.on("/upload", HTTP_POST,
    []() { server.send(200, "text/html", "<h3>Uploaded!</h3><a href='/'>Back</a>"); },
    handleFileUpload
  );
  server.begin();
}

// ---------------------------------------------------------------
//  handleRoot  —  Admin Dashboard
// ---------------------------------------------------------------
void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>" + entityName + " Hub | Admin</title>";
  html += "<style>";
  html += "body{font-family:'Inter',sans-serif;background:#fff5f7;padding:20px;color:#4a0e1c;line-height:1.6}";
  html += ".dashboard{max-width:1000px;margin:0 auto}";
  html += ".card{background:#fff;border-radius:24px;padding:1.8rem;box-shadow:0 10px 25px rgba(236,72,153,0.08);margin-bottom:24px;border:1px solid rgba(236,72,153,0.1)}";
  html += "h2{font-size:1.8rem;font-weight:800;color:#9d174d;margin-bottom:0.5rem}";
  html += "h3{font-size:1.2rem;font-weight:600;color:#be185d;margin-bottom:1.2rem}";
  html += ".status-bar{display:flex;flex-wrap:wrap;gap:15px;margin-bottom:20px}";
  html += ".badge{padding:8px 16px;border-radius:50px;font-weight:600;font-size:0.9rem}";
  html += ".bg-blue{background:#fce7f3;color:#9d174d}";
  html += ".bg-warn{background:#fff3cd;color:#856404}";
  html += ".bg-empty{background:#ffe4e6;color:#be123c}";
  html += ".wifi-on{background:#e0f7ea;color:#0f7b3a} .wifi-off{background:#ffe6e5;color:#b13e3e}";
  html += ".btn{display:inline-block;text-decoration:none;background:#ec4899;color:white;padding:12px 24px;border-radius:12px;font-weight:600;border:none;cursor:pointer;transition:0.3s;font-size:0.9rem}";
  html += ".btn:hover{background:#db2777;transform:translateY(-1px)}";
  html += ".btn-reset{background:#f472b6} .btn-reset:hover{background:#e879f9}";
  html += ".btn-danger{background:#e11d48;color:white;padding:10px 20px;border-radius:8px;border:none;cursor:pointer}";
  html += ".btn-sec{background:#7c3aed;color:white;padding:12px 24px;border-radius:12px;font-weight:600;border:none;cursor:pointer;transition:0.3s;font-size:0.9rem;width:100%}";
  html += ".btn-sec:hover{background:#6d28d9}";
  html += ".grid-btns{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px}";
  html += ".activity-table{width:100%;border-collapse:collapse}";
  html += "th{text-align:left;color:#be185d;font-size:0.8rem;text-transform:uppercase;padding:12px;border-bottom:2px solid #fff5f7}";
  html += "td{padding:12px;border-bottom:1px solid #fff5f7;font-size:0.95rem}";
  html += ".form-group{margin-bottom:15px} label{display:block;font-size:0.85rem;font-weight:600;margin-bottom:5px;color:#701a75}";
  html += "input[type='text'],input[type='password'],input[type='number']{width:100%;padding:10px;border-radius:8px;border:1px solid #fbcfe8;box-sizing:border-box}";
  html += "input:focus{outline:none;border-color:#ec4899;box-shadow:0 0 0 2px rgba(236,72,153,0.1)}";
  html += ".inline-inputs{display:flex;gap:10px;flex-wrap:wrap} .inline-inputs div{flex:1;min-width:80px}";
  // Slot totals bar chart style
  html += ".slot-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:16px;margin-top:8px}";
  html += ".slot-box{text-align:center;background:#fff5f7;border-radius:16px;padding:16px 8px}";
  html += ".slot-label{font-size:1.4rem;font-weight:800;color:#9d174d}";
  html += ".slot-total{font-size:2rem;font-weight:800;color:#ec4899;margin:4px 0}";
  html += ".slot-stock{font-size:0.8rem;color:#be185d;opacity:0.7}";
  html += ".bar-wrap{background:#fce7f3;border-radius:8px;height:8px;margin-top:8px}";
  html += ".bar-fill{background:#ec4899;height:8px;border-radius:8px;transition:width 0.3s}";
  html += ".first-boot-banner{background:#fef3c7;border:1px solid #f59e0b;border-radius:12px;padding:14px 18px;margin-bottom:20px;color:#92400e;font-weight:600}";
  html += "</style></head><body><div class='dashboard'>";

  // --- First-boot warning banner (Fix 1) ---
  if (adminPassword == DEFAULT_ADMIN_PASSWORD) {
    html += "<div class='first-boot-banner'>⚠️ You are using the <strong>default password</strong>. ";
    html += "Please change it below before deploying the machine.</div>";
  }

  // --- Header ---
  html += "<h2>🌸 " + entityName + " Hub &mdash; " + locationName + "</h2>";

  // --- Status Bar ---
  auto stockBadge = [](char lbl, int qty) -> String {
    String cls = (qty <= 0) ? "badge bg-empty" : (qty <= LOW_STOCK_THRESHOLD) ? "badge bg-warn" : "badge bg-blue";
    String val = (qty <= 0) ? "EMPTY" : String(qty);
    return "<div class='" + cls + "'>" + String(lbl) + ": " + val + "</div>";
  };

  html += "<div class='status-bar'>";
  html += stockBadge('A', stA);
  html += stockBadge('B', stB);
  html += stockBadge('C', stC);
  html += stockBadge('D', stD);
  html += "<div class='badge " + String(WiFi.status() == WL_CONNECTED ? "wifi-on" : "wifi-off") + "'>📡 ";
  html += (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "OFFLINE") + "</div>";
  html += "</div>";

  // ---------------------------------------------------------------
  //  Fix 5 — Per-slot dispense totals card
  // ---------------------------------------------------------------
  int grandTotal = totalA + totalB + totalC + totalD;
  int maxTotal   = max({totalA, totalB, totalC, totalD, 1});  // avoid div/0

  html += "<div class='card'><h3>📊 Dispense Totals</h3>";
  html += "<p style='font-size:0.85rem;color:#6b7280;margin-bottom:12px'>Cumulative pads dispensed per slot since last factory reset. Grand total: <strong>" + String(grandTotal) + "</strong></p>";
  html += "<div class='slot-grid'>";

  auto slotCard = [&](char lbl, int total, int stock) -> String {
    int barPct = (total * 100) / maxTotal;
    String s = "<div class='slot-box'>";
    s += "<div class='slot-label'>Slot " + String(lbl) + "</div>";
    s += "<div class='slot-total'>" + String(total) + "</div>";
    s += "<div class='slot-stock'>Stock left: " + String(stock) + "</div>";
    s += "<div class='bar-wrap'><div class='bar-fill' style='width:" + String(barPct) + "%'></div></div>";
    s += "</div>";
    return s;
  };

  html += slotCard('A', totalA, stA);
  html += slotCard('B', totalB, stB);
  html += slotCard('C', totalC, stC);
  html += slotCard('D', totalD, stD);
  html += "</div>";
  html += "<div style='margin-top:14px'>";
  html += "<a href='/qr' target='_blank' class='btn' style='display:block;text-align:center;text-decoration:none'>📱 Print Student QR Code</a>";
  html += "</div></div>";

  // --- Quick Actions (coil reset — POST forms) ---
  html += "<div class='card'><h3>⚡ Quick Actions</h3>";
  html += "<label style='margin-bottom:10px;display:block;'>Unjam / Reset Coils:</label>";
  html += "<div class='grid-btns'>";
  for (int i = 0; i < 4; i++) {
    char lbl = 'A' + i;
    html += "<form action='/executeReset' method='POST' style='margin:0'>";
    html += "<input type='hidden' name='ch' value='" + String(i) + "'>";
    html += "<input type='submit' class='btn btn-reset' value='Reset " + String(lbl) + "'>";
    html += "</form>";
  }
  html += "</div></div>";

  // --- Activity Log ---
  html += "<div class='card'><h3>📋 Recent Activity (Last " + String(HISTORY_SIZE) + ")</h3>";
  html += "<div style='overflow-x:auto'><table class='activity-table'>";
  html += "<tr><th>ID</th><th>Date</th><th>Time</th><th>Day</th><th>Month</th></tr>";
  bool anyActivity = false;
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (history[i].active) {
      anyActivity = true;
      html += "<tr><td>" + history[i].id + "</td>";
      html += "<td>" + String(history[i].d) + "/" + String(history[i].m) + "</td>";
      html += "<td>" + history[i].timeStr + "</td>";
      html += "<td>" + String(history[i].d) + "/" + String(dailyLimit) + "</td>";
      html += "<td>" + String(history[i].m) + "/" + String(monthlyLimit) + "</td></tr>";
    }
  }
  if (!anyActivity) {
    html += "<tr><td colspan='5' style='text-align:center;color:#be185d;opacity:0.5'>No activity yet</td></tr>";
  }
  html += "</table></div></div>";

  // --- System Settings ---
  html += "<div class='card'><h3>⚙️ System Configuration</h3>";
  html += "<form action='/updateSettings' method='POST'>";
  html += "<div class='form-group'><label>Current Admin Password (required to save)</label><input type='password' name='pass' required></div>";
  html += "<div class='inline-inputs'>";
  html += "<div class='form-group'><label>Entity Name</label><input type='text' name='en' value='" + entityName + "'></div>";
  html += "<div class='form-group'><label>Location</label><input type='text' name='loc' value='" + locationName + "'></div>";
  html += "</div>";
  html += "<div class='inline-inputs'>";
  html += "<div class='form-group'><label>WiFi SSID</label><input type='text' name='ssid' value='" + staSSID + "'></div>";
  html += "<div class='form-group'><label>WiFi Password</label><input type='password' name='pw' placeholder='Leave blank to keep current'></div>";
  html += "</div>";
  html += "<div class='inline-inputs'>";
  html += "<div class='form-group'><label>Daily Limit</label><input type='number' name='dl' value='" + String(dailyLimit) + "' min='1'></div>";
  html += "<div class='form-group'><label>Monthly Limit</label><input type='number' name='ml' value='" + String(monthlyLimit) + "' min='1'></div>";
  html += "</div>";
  html += "<label>Manual Stock Levels</label><div class='inline-inputs'>";
  html += "<div><label>A</label><input type='number' name='stA' value='" + String(stA) + "' min='0'></div>";
  html += "<div><label>B</label><input type='number' name='stB' value='" + String(stB) + "' min='0'></div>";
  html += "<div><label>C</label><input type='number' name='stC' value='" + String(stC) + "' min='0'></div>";
  html += "<div><label>D</label><input type='number' name='stD' value='" + String(stD) + "' min='0'></div>";
  html += "</div><br>";
  html += "<input type='submit' class='btn' style='width:100%' value='💾 Save Configuration'></form></div>";

  // --- Change Password Card (Fix 1) ---
  html += "<div class='card'><h3>🔐 Change Admin Password</h3>";
  html += "<form action='/changePassword' method='POST'>";
  html += "<div class='form-group'><label>Current Password</label><input type='password' name='oldpass' required></div>";
  html += "<div class='form-group'><label>New Password</label><input type='password' name='newpass' required minlength='6'></div>";
  html += "<div class='form-group'><label>Confirm New Password</label><input type='password' name='confirmpass' required minlength='6'></div>";
  html += "<input type='submit' class='btn-sec' value='🔐 Update Password'></form></div>";

  // --- Cloud Status ---
  html += "<div style='text-align:center;color:#9d174d;font-size:0.75rem;margin-bottom:10px;opacity:0.7'>" + cloudLog + "</div>";

  // --- File Upload ---
  html += "<div class='card'><h3>📂 Upload Student Register (CSV)</h3>";
  html += "<p style='font-size:0.85rem;color:#6b7280;margin-bottom:12px'>Upload a CSV where female students have <code>,F</code> at the end of their row.</p>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data' style='display:flex;flex-direction:column;gap:10px'>";
  html += "<input type='file' name='update' accept='.csv' style='padding:10px;border:1px dashed #ec4899;border-radius:8px;width:100%;color:#be185d;'>";
  html += "<input type='submit' class='btn' value='⬆️ Upload to Hub'></form></div>";

  // --- Danger Zone ---
  html += "<div class='card'><h3>⚠️ Danger Zone</h3>";
  html += "<form action='/factoryReset' method='POST' onsubmit='return confirm(\"This will wipe ALL data and reboot. Are you sure?\")'>";
  html += "<div class='form-group'><label>Confirm Admin Password</label><input type='password' name='pass' required></div>";
  html += "<input type='submit' class='btn btn-danger' style='width:100%' value='⚠️ Factory Reset — Wipe Everything'></form></div>";

  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

// ---------------------------------------------------------------
//  handleChangePassword  (Fix 1)
//  Validates old password, checks new passwords match, saves to NVS.
// ---------------------------------------------------------------
void handleChangePassword() {
  if (!server.hasArg("oldpass") || !server.hasArg("newpass") || !server.hasArg("confirmpass")) {
    server.send(400, "text/plain", "Missing fields");
    return;
  }

  String oldPass     = server.arg("oldpass");
  String newPass     = server.arg("newpass");
  String confirmPass = server.arg("confirmpass");

  if (oldPass != adminPassword) {
    server.send(403, "text/html", "<h3>❌ Current password is incorrect.</h3><a href='/'>Back</a>");
    return;
  }
  if (newPass != confirmPass) {
    server.send(400, "text/html", "<h3>❌ New passwords do not match.</h3><a href='/'>Back</a>");
    return;
  }
  if (newPass.length() < 6) {
    server.send(400, "text/html", "<h3>❌ Password must be at least 6 characters.</h3><a href='/'>Back</a>");
    return;
  }

  saveAdminPassword(newPass);
  server.send(200, "text/html", "<h3>✅ Password updated successfully.</h3><a href='/'>Back to Dashboard</a>");
}

// ---------------------------------------------------------------
//  handleRemoteReset  —  Shows confirmation page (GET)
// ---------------------------------------------------------------
void handleRemoteReset() {
  String html = "<!DOCTYPE html><html><body style='font-family:sans-serif;padding:20px'>";
  html += "<h3>🔧 Coil Reset</h3>";
  html += "<form action='/executeReset' method='POST'>";
  html += "<label>Select Channel: </label>";
  html += "<select name='ch'>";
  html += "<option value='0'>A</option><option value='1'>B</option>";
  html += "<option value='2'>C</option><option value='3'>D</option>";
  html += "</select><br><br>";
  html += "<input type='submit' value='Confirm Reset' style='padding:10px 20px;background:#f472b6;border:none;border-radius:8px;color:white;cursor:pointer'>";
  html += "</form><br><a href='/'>Cancel</a></body></html>";
  server.send(200, "text/html", html);
}

// ---------------------------------------------------------------
//  handleExecuteReset  —  Queues coil reset (POST only)
// ---------------------------------------------------------------
void handleExecuteReset() {
  if (!server.hasArg("ch")) {
    server.send(400, "text/plain", "Missing channel parameter");
    return;
  }
  int ch = server.arg("ch").toInt();
  if (ch < 0 || ch > 3) {
    server.send(400, "text/plain", "Invalid channel (must be 0-3)");
    return;
  }
  webResetChannel = ch;
  String html = "<h3>✅ Reset queued for Channel " + String((char)('A' + ch)) + "</h3>";
  html += "<a href='/'>Back to Dashboard</a>";
  server.send(200, "text/html", html);
}

// ---------------------------------------------------------------
//  handleUpdateSettings
//  FIX 1: now checks against adminPassword (NVS) not a #define
// ---------------------------------------------------------------
void handleUpdateSettings() {
  if (!server.hasArg("pass") || server.arg("pass") != adminPassword) {
    server.send(403, "text/plain", "Invalid password");
    return;
  }

  settings.begin("config", false);
  if (server.hasArg("en")  && server.arg("en").length()  > 0) { entityName   = server.arg("en");  settings.putString("en",  entityName);   }
  if (server.hasArg("loc") && server.arg("loc").length() > 0) { locationName = server.arg("loc"); settings.putString("loc", locationName);  }
  if (server.hasArg("dl"))  { dailyLimit   = server.arg("dl").toInt(); settings.putInt("dl",  dailyLimit);   }
  if (server.hasArg("ml"))  { monthlyLimit = server.arg("ml").toInt(); settings.putInt("ml",  monthlyLimit); }
  if (server.hasArg("ssid") && server.arg("ssid").length() > 0) {
    staSSID = server.arg("ssid"); settings.putString("ssid", staSSID);
  }
  if (server.hasArg("pw") && server.arg("pw").length() > 0) {
    staPASS = server.arg("pw"); settings.putString("pw", staPASS);
  }
  settings.end();

  if (server.hasArg("stA")) stA = server.arg("stA").toInt();
  if (server.hasArg("stB")) stB = server.arg("stB").toInt();
  if (server.hasArg("stC")) stC = server.arg("stC").toInt();
  if (server.hasArg("stD")) stD = server.arg("stD").toInt();
  saveStock();

  server.sendHeader("Location", "/");
  server.send(303);
}

// ---------------------------------------------------------------
//  handleFactoryReset
//  FIX 1: checks against adminPassword (NVS)
// ---------------------------------------------------------------
void handleFactoryReset() {
  if (!server.hasArg("pass") || server.arg("pass") != adminPassword) {
    server.send(403, "text/plain", "Invalid password");
    return;
  }
  SPIFFS.remove("/master.csv");
  SPIFFS.remove("/stock.txt");
  SPIFFS.remove("/history.txt");
  ledger.begin("usage",  false); ledger.clear();  ledger.end();
  settings.begin("config", false); settings.clear(); settings.end();
  server.send(200, "text/plain", "System wiped. Rebooting in 2 seconds...");
  delay(2000);
  ESP.restart();
}

// ---------------------------------------------------------------
//  handleFileUpload  —  Parses uploaded CSV to build master.csv
// ---------------------------------------------------------------
static File uploadFile;

void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("Upload started: " + String(upload.filename));
    uploadFile = SPIFFS.open("/temp.csv", "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      File rawFile   = SPIFFS.open("/temp.csv",  "r");
      File cleanFile = SPIFFS.open("/master.csv", "w");
      if (rawFile && cleanFile) {
        if (rawFile.available()) rawFile.readStringUntil('\n');  // Skip header
        int count = 0;
        while (rawFile.available()) {
          String line = rawFile.readStringUntil('\n');
          line.trim();
          if (!line.endsWith(",F")) continue;
          int fc = line.indexOf(',');
          int sc = line.indexOf(',', fc + 1);
          if (fc == -1 || sc == -1) continue;
          String ref = line.substring(fc + 1, sc);
          ref.trim();
          if (ref.length() > 0) { cleanFile.println(ref); count++; }
        }
        cleanFile.close();
        rawFile.close();
        Serial.printf("CSV processed: %d IDs added.\n", count);
      }
      SPIFFS.remove("/temp.csv");
    }
  }
}
