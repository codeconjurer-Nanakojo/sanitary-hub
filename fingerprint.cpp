#include "fingerprint.h"
#include "hardware.h"
#include "storage.h"
#include <Adafruit_Fingerprint.h>

HardwareSerial fingerSerial(2);
Adafruit_Fingerprint fingerprintSensor = Adafruit_Fingerprint(&fingerSerial);
Preferences fingerMap;

// ---------------------------------------------------------------
//  initFingerprint
//  Initialize the fingerprint sensor on HardwareSerial(2)
// ---------------------------------------------------------------
void initFingerprint() {
  fingerSerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  fingerprintSensor.begin(57600);
  if (!fingerprintSensor.verifyPassword()) {
    Serial.println("Fingerprint sensor init failed!");
    lcdMessage("Fingerprint Fail", "Check sensor");
    delay(2000);
  } else {
    Serial.println("Fingerprint sensor ready");
  }
}

// ---------------------------------------------------------------
//  startRemoteEnrollment
//  Web route /remoteEnroll triggers this
// ---------------------------------------------------------------
void startRemoteEnrollment() {
  isEnrolling      = true;
  enrollmentStep   = 0;  // Waiting for keypad ID input
  currentID        = "";
  enrollmentTimer  = millis();
  
  lcdMessage("Enrollment Mode", "Enter ID + #");
  Serial.println("Enrollment started from web");
}

// ---------------------------------------------------------------
//  getStoredFingerprintID
//  Look up fingerprint slot for a given student ID
// ---------------------------------------------------------------
int getStoredFingerprintID(String studentID) {
  fingerMap.begin("fingers", true);  // read-only
  int fpID = fingerMap.getInt(studentID.c_str(), -1);
  fingerMap.end();
  return fpID;
}

// ---------------------------------------------------------------
//  saveFingerprint
//  Store the mapping: studentID -> fingerprintSlot
// ---------------------------------------------------------------
void saveFingerprint(String studentID, int fingerprintSlot) {
  fingerMap.begin("fingers", false);  // read-write
  fingerMap.putInt(studentID.c_str(), fingerprintSlot);
  fingerMap.end();
  Serial.printf("Saved fingerprint: ID=%s, Slot=%d\n", studentID.c_str(), fingerprintSlot);
}

// ---------------------------------------------------------------
//  resetFingerprintForID
//  Removes studentID->fingerprint mapping and optionally deletes
//  the template from the fingerprint sensor database.
// ---------------------------------------------------------------
bool resetFingerprintForID(String studentID, bool removeTemplate) {
  studentID.trim();
  if (studentID.length() == 0) return false;

  int fpID = getStoredFingerprintID(studentID);
  if (fpID < 0) return false;

  fingerMap.begin("fingers", false);
  bool removed = fingerMap.remove(studentID.c_str());
  fingerMap.end();

  if (removed && removeTemplate) {
    int delResult = fingerprintSensor.deleteModel(fpID);
    if (delResult == FINGERPRINT_OK) {
      Serial.printf("Deleted fingerprint template slot %d for ID=%s\n", fpID, studentID.c_str());
    } else {
      Serial.printf("Template delete failed for slot %d (code %d)\n", fpID, delResult);
    }
  }

  return removed;
}

