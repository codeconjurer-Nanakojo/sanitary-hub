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

1. Enter current admin password in the enrollment form.
2. Click `Start Fingerprint Enrollment`.
3. On keypad, enter Student ID and press `#`.
4. Place finger, remove, place same finger again.
5. Success message confirms slot save.

### Admin debug controls

- `Admin Debug Dispense` allows manual dispense from Slot `A-D` for testing.
- `Unjam / Reset Coil` allows manual reset of Channel `A-D`.
- Both actions require current admin password.

### Device-specific settings behavior

- Settings are local to each ESP32 board (stored in that board's NVS).
- Updating `Entity Name` or `Location` changes only the currently connected unit.
- In AP mode, devices may share `192.168.4.1`, but each AP is a separate network.

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
