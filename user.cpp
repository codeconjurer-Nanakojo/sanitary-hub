#include "user.h"
#include "hardware.h"
#include "storage.h"
#include "usage.h"
#include "cloud.h"
#include "fingerprint.h"
#include <Keypad.h>

// ---------------------------------------------------------------
//  Keypad definition
// ---------------------------------------------------------------
static const byte ROWS = KP_ROWS;
static const byte COLS = KP_COLS;

static char keys[ROWS][COLS] = {
  {'D', '#', '0', '*'},
  {'C', '9',  '8', '7'},
  {'B', '6',  '5', '4'},
  {'A', '3',  '2', '1'}
};

static byte rowPins[ROWS] = {13, 12, 14, 27};
static byte colPins[COLS] = {26, 25, 33, 32};

static Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------------------------------------------------------
//  initKeypad
//  Nothing extra needed for this library — placeholder for any
//  future hardware init (e.g. interrupt-based scanning).
// ---------------------------------------------------------------
void initKeypad() {
  // Keypad library handles pin modes internally
}

// ---------------------------------------------------------------
//  handleKeypad
//  Must be called on every loop() iteration.
//  Reads one keypress and routes it to the correct handler based
//  on the current MachineState.
// ---------------------------------------------------------------
void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  // Debounce: ignore presses faster than 250 ms
  if (millis() - lastKeyPressTime < 250) return;
  lastKeyPressTime = millis();

  // '*' = backspace in both states
  if (key == '*') {
    if (currentID.length() > 0) {
      currentID.remove(currentID.length() - 1);
      lcd.setCursor(0, 1);
      lcd.print("ID: " + currentID + "  ");
    }
    return;
  }

  switch (hubState) {
    case WAITING_ID:
      if (key == '#') {
        if (isEnrolling) {
          // In enrollment mode: transition from ID entry to fingerprint capture
          enrollmentStep = 1;
          lcd.clear();
          lcd.print("ID: " + currentID);
          lcd.setCursor(0, 1);
          lcd.print("Place Finger");
        } else {
          handleIDInput();
        }
      } else {
        currentID += key;
        lcd.setCursor(0, 0);
        lcd.print("Enter ID:       ");
        lcd.setCursor(0, 1);
        lcd.print("ID: " + currentID + "  ");
      }
      break;

    case WAITING_FINGER:
      // During fingerprint verification, keypad is idle
      // Fingerprint checking is done in checkFingerprint()
      break;

    case CHOOSING_PRODUCT:
      if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
        handleDispense(key);
      }
      break;
  }
}

// ---------------------------------------------------------------
//  handleIDInput
//  Triggered when the user presses '#' to confirm their ID.
//  Checks for stored fingerprint and transitions to either
//  WAITING_FINGER (for verification) or displays enrollment prompt
//  for new users.
// ---------------------------------------------------------------
void handleIDInput() {
  lcdMessage("Verifying...");
  currentID.trim();

  if (currentID.length() == 0) {
    lcdMessage("No ID entered", "Try again");
    delay(2000);
    lcdReady();
    return;
  }

  if (verifyUser(currentID)) {
    // User ID is valid — check for fingerprint
    validatedFingerID = getStoredFingerprintID(currentID);

    if (validatedFingerID == -1) {
      // New user — no fingerprint yet
      lcdMessage("ID OK. New User", "Enrolling...");
      delay(1500);
      // Don't auto-start enrollment; wait for web trigger
      lcdReady();
      currentID = "";
    } else {
      // Existing user — require fingerprint verification
      hubState = WAITING_FINGER;
      fingerTries = 0;
      lcdMessage("ID Verified", "Place Finger");
    }
  } else {
    lcdMessage("Access Denied", "Check your ID");
    delay(2000);
    lcdReady();
    currentID = "";
  }
}

// ---------------------------------------------------------------
//  handleDispense
//  Called when the user selects a product channel (A–D).
// ---------------------------------------------------------------
void handleDispense(char choice) {
  int dUse = 0, mUse = 0;   // FIX: always initialised

  // Pointer to the correct stock counter
  int* currentStock = (choice == 'A') ? &stA :
                      (choice == 'B') ? &stB :
                      (choice == 'C') ? &stC : &stD;

  // --- Stock check ---
  if (*currentStock <= 0) {
    lcdMessage("Out of Stock!", "Choose another");
    delay(2000);
    lcdMessage("ID Verified!", "Select: A B C D");
    return;
  }

  // --- Low-stock warning (FIX: new feature) ---
  if (*currentStock <= LOW_STOCK_THRESHOLD) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dispensing " + String(choice));
    lcd.setCursor(0, 1);
    lcd.print("Low stock! " + String(*currentStock - 1));
    delay(1000);
  } else {
    lcdMessage(("Dispensing " + String(choice)).c_str());
  }

  // --- Usage limit check ---
  if (checkAndUpdateUsage(currentID, dUse, mUse)) {
    dispenseAction(choice);
    (*currentStock)--;
    saveStock();
    incrementSlotTotal(choice);      // Fix 5: update cumulative counter
    logActivity(currentID, dUse, mUse);
    sendToSupabase(currentID, dUse, mUse);

    lcd.setCursor(0, 1);
    lcd.print("Thank You!      ");
    delay(2000);
  } else {
    lcdMessage("Limit Reached!", "Come back later");
    delay(2000);
  }

  // Return to idle
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter ID:");
  currentID = "";
  hubState  = WAITING_ID;
}

// ---------------------------------------------------------------
//  checkIdleTimeout  (NEW)
//  If the machine has been in CHOOSING_PRODUCT for too long
//  without a keypress, it resets back to WAITING_ID so nobody
//  else can use a previous user's verified session.
// ---------------------------------------------------------------
void checkIdleTimeout() {
  if (hubState == CHOOSING_PRODUCT) {
    if (millis() - productSelectTime > IDLE_TIMEOUT_MS) {
      lcdMessage("Session Timeout", "Enter ID:");
      currentID = "";
      hubState  = WAITING_ID;
      delay(1500);
      lcdReady();
    }
  }
}
