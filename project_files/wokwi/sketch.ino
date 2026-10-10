#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Protection.h"

// SAFE DEMONSTRATOR ONLY: A0/A1 are 0..5 V potentiometer signals.
// D8 drives a resistor + LED, representing the normally-open load switch.
// No mains connections, relay coil or appliance may be wired to this sketch.
LiquidCrystal_I2C lcd(0x27, 16, 2); // Uno: SDA=A4, SCL=A5
Protection protector;
const uint8_t LOAD_PIN = 8, FAULT_PIN = 13;
const uint8_t BUTTONS[] = {10, 9, 7, 6}; // menu, +, -, reset; ordered for clean routing
uint8_t menu = 0;
uint32_t lastSample = 0, lastLcd = 0, lastTelemetry = 0;
char inputLine[48];
uint8_t inputLength = 0;
bool overflow = false;
bool rawButtons[4] = {true, true, true, true};
bool stableButtons[4] = {true, true, true, true};
uint32_t changedAt[4] = {0, 0, 0, 0};
char lcdRows[32];
uint8_t lcdPosition = 32;

void outputs() {
  digitalWrite(LOAD_PIN, protector.on() ? HIGH : LOW);
  digitalWrite(FAULT_PIN, protector.state >= UNDERVOLTAGE ? HIGH : LOW);
}
// All output lines fit the UNO's 63-byte free UART ring buffer.
// Drop a telemetry/response line if busy rather than block protection sampling.
void sendLine(const char *line) {
  size_t length = strlen(line);
  if (Serial.availableForWrite() >= (int)(length + 2)) Serial.println(line);
}
void status() {
  char b[62];
  snprintf(b, sizeof(b), "S %s %u %u %u %u %u %lu", stateName(protector.state),
    protector.voltage, protector.current, protector.settings.vmin,
    protector.settings.vmax, protector.settings.imax, protector.settings.delayMs);
  sendLine(b);
}
void serialInput() {
  // Work budget prevents serial flooding from starving the protection loop.
  for (uint8_t n = 0; n < 16 && Serial.available(); ++n) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (overflow) sendLine("ERR LENGTH");
      else {
        inputLine[inputLength] = 0;
        const char *reply = command(protector, inputLine);
        outputs();
        if (!strcmp(reply, "STATUS")) status(); else sendLine(reply);
      }
      inputLength = 0; overflow = false;
    } else if (c < 32 || c > 126) overflow = true;
    else if (inputLength < sizeof(inputLine) - 1) inputLine[inputLength++] = c;
    else overflow = true;
  }
}
void changeSetting(int direction) {
  Settings s = protector.settings;
  if (menu == 1) s.vmin += direction;
  if (menu == 2) s.vmax += direction;
  if (menu == 3) s.imax += direction * 500;
  if (menu == 4) s.delayMs += direction * 1000L;
  if (menu) {
    sendLine(protector.configure(s) ? "OK LOCAL" : "ERR RANGE");
    outputs();
  }
}
void buttons(uint32_t now) {
  for (uint8_t n = 0; n < 4; ++n) {
    bool raw = digitalRead(BUTTONS[n]);
    if (raw != rawButtons[n]) { rawButtons[n] = raw; changedAt[n] = now; }
    if (raw != stableButtons[n] && (uint32_t)(now - changedAt[n]) >= 30) {
      stableButtons[n] = raw;
      if (!raw) {
        if (n == 0) menu = (menu + 1) % 5;
        if (n == 1) changeSetting(1);
        if (n == 2) changeSetting(-1);
        if (n == 3) { sendLine(protector.reset() ? "OK RESET" : "ERR UNSAFE"); outputs(); }
      }
    }
  }
}
void display() {
  char row[17];
  snprintf(row, sizeof(row), "%3uV %2u.%02uA %s", protector.voltage,
    protector.current / 1000, (protector.current % 1000) / 10,
    protector.on() ? "ON" : "OFF");
  memset(lcdRows, ' ', sizeof(lcdRows));
  memcpy(lcdRows, row, strlen(row));
  if (menu == 1) snprintf(row, sizeof(row), "VMIN %u V", protector.settings.vmin);
  else if (menu == 2) snprintf(row, sizeof(row), "VMAX %u V", protector.settings.vmax);
  else if (menu == 3) snprintf(row, sizeof(row), "IMAX %u mA", protector.settings.imax);
  else if (menu == 4) snprintf(row, sizeof(row), "DELAY %lu ms", protector.settings.delayMs);
  else snprintf(row, sizeof(row), "%s MENU=settings", stateName(protector.state));
  memcpy(lcdRows + 16, row, strlen(row));
  lcdPosition = 0;
}
void serviceDisplay() {
  // One character per pass: the next pass samples protection before more I2C work.
  if (lcdPosition >= 32) return;
  if (lcdPosition == 0 || lcdPosition == 16) lcd.setCursor(0, lcdPosition / 16);
  lcd.write((uint8_t)lcdRows[lcdPosition++]);
}
void setup() {
  digitalWrite(LOAD_PIN, LOW); pinMode(LOAD_PIN, OUTPUT);
  digitalWrite(FAULT_PIN, LOW); pinMode(FAULT_PIN, OUTPUT);
  for (uint8_t n = 0; n < 4; ++n) pinMode(BUTTONS[n], INPUT_PULLUP);
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  Wire.setWireTimeout(3000, true); // Bound a stalled I2C transaction.
  sendLine("Voltage Protector UNO / RAM settings / GET for status");
}
void loop() {
  uint32_t now = millis();
  if ((uint32_t)(now - lastSample) >= 5) {
    lastSample = now;
    uint16_t av = analogRead(A0), ai = analogRead(A1);
    uint16_t v = (uint32_t)av * 300 / 1023;
    uint16_t i = (uint32_t)ai * 20000 / 1023;
    // ADC top rail is treated as invalid/saturated; low voltage trips via UV.
    protector.sample(now, v, i, av < 1022 && ai < 1022);
    outputs();
  }
  serialInput();
  buttons(millis());
  if (lcdPosition == 32 && (uint32_t)(now - lastLcd) >= 250) { lastLcd = now; display(); }
  serviceDisplay();
  if ((uint32_t)(now - lastTelemetry) >= 500) { lastTelemetry = now; status(); }
}
