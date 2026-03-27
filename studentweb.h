#pragma once
#include "config.h"

void initStudentWeb();
void handleStudentDispensePage();   // GET  /dispense  — student-facing form
void handleStudentDispensePost();   // POST /dispense  — verify + dispense
void handleQRPage();                // GET  /qr        — printable QR code page
