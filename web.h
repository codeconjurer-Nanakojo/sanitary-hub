#pragma once
#include "config.h"

void initWebServer();
void handleRoot();
void handleUpdateSettings();
void handleChangePassword();    // Fix 1: dedicated password change route
void handleFactoryReset();
void handleRemoteReset();
void handleExecuteReset();
void handleFileUpload();
