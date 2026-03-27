#pragma once
#include "config.h"

// =============================================================
//  Fingerprint Enrollment & Verification Module
// =============================================================

// Public enrollment functions
void initFingerprint();
void startRemoteEnrollment();
void enrollmentSequence();
void finishEnrollment(bool success);

// Public verification function
void checkFingerprint();

// Helper: retrieve fingerprint ID for a given student ID
int getStoredFingerprintID(String studentID);

// Helper: save mapping between student ID and fingerprint slot
void saveFingerprint(String studentID, int fingerprintSlot);
