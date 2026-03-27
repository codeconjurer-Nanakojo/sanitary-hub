#include "hardware.h"

// ---------------------------------------------------------------
//  initHardware
//  Called once from setup(). Initialises LCD, PCA9685, and stops
//  all coils so nothing spins on power-up.
// ---------------------------------------------------------------
void initHardware() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Loading..");

  Wire.begin();
  driver.resetDevices();
  driver.init();
  driver.setPWMFrequency(SERVO_FREQ);
  stopAllCoils();
}

// ---------------------------------------------------------------
//  stopAllCoils
//  Sends the neutral/stop pulse to all 4 PCA9685 channels.
// ---------------------------------------------------------------
void stopAllCoils() {
  for (int i = 0; i < 4; i++) {
    driver.setChannelPWM(i, STOP_PULSE);
  }
}

// ---------------------------------------------------------------
//  dispenseAction
//  Rotates the chosen coil one full turn (360°) clockwise to
//  push out one pad, then stops.
// ---------------------------------------------------------------
void dispenseAction(char choice) {
  int chan = (choice == 'A') ? CHAN_A :
             (choice == 'B') ? CHAN_B :
             (choice == 'C') ? CHAN_C : CHAN_D;

  driver.setChannelPWM(chan, CW_PULSE);
  delay(4 * TIME_PER_90_DEG); // Rotate for 4 = 360 degrees
  driver.setChannelPWM(chan, STOP_PULSE);
}

// ---------------------------------------------------------------
//  performReset
//  Spins the coil counter-clockwise to unjam, then stops.
//  Called both from keypad admin mode and the web dashboard.
// ---------------------------------------------------------------
void performReset(int chan, char label) {
  lcdMessage("Resetting Coil", String(label).c_str());
  driver.setChannelPWM(chan, CCW_PULSE);
  delay(TIME_PER_90_DEG * 3);
  driver.setChannelPWM(chan, STOP_PULSE);
  lcdReady();
}

// ---------------------------------------------------------------
//  lcdMessage
//  Convenience wrapper: clears the LCD and prints up to two lines.
// ---------------------------------------------------------------
void lcdMessage(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2 && line2[0] != '\0') {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

// ---------------------------------------------------------------
//  lcdReady
//  Returns the LCD to the default idle prompt.
// ---------------------------------------------------------------
void lcdReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sanitary Hub");
  lcd.setCursor(0, 1);
  lcd.print("READY: Enter ID");
}
