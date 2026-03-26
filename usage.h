#pragma once
#include "config.h"

// Returns true and increments counters if the user is within
// their daily and monthly limits. dOut / mOut receive the new counts.
bool checkAndUpdateUsage(String id, int &dOut, int &mOut);

// Returns true if the given ID exists in /master.csv
bool verifyUser(String id);
