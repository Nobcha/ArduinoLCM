// The final version  MEM: 56%/43%  !!RELAY VERSION!!
// L/C Meter Version 3.0 with OLED (SSD1306 I2C, u8g2 paged buffer) and PJRC FreqCount library
// MCU: Arduino NANO, OSC: 74HCU04, Relay
// Display: OLED SSD1306 128x64 I2C (u8g2 paged buffer)
// Frequency Counter: FreqCount library (PJRC version)
// Full calibration with safe sequence and minimal SRAM usage
// To wait 500mS for stability
// F1: f_without;
// F2: f_with;
// F3: MeasureFrequency
// 
// Reviced Freq Time 2025.06.23 21:30

#include <Wire.h>
#include <U8g2lib.h>
#include <FreqCount.h>

// Paged buffer (page mode)
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

const int FREQ_INPUT_PIN = 5;      // Frequency counter input (T1)
const int RELAY_CONTROL_PIN = 6;   // Calibration relay control (moved to avoid conflicts)
const int LC_MODE_PIN = 7;         // L/C select switch (DPDT)
const float STD_CAPA = 1000.0e-12; // Standard capacitor 1000pF

float L_calibrated = 0;
float C1_calibrated = 0;
unsigned long F1_saved = 0;        // To save F1
bool calibrated = false;

void setup() {
  pinMode(RELAY_CONTROL_PIN, OUTPUT);
  pinMode(LC_MODE_PIN, INPUT_PULLUP);
  digitalWrite(RELAY_CONTROL_PIN, LOW);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);

  showStartupMessage();
}

void loop() {
  if (!calibrated) {
    if (digitalRead(LC_MODE_PIN) == HIGH) {
      showMessage("C mode detected", "Hold for calibration");
      delay(2000);
      if (digitalRead(LC_MODE_PIN) == HIGH) {
        calibrate();
      }
    } else {
      showMessage("Waiting for C", "switch position...");
    }
  } else {
    measure();
  }
  delay(500);
}

void calibrate() {
  digitalWrite(RELAY_CONTROL_PIN, HIGH);
  delay(500);
  unsigned long f_with = stableMeasureFrequency();     // F2

  digitalWrite(RELAY_CONTROL_PIN, LOW);
  delay(500);
  unsigned long f_without = stableMeasureFrequency();  // F1

  if (f_with < 100 || f_without < 100) {
    showMessage("Calibration Failed", "NO OSC");
    return;
  }

  float w1 = 2 * PI * f_without;  // F1
  float w2 = 2 * PI * f_with;     // F2
  L_calibrated = (1.0 / (w2 * w2) - 1.0 / (w1 * w1)) / STD_CAPA;
  C1_calibrated = 1.0 / (w1 * w1 * L_calibrated);
  F1_saved = f_without;

  u8g2.firstPage();
  do {
    u8g2.setCursor(0, 10);
    u8g2.print("Calibration OK");
    u8g2.setCursor(0, 25);
    u8g2.print("F1: "); u8g2.print(f_without); u8g2.print(" Hz");
    u8g2.setCursor(0, 40);
    u8g2.print("F2: "); u8g2.print(f_with); u8g2.print(" Hz");
    u8g2.setCursor(0, 55);
    u8g2.print("L="); u8g2.print(L_calibrated * 1e6, 2); u8g2.print("uH");
    u8g2.setCursor(70, 55);
    u8g2.print("C="); u8g2.print(C1_calibrated * 1e12, 2); u8g2.print("pF");
  } while (u8g2.nextPage());

  calibrated = true;
  delay(3000);
}

void measure() {
  bool isLmode = (digitalRead(LC_MODE_PIN) == LOW);
  unsigned long f3 = stableMeasureFrequency();

  if (f3 < 100) {    // too law 
    showMessage("NO OSC", "");
    return;
  }

  float value = 0;
  char unit[4] = "";

  if (isLmode) {
    float w3 = 2 * PI * f3;
    float L_total = 1.0 / (w3 * w3 * C1_calibrated);
    float L2 = L_total - L_calibrated;
    value = L2;
    convertLunit(value, unit);
  } else {
    float w3 = 2 * PI * f3;
    float Ctotal = 1.0 / (w3 * w3 * L_calibrated);
    float Cx = Ctotal - C1_calibrated;
    value = Cx;
    convertCunit(value, unit);
  }

  u8g2.firstPage();
  do {
    drawIcon(isLmode);
    u8g2.setFont(u8g2_font_fub17_tr);
    u8g2.setCursor(0, 35);
    u8g2.print(value, 2);
    u8g2.print(" ");
    u8g2.print(unit);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 55);
    u8g2.print("F1: "); u8g2.print(F1_saved);
    u8g2.setCursor(0, 63);
    u8g2.print("F3: "); u8g2.print(f3);
  } while (u8g2.nextPage());
}

// To read twice and check within 0.5% error
unsigned long stableMeasureFrequency() {
  delay(100);
  FreqCount.begin(1000);
  while (!FreqCount.available());
  unsigned long freq1 = FreqCount.read();
  delay(10);
  FreqCount.begin(1000);
  while (!FreqCount.available());
  unsigned long freq2 = FreqCount.read();

  if (abs((long)(freq2 - freq1)) > freq1 * 0.005) {
    delay(10);
    FreqCount.begin(1000);
    while (!FreqCount.available());
    freq2 = FreqCount.read();
    if (abs((long)(freq2 - freq1)) > freq1 * 0.005) {
      return 0;
    }
  }
  return freq2;
}

void showStartupMessage() {
  showMessage("L/C Meter V6.4", "Remove L/C for CAL");
  delay(3000);
}

void showMessage(const char* line1, const char* line2) {
  u8g2.firstPage();
  do {
    u8g2.setCursor(0, 30);
    u8g2.print(line1);
    u8g2.setCursor(0, 50);
    u8g2.print(line2);
  } while (u8g2.nextPage());
}

void convertCunit(float &val, char *unit) {
  if (val >= 1e-6) {
    val *= 1e6; strcpy(unit, "uF");
  } else if (val >= 1e-9) {
    val *= 1e9; strcpy(unit, "nF");
  } else {
    val *= 1e12; strcpy(unit, "pF");
  }
}

void convertLunit(float &val, char *unit) {
  if (val >= 1e-3) {
    val *= 1e3; strcpy(unit, "mH");
  } else {
    val *= 1e6; strcpy(unit, "uH");
  }
}

void drawIcon(bool isLmode) {
  if (isLmode) {
    u8g2.drawLine(5,10,10,5);
    u8g2.drawLine(10,5,15,10);
    u8g2.drawLine(15,10,20,5);
    u8g2.drawLine(20,5,25,10);
    u8g2.drawLine(25,10,30,5);
    u8g2.drawLine(30,5,35,10);
  } else {
    u8g2.drawVLine(15, 5, 15);
    u8g2.drawVLine(25, 5, 15);
    u8g2.drawLine(0, 12, 15, 12);
    u8g2.drawLine(25, 12, 50, 12);
  }
}
