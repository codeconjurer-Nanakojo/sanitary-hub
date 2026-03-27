# Sanitary Hub Instruction Manual

## Overview

This firmware runs the ESP32-based sanitary pad dispenser with keypad and web access, backed by fingerprint verification.

## Admin Operations

### Open dashboard

- Connect to the device AP.
- Open `http://192.168.4.1/`.

### Update identity and location

Use the System Configuration form on the dashboard:

- `Entity Name` updates `entityName` (`en`).
- `Location` updates `locationName` (`loc`).

These values are persisted in NVS and reused after reboot.

### Start fingerprint enrollment

1. Click `Start Fingerprint Enrollment` on dashboard.
2. On keypad, enter Student ID and press `#`.
3. Place finger, remove, place same finger again.
4. Success message confirms slot save.

## Student Dispense Paths

### Keypad path

1. Enter Student ID on keypad.
2. Place enrolled finger when prompted.
3. Select slot `A-D`.

### QR/Web path

1. Student opens `/dispense` from QR page.
2. Student enters ID and chooses slot.
3. Device asks for fingerprint scan on the machine.
4. Dispense occurs only after fingerprint match.

## Safety Checks In Code

- ID must exist in uploaded register.
- Daily and monthly limits are enforced.
- Slot stock must be available.
- Fingerprint must be enrolled and matched.
- Activity and cloud logs are updated after successful dispense.

## Troubleshooting

- Enrollment timeout: restart enrollment from dashboard.
- Fingerprint mismatch/timeout: retry with correct finger.
- Empty slot: refill stock and update stock values in dashboard.
- WiFi warning for library mismatch: verify ESP32-compatible LCD library if display is unstable.