// ---------------------------------------------------------------
//  enrollmentSequence
//  Multi-stage fingerprint capture and enrollment.
//  Called every loop() when isEnrolling=true
//
//  Flow:
//    Step 0: Wait for keypad ID (handled in keypad handler)
//    Step 1: Capture first fingerprint image
//    Step 2: Wait for finger removal
//    Step 3: Capture second fingerprint image, create model, store
// ---------------------------------------------------------------
void enrollmentSequence() {
  // Step 0: waiting for keypad input, handled elsewhere
  if (enrollmentStep == 0) return;

  int p = fingerprintSensor.getImage();

  // --- STEP 1: Capture First Image ---
  if (enrollmentStep == 1) {
    if (p == FINGERPRINT_OK) {
      if (fingerprintSensor.image2Tz(1) == FINGERPRINT_OK) {
        enrollmentStep = 2;
        enrollmentTimer = millis();
        lcdMessage("Remove Finger", "");
        Serial.println("Image 1 captured");
      }
    }
  }
  // --- STEP 2: Wait for Finger Removal ---
  else if (enrollmentStep == 2) {
    if (p == FINGERPRINT_NOFINGER) {
      enrollmentStep = 3;
      lcdMessage("Place Same", "Finger Again");
      Serial.println("Waiting for second press");
    }
  }
  // --- STEP 3: Capture Second Image & Create Model ---
  else if (enrollmentStep == 3) {
    if (p == FINGERPRINT_OK) {
      if (fingerprintSensor.image2Tz(2) == FINGERPRINT_OK) {
        if (fingerprintSensor.createModel() == FINGERPRINT_OK) {
          // Find next available slot
          fingerprintSensor.getTemplateCount();
          int nextID = fingerprintSensor.templateCount + 1;

          if (fingerprintSensor.storeModel(nextID) == FINGERPRINT_OK) {
            // Save mapping
            saveFingerprint(currentID, nextID);

            lcdMessage("Success!", ("Slot: " + String(nextID)).c_str());
            delay(2000);
            finishEnrollment(true);
          } else {
            lcdMessage("Store Error!", "");
            Serial.println("Failed to store model");
            finishEnrollment(false);
          }
        } else {
          lcdMessage("Model Error!", "");
          Serial.println("Failed to create model");
          finishEnrollment(false);
        }
      }
    }
  }
}

// ---------------------------------------------------------------
//  checkFingerprint
//  Verify fingerprint during login (WAITING_FINGER state).
//  Called from handleKeypad when hubState == WAITING_FINGER
// ---------------------------------------------------------------
void checkFingerprint() {
  int p = fingerprintSensor.getImage();
  if (p == FINGERPRINT_NOFINGER) return;

  p = fingerprintSensor.image2Tz();
  if (p != FINGERPRINT_OK) return;

  p = fingerprintSensor.fingerFastSearch();

  // Check if the matched fingerprint matches the user's stored ID
  if (p == FINGERPRINT_OK && fingerprintSensor.fingerID == validatedFingerID) {
    lcdMessage("Match! Select:", "A, B, C, or D");
    productSelectTime = millis();
    hubState = CHOOSING_PRODUCT;
    Serial.println("Fingerprint match!");
  } else {
    fingerTries++;
    if (fingerTries >= 3) {
      lcdMessage("3 Fails. Reset", "");
      delay(2000);
      currentID     = "";
      validatedFingerID = -1;
      hubState      = WAITING_ID;
      lcdReady();
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Retry: " + String(fingerTries) + "/3");
      delay(1000);
    }
  }
}

// ---------------------------------------------------------------
//  verifyFingerprintForID
//  Used by QR/web dispense to enforce a local fingerprint check
//  before dispensing. Returns true only on matching fingerprint.
// ---------------------------------------------------------------
bool verifyFingerprintForID(String studentID, unsigned long timeoutMs) {
  int expectedFingerID = getStoredFingerprintID(studentID);
  if (expectedFingerID < 0) {
    return false;
  }

  lcdMessage("Web verify", "Place finger");
  unsigned long startMs = millis();

  while (millis() - startMs < timeoutMs) {
    int p = fingerprintSensor.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      delay(25);
      yield();
      continue;
    }
    if (p != FINGERPRINT_OK) {
      delay(25);
      yield();
      continue;
    }

    p = fingerprintSensor.image2Tz();
    if (p != FINGERPRINT_OK) {
      delay(25);
      yield();
      continue;
    }

    p = fingerprintSensor.fingerFastSearch();
    if (p == FINGERPRINT_OK && fingerprintSensor.fingerID == expectedFingerID) {
      lcdMessage("Fingerprint OK", "Dispensing...");
      delay(500);
      return true;
    }

    lcdMessage("No match", "Try again");
    delay(800);
    lcdMessage("Web verify", "Place finger");
  }

  lcdMessage("Finger timeout", "Try again");
  return false;
}

// ---------------------------------------------------------------
//  finishEnrollment
//  Clean up after enrollment (success or failure)
// ---------------------------------------------------------------
void finishEnrollment(bool success) {
  isEnrolling       = false;
  enrollmentStep    = 0;
  currentID         = "";
  validatedFingerID = -1;
  fingerTries       = 0;
  hubState          = WAITING_ID;

  lcdReady();
  
  if (success) {
    Serial.println("Enrollment successful");
  } else {
    Serial.println("Enrollment failed");
  }
}
