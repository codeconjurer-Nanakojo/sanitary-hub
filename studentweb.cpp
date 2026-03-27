#include "studentweb.h"
#include "hardware.h"
#include "usage.h"
#include "storage.h"
#include "cloud.h"

// ---------------------------------------------------------------
//  initStudentWeb  —  Register student-facing routes
//  Call this from setup() AFTER initWebServer()
// ---------------------------------------------------------------
void initStudentWeb() {
  server.on("/dispense", HTTP_GET,  handleStudentDispensePage);
  server.on("/dispense", HTTP_POST, handleStudentDispensePost);
  server.on("/qr",       HTTP_GET,  handleQRPage);
}

// ---------------------------------------------------------------
//  Shared inline CSS — warm-pink, mobile-first, lightweight
// ---------------------------------------------------------------
static const char STUDENT_CSS[] PROGMEM = R"RAW(
  @import url('https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;600;700&display=swap');
  *{box-sizing:border-box;margin:0;padding:0}
  body{
    font-family:'DM Sans',sans-serif;
    background:#fff0f5;
    min-height:100vh;
    display:flex;
    align-items:center;
    justify-content:center;
    padding:20px;
  }
  .card{
    background:#fff;
    border-radius:28px;
    padding:2rem 1.8rem;
    max-width:420px;
    width:100%;
    box-shadow:0 20px 60px rgba(236,72,153,0.12);
    border:1px solid rgba(236,72,153,0.12);
  }
  .logo{font-size:2.2rem;text-align:center;margin-bottom:6px}
  h1{
    text-align:center;
    font-size:1.35rem;
    font-weight:700;
    color:#9d174d;
    margin-bottom:4px;
  }
  .subtitle{
    text-align:center;
    font-size:0.82rem;
    color:#be185d;
    opacity:0.7;
    margin-bottom:1.6rem;
  }
  label{
    display:block;
    font-size:0.8rem;
    font-weight:700;
    color:#9d174d;
    text-transform:uppercase;
    letter-spacing:0.05em;
    margin-bottom:6px;
  }
  .field{margin-bottom:1.2rem}
  input[type=text]{
    width:100%;
    padding:14px 16px;
    border:2px solid #fce7f3;
    border-radius:14px;
    font-size:1.1rem;
    color:#4a0e1c;
    font-family:inherit;
    background:#fff5f7;
    transition:border-color 0.2s,box-shadow 0.2s;
  }
  input[type=text]:focus{
    outline:none;
    border-color:#ec4899;
    background:#fff;
    box-shadow:0 0 0 4px rgba(236,72,153,0.08);
  }
  .slot-grid{
    display:grid;
    grid-template-columns:1fr 1fr;
    gap:10px;
    margin-bottom:1.4rem;
  }
  .slot-btn{
    position:relative;
    padding:18px 10px;
    border:2px solid #fce7f3;
    border-radius:16px;
    background:#fff5f7;
    cursor:pointer;
    text-align:center;
    transition:all 0.18s;
  }
  .slot-btn input[type=radio]{
    position:absolute;
    opacity:0;
    width:0;
    height:0;
  }
  .slot-btn .lbl{
    display:block;
    font-size:1.6rem;
    font-weight:800;
    color:#be185d;
    line-height:1;
  }
  .slot-btn .sub{
    display:block;
    font-size:0.72rem;
    color:#9d174d;
    margin-top:4px;
    opacity:0.7;
  }
  .slot-btn:has(input:checked){
    border-color:#ec4899;
    background:#fce7f3;
    box-shadow:0 0 0 3px rgba(236,72,153,0.15);
  }
  .slot-btn.empty{
    opacity:0.38;
    cursor:not-allowed;
    pointer-events:none;
  }
  .submit-btn{
    width:100%;
    padding:16px;
    background:linear-gradient(135deg,#ec4899,#db2777);
    color:#fff;
    border:none;
    border-radius:16px;
    font-size:1.05rem;
    font-weight:700;
    font-family:inherit;
    cursor:pointer;
    transition:opacity 0.2s,transform 0.15s;
    letter-spacing:0.02em;
  }
  .submit-btn:hover{opacity:0.92;transform:translateY(-1px)}
  .submit-btn:active{transform:translateY(0)}
  .msg{
    text-align:center;
    font-size:1rem;
    margin:1rem 0 0.5rem;
    font-weight:600;
  }
  .msg.ok{color:#0f7b3a}
  .msg.err{color:#e11d48}
  .back{
    display:block;
    text-align:center;
    margin-top:1.2rem;
    color:#ec4899;
    font-size:0.85rem;
    text-decoration:none;
    font-weight:600;
  }
  .back:hover{text-decoration:underline}
  .stock-dot{
    display:inline-block;
    width:8px;height:8px;
    border-radius:50%;
    margin-right:4px;
    vertical-align:middle;
  }
  .dot-ok{background:#22c55e}
  .dot-low{background:#f59e0b}
  .dot-empty{background:#e11d48}
)RAW";

// ---------------------------------------------------------------
//  Helper: stock indicator dot
// ---------------------------------------------------------------
static String stockDot(int qty) {
  if (qty <= 0)                    return "<span class='stock-dot dot-empty'></span>Empty";
  if (qty <= LOW_STOCK_THRESHOLD)  return "<span class='stock-dot dot-low'></span>" + String(qty) + " left";
  return "<span class='stock-dot dot-ok'></span>Available";
}

// ---------------------------------------------------------------
//  GET /dispense  —  Student-facing dispense form
// ---------------------------------------------------------------
void handleStudentDispensePage() {
  String html = F("<!DOCTYPE html><html lang='en'><head>");
  html += F("<meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
  html += "<title>" + entityName + " — Get a Pad</title>";
  html += "<style>" + String(STUDENT_CSS) + "</style>";
  html += F("</head><body><div class='card'>");
  html += F("<div class='logo'>🌸</div>");
  html += "<h1>" + entityName + "</h1>";
  html += "<p class='subtitle'>" + locationName + " &mdash; Free Pad Dispenser</p>";

  html += F("<form action='/dispense' method='POST'>");

  // Student ID field
  html += F("<div class='field'>");
  html += F("<label>Your Student ID</label>");
  html += F("<input type='text' name='sid' placeholder='e.g. STU042' required autocomplete='off' autocapitalize='characters'>");
  html += F("</div>");

  // Slot selection
  html += F("<label style='margin-bottom:10px'>Choose a Slot</label>");
  html += F("<div class='slot-grid'>");

  int stocks[4] = { stA, stB, stC, stD };
  for (int i = 0; i < 4; i++) {
    char lbl  = 'A' + i;
    bool empty = (stocks[i] <= 0);
    html += "<label class='slot-btn" + String(empty ? " empty" : "") + "'>";
    html += "<input type='radio' name='slot' value='" + String(lbl) + "'" +
            String(i == 0 && !empty ? " checked" : "") +
            String(empty ? " disabled" : "") + ">";
    html += "<span class='lbl'>Slot " + String(lbl) + "</span>";
    html += "<span class='sub'>" + stockDot(stocks[i]) + "</span>";
    html += F("</label>");
  }
  html += F("</div>");

  html += F("<button type='submit' class='submit-btn'>🩸 Dispense Pad</button>");
  html += F("</form>");
  html += F("</div></body></html>");

  server.send(200, "text/html", html);
}

// ---------------------------------------------------------------
//  POST /dispense  —  Verify student, enforce limits, dispense
// ---------------------------------------------------------------
void handleStudentDispensePost() {
  // --- Validate inputs ---
  if (!server.hasArg("sid") || !server.hasArg("slot")) {
    server.send(400, "text/html", "<p>Missing fields. <a href='/dispense'>Try again</a></p>");
    return;
  }

  String sid  = server.arg("sid");
  String slot = server.arg("slot");
  sid.trim();
  slot.trim();
  slot.toUpperCase();

  if (sid.length() == 0 || slot.length() != 1 || slot[0] < 'A' || slot[0] > 'D') {
    server.send(400, "text/html", "<p>Invalid input. <a href='/dispense'>Try again</a></p>");
    return;
  }

  // --- Build result page helper ---
  auto resultPage = [&](bool ok, const String& msg) {
    String html = F("<!DOCTYPE html><html lang='en'><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    html += "<title>" + entityName + " — Result</title>";
    html += "<style>" + String(STUDENT_CSS) + "</style>";
    html += F("</head><body><div class='card'>");
    html += F("<div class='logo'>");
    html += ok ? "✅" : "❌";
    html += F("</div>");
    html += "<h1>" + entityName + "</h1>";
    html += "<p class='subtitle'>" + locationName + "</p>";
    html += "<p class='msg " + String(ok ? "ok" : "err") + "'>" + msg + "</p>";
    html += F("<a class='back' href='/dispense'>← Try Again</a>");
    html += F("</div></body></html>");
    server.send(ok ? 200 : 403, "text/html", html);
  };

  // --- 1. Verify student ID ---
  if (!verifyUser(sid)) {
    resultPage(false, "❌ Student ID not recognised.<br>Please check and try again.");
    return;
  }

  // --- 2. Check stock for chosen slot ---
  int* stockPtr = nullptr;
  switch (slot[0]) {
    case 'A': stockPtr = &stA; break;
    case 'B': stockPtr = &stB; break;
    case 'C': stockPtr = &stC; break;
    case 'D': stockPtr = &stD; break;
  }
  if (*stockPtr <= 0) {
    resultPage(false, "Slot " + slot + " is empty.<br>Please choose another slot.");
    return;
  }

  // --- 3. Enforce daily/monthly limits ---
  int dUse = 0;
  int mUse = 0;
  if (!checkAndUpdateUsage(sid, dUse, mUse)) {
    resultPage(false, "Usage limit reached.<br>Come back later. 💛");
    return;
  }

  // --- 4. Dispense ---
  dispenseAction(slot[0]);
  (*stockPtr)--;
  saveStock();

  // --- 5. Update cumulative totals ---
  switch (slot[0]) {
    case 'A': totalA++; break;
    case 'B': totalB++; break;
    case 'C': totalC++; break;
    case 'D': totalD++; break;
  }
  saveSlotTotals();

  // --- 6. Log to history + cloud ---
  logActivity(sid, dUse, mUse);
  sendToSupabase(sid, dUse, mUse);

  // --- 7. LCD feedback ---
  String line2 = "Slot " + slot + " dispensed";
  lcdMessage("Dispensing...", line2.c_str());

  // --- 8. Low-stock LCD warning ---
  if (*stockPtr <= LOW_STOCK_THRESHOLD && *stockPtr > 0) {
    delay(2000);
    String lowStockLine = "Slot " + slot + ": " + String(*stockPtr) + " left";
    lcdMessage("Low Stock!", lowStockLine.c_str());
  }

  resultPage(true, "🌸 Pad dispensed from Slot " + slot + "!<br><span style='font-size:0.85rem;font-weight:400;color:#6b7280'>Take your pad from the tray.</span>");
}

// ---------------------------------------------------------------
//  GET /qr  —  Printable QR code page (admin opens, prints)
//
//  Uses the qrcodejs library (CDN) to generate the QR entirely
//  in the browser — zero flash storage cost on the ESP32.
//  The QR encodes: http://192.168.4.1/dispense
// ---------------------------------------------------------------
void handleQRPage() {
  // The AP IP is always 192.168.4.1 on ESP32
  const String dispenseURL = "http://192.168.4.1/dispense";

  String html = F("<!DOCTYPE html><html lang='en'><head>");
  html += F("<meta charset='UTF-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
  html += "<title>" + entityName + " — QR Code</title>";
  html += F("<script src='https://cdnjs.cloudflare.com/ajax/libs/qrcodejs/1.0.0/qrcode.min.js'></script>");
  html += F("<style>");
  html += F("@import url('https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;600;700;800&display=swap');");
  html += F("*{box-sizing:border-box;margin:0;padding:0}");
  html += F("body{font-family:'DM Sans',sans-serif;background:#fff0f5;min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center;padding:24px;gap:20px}");
  html += F(".poster{background:#fff;border-radius:32px;padding:2.4rem 2rem;max-width:380px;width:100%;box-shadow:0 24px 60px rgba(236,72,153,0.14);border:1px solid rgba(236,72,153,0.12);text-align:center}");
  html += F(".flower{font-size:2.8rem;margin-bottom:8px}");
  html += F("h1{font-size:1.5rem;font-weight:800;color:#9d174d;margin-bottom:4px}");
  html += F(".loc{font-size:0.85rem;color:#be185d;opacity:0.7;margin-bottom:1.6rem}");
  html += F("#qrcode{display:flex;justify-content:center;margin-bottom:1.4rem}");
  html += F("#qrcode img,#qrcode canvas{border-radius:16px;padding:12px;background:#fff5f7;border:2px solid #fce7f3}");
  html += F(".step{display:flex;align-items:flex-start;gap:10px;text-align:left;margin-bottom:10px}");
  html += F(".step-num{background:#ec4899;color:#fff;border-radius:50%;width:24px;height:24px;min-width:24px;display:flex;align-items:center;justify-content:center;font-size:0.8rem;font-weight:800;margin-top:1px}");
  html += F(".step-txt{font-size:0.88rem;color:#4a0e1c;line-height:1.4}");
  html += F(".step-txt strong{color:#9d174d}");
  html += F(".ssid-box{background:#fce7f3;border-radius:12px;padding:10px 14px;margin:1rem 0;font-size:0.9rem;color:#9d174d;font-weight:700;letter-spacing:0.04em}");
  html += F(".url-hint{font-size:0.72rem;color:#be185d;opacity:0.55;margin-top:0.8rem;word-break:break-all}");
  html += F(".print-btn{background:linear-gradient(135deg,#ec4899,#db2777);color:#fff;border:none;border-radius:14px;padding:13px 32px;font-size:1rem;font-weight:700;font-family:inherit;cursor:pointer;margin-top:4px;transition:opacity 0.2s}");
  html += F(".print-btn:hover{opacity:0.88}");
  html += F("@media print{.print-btn{display:none}body{background:#fff;padding:0}.poster{box-shadow:none;border:none}}");
  html += F("</style></head><body>");

  html += F("<div class='poster'>");
  html += F("<div class='flower'>🌸</div>");
  html += "<h1>" + entityName + "</h1>";
  html += "<p class='loc'>" + locationName + "</p>";

  // QR code rendered by JS
  html += F("<div id='qrcode'></div>");

  // Instructions
  html += F("<div class='step'><div class='step-num'>1</div><div class='step-txt'>Connect your phone to the WiFi network:</div></div>");
  html += "<div class='ssid-box'>📶 " + String(AP_SSID) + "</div>";  
  html += F("<div class='step'><div class='step-num'>2</div><div class='step-txt'>Scan the QR code above or open your browser and go to <strong>192.168.4.1/dispense</strong></div></div>");
  html += F("<div class='step'><div class='step-num'>3</div><div class='step-txt'>Enter your <strong>Student ID</strong>, choose a slot, and get your pad.</div></div>");
  html += "<p class='url-hint'>" + dispenseURL + "</p>";
  html += F("</div>"); // end .poster

  html += F("<button class='print-btn' onclick='window.print()'>🖨️ Print this page</button>");

  // QR generation script
  html += F("<script>");
  html += "new QRCode(document.getElementById('qrcode'),{text:'" + dispenseURL + "',width:220,height:220,colorDark:'#9d174d',colorLight:'#fff5f7',correctLevel:QRCode.CorrectLevel.H});";
  html += F("</script>");
  html += F("</body></html>");

  server.send(200, "text/html", html);
}
