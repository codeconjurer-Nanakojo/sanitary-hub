#include "usage.h"

// ---------------------------------------------------------------
//  verifyUser
//  Opens /master.csv (one ID per line) and checks for a match.
//  The file is populated by the admin via the web CSV upload.
// ---------------------------------------------------------------
bool verifyUser(String id) {
  id.trim();
  File file = SPIFFS.open("/master.csv", "r");
  if (!file) return false;

  while (file.available()) {
    String ref = file.readStringUntil('\n');
    ref.trim();
    if (ref == id) {
      file.close();
      return true;
    }
  }
  file.close();
  return false;
}

// ---------------------------------------------------------------
//  checkAndUpdateUsage
//  Reads the NVS ledger for this user ID and checks whether they
//  are within the configured daily and monthly limits.
//
//  If within limits:  increments both counters, writes back,
//                     sets dOut/mOut, returns true.
//  If over limit:     leaves counters unchanged, returns false.
//
//  FIX APPLIED: dOut and mOut are always initialised to 0 before
//  use so they are never left in an undefined state.
// ---------------------------------------------------------------
bool checkAndUpdateUsage(String id, int &dOut, int &mOut) {
  // Initialise outputs to safe values (bug fix)
  dOut = 0;
  mOut = 0;

  ledger.begin("usage", false);

  struct tm ti;
  bool hasTime = getLocalTime(&ti);
  int curD = hasTime ? ti.tm_mday       : 1;
  int curM = hasTime ? (ti.tm_mon + 1)  : 1;

  int lastD = ledger.getInt((id + "_d").c_str(),      -1);
  int lastM = ledger.getInt((id + "_m_date").c_str(), -1);

  // Reset daily counter if it's a new day
  int dC = (lastD == curD) ? ledger.getInt((id + "_dc").c_str(), 0) : 0;
  // Reset monthly counter if it's a new month
  int mC = (lastM == curM) ? ledger.getInt((id + "_mc").c_str(), 0) : 0;

  if (dC >= dailyLimit || mC >= monthlyLimit) {
    ledger.end();
    return false;
  }

  dC++;
  mC++;

  ledger.putInt((id + "_d").c_str(),      curD);
  ledger.putInt((id + "_dc").c_str(),     dC);
  ledger.putInt((id + "_m_date").c_str(), curM);
  ledger.putInt((id + "_mc").c_str(),     mC);

  dOut = dC;
  mOut = mC;

  ledger.end();
  return true;
}
