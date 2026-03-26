#include "cloud.h"
#include <HTTPClient.h>

// ---------------------------------------------------------------
//  sendToSupabase
//  POSTs a dispense event to the configured Supabase REST endpoint.
//  Silently skips if WiFi is not connected or URL is empty.
//
//  Payload fields:
//    student_ref   - the user's ID string
//    daily_usage   - how many times dispensed today
//    monthly_usage - how many times dispensed this month
//    stock_a/b/c/d - current stock levels (NEW: for trend tracking)
// ---------------------------------------------------------------
void sendToSupabase(String id, int d, int m) {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = String(SUPABASE_URL);
  if (url.length() == 0) return;   // Not configured yet

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type",  "application/json");
  http.addHeader("apikey",        SUPABASE_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("Prefer",        "return=minimal");

  // Include stock levels so remote dashboard can track depletion
  String json = "{";
  json += "\"student_ref\":\""   + id          + "\",";
  json += "\"daily_usage\":"     + String(d)   + ",";
  json += "\"monthly_usage\":"   + String(m)   + ",";
  json += "\"stock_a\":"         + String(stA) + ",";
  json += "\"stock_b\":"         + String(stB) + ",";
  json += "\"stock_c\":"         + String(stC) + ",";
  json += "\"stock_d\":"         + String(stD);
  json += "}";

  int code = http.POST(json);
  cloudLog = (code == 201 || code == 200)
             ? "Cloud: OK " + String(code)
             : "Cloud: Err " + String(code);

  http.end();
}
