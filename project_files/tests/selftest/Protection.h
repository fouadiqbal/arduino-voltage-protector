#ifndef PROTECTION_H
#define PROTECTION_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Integer engineering units: volts, milliamps and milliseconds.
struct Settings {
  uint16_t vmin, vmax, imax;
  uint32_t delayMs;
};
inline Settings defaults() { Settings s = {180, 250, 5000, 3000}; return s; }
inline bool validSettings(const Settings &s) {
  return s.vmin >= 80 && s.vmin <= 230 && s.vmax >= 200 &&
    s.vmax <= 280 && s.vmax >= s.vmin + 20 && s.imax >= 1000 &&
    s.imax <= 15000 && s.delayMs >= 1000 && s.delayMs <= 30000;
}
enum State { WAITING, CONNECTED, UNDERVOLTAGE, OVERVOLTAGE, OVERCURRENT, SENSOR };
inline const char *stateName(State s) {
  switch(s) {
    case WAITING: return "WAIT"; case CONNECTED: return "ON";
    case UNDERVOLTAGE: return "UV"; case OVERVOLTAGE: return "OV";
    case OVERCURRENT: return "OC"; default: return "SENSOR";
  }
}
class Protection {
 public:
  Settings settings;
  State state;
  uint16_t voltage, current;
  bool valid;
  Protection(): settings(defaults()), state(WAITING), voltage(0), current(0),
    valid(false), timing(false), since(0) {}
  bool on() const { return state == CONNECTED; }
  bool safe() const {
    return valid && voltage >= settings.vmin + 5 && voltage <= settings.vmax - 5
      && current <= settings.imax - 500;
  }
  void sample(uint32_t now, uint16_t v, uint16_t i, bool sensorValid = true) {
    voltage = v; current = i; valid = sensorValid;
    State fault = !valid ? SENSOR : (i > settings.imax ? OVERCURRENT :
      (v > settings.vmax ? OVERVOLTAGE : (v < settings.vmin ? UNDERVOLTAGE : WAITING)));
    // Keep the first cause latched. No command can force an unsafe reconnection.
    if (state >= UNDERVOLTAGE) return;
    if (fault != WAITING) { state = fault; timing = false; return; }
    if (state == CONNECTED) return;
    if (!safe()) { timing = false; return; }
    if (!timing) { since = now; timing = true; }
    if ((uint32_t)(now - since) >= settings.delayMs) state = CONNECTED;
  }
  bool reset() {
    if (!safe()) return false;
    state = WAITING; timing = false; return true;
  }
  bool configure(const Settings &candidate) {
    if (!validSettings(candidate)) return false;
    settings = candidate;
    if (state == CONNECTED) state = WAITING;
    timing = false;
    return true;
  }
 private:
  bool timing;
  uint32_t since;
};

// Strict line parser shared by the firmware and compiled AVR tests.
// Returns OK, STATUS, or a short error string. Mutates only on a valid full line.
inline const char *command(Protection &p, char *line) {
  if (!strcmp(line, "GET")) return "STATUS";
  if (!strcmp(line, "RESET")) return p.reset() ? "OK" : "ERR UNSAFE";
  if (strncmp(line, "SET ", 4)) return "ERR COMMAND";
  char *key = line + 4;
  char *space = strchr(key, ' ');
  if (!space || space == key) return "ERR FORMAT";
  *space++ = 0;
  if (!*space) return "ERR NUMBER";
  uint32_t value = 0;
  for (char *q = space; *q; ++q) {
    if (*q < '0' || *q > '9') return "ERR NUMBER";
    value = value * 10 + (*q - '0');
    if (value > 30000) return "ERR RANGE";
  }
  Settings s = p.settings;
  if (!strcmp(key, "VMIN")) s.vmin = value;
  else if (!strcmp(key, "VMAX")) s.vmax = value;
  else if (!strcmp(key, "IMAX")) s.imax = value;
  else if (!strcmp(key, "DELAY")) s.delayMs = value;
  else return "ERR KEY";
  return p.configure(s) ? "OK" : "ERR RANGE";
}
#endif
