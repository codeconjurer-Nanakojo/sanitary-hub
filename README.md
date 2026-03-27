# sanitary-hub

ESP32-based smart sanitary pad vending machine firmware with keypad + fingerprint verification, admin dashboard, QR student access, usage limits, stock tracking, and Supabase logging.

## Features

- Modular architecture (`hardware`, `storage`, `usage`, `user`, `fingerprint`, `web`, `studentweb`, `cloud`, `ota`)
- Dual access flow:
	- Keypad flow on device
	- QR/web flow from phone (`/dispense`)
- Fingerprint enrollment from admin dashboard (`POST /remoteEnroll`) with admin password validation
- Admin quick actions are password-protected (`POST /executeReset`, `POST /executeDispense`)
- Admin debug dispense tool for manual slot testing (A/B/C/D)
- Fingerprint verification required before dispense
	- Keypad flow: ID -> fingerprint -> slot selection
	- QR/web flow: ID + slot from phone -> fingerprint on machine -> dispense
- Admin-configurable `entityName` and `locationName` via dashboard settings
- Daily/monthly usage limits
- Per-slot stock and cumulative slot totals
- CSV student register upload and filtering
- Supabase event sync

## Project Layout

- `SanitaryHub/SanitaryHub.ino`: app entry point (`setup`, `loop`, globals)
- `SanitaryHub/modules_bridge.cpp`: compile bridge so Arduino includes parent module `.cpp` files
- `config.h`: global config/constants/externs
- `hardware.*`: LCD and coil/servo actions
- `storage.*`: SPIFFS + NVS persistence
- `usage.*`: student verification and limit counters
- `user.*`: keypad state machine
- `fingerprint.*`: enrollment + fingerprint verification
- `web.*`: admin routes/dashboard
- `studentweb.*`: student `/dispense` and `/qr` routes
- `cloud.*`: Supabase integration
- `ota.*`: OTA update handler

## Dashboard Settings

`POST /updateSettings` supports and persists:

- `en` -> entity name
- `loc` -> location
- `ssid`, `pw` -> station WiFi credentials
- `dl`, `ml` -> daily/monthly limits
- `stA`, `stB`, `stC`, `stD` -> slot stock levels

## Enrollment And Dispense Flow

1. Admin enters current password and clicks `Start Fingerprint Enrollment` in dashboard.
2. User enters Student ID on keypad and presses `#`.
3. User scans same finger twice.
4. Mapping `studentID -> fingerprintSlot` is saved in NVS (`fingers`).

QR/web dispense flow:

1. Phone opens `/dispense` (usually from `/qr`).
2. User submits Student ID + slot.
3. Firmware verifies user and usage limits.
4. Firmware requires fingerprint match on machine (`verifyFingerprintForID`).
5. On match, machine dispenses and logs usage/history/cloud.

## Notes

- If a student has no enrolled fingerprint, web dispense is rejected until enrollment is completed.
- Admin manual reset and admin debug dispense both require current admin password.
- Each ESP32 stores its own settings (`entityName`, `locationName`, limits, password) in its own flash/NVS.
- Dashboard changes apply only to the unit whose AP/web server you are currently connected to.
- Multiple devices can all use `192.168.4.1` in AP mode because each one is on its own hotspot network.
- The warning about `LiquidCrystal_I2C` architecture can be non-fatal, but use an ESP32-compatible library build if LCD issues appear.
