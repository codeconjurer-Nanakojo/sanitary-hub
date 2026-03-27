#pragma once
#include "config.h"

void initWebServer();
void handleRoot();
void handleUpdateSettings();
void handleChangePassword();    // Fix 1: dedicated password change route
void handleFactoryReset();
void handleRemoteReset();
void handleExecuteReset();
void handleExecuteDispense();
void handleFileUpload();
void handleRemoteEnroll();      // Fingerprint enrollment trigger
void handleDebugFile();         // Debug file preview
