#include "Protection.h"
uint16_t passed = 0, failed = 0;
void check(bool ok, const __FlashStringHelper *name) {
  Serial.print(ok ? F("PASS ") : F("FAIL ")); Serial.println(name);
  if (ok) ++passed; else ++failed;
}
void connect(Protection &p) { p.sample(0,230,2000); p.sample(3000,230,2000); }
const char *cmd(Protection &p, const char *s) {
  char b[64]; strncpy(b,s,sizeof(b)); b[63]=0; return command(p,b);
}
void setup() {
  Serial.begin(115200);
  Protection p;
  check(!p.on(),F("power-up load off"));
  p.sample(0,230,2000); p.sample(2999,230,2000);
  check(!p.on(),F("startup delay not shortened"));
  p.sample(3000,230,2000); check(p.on(),F("startup delay completes"));
  p.sample(3005,251,2000); check(p.state==OVERVOLTAGE && !p.on(),F("OV disconnects on first sample"));
  check(!p.reset(),F("reset rejected while unsafe"));
  p.sample(4000,230,2000); check(!p.on(),F("fault stays latched after recovery"));
  check(p.reset(),F("safe reset accepted"));
  p.sample(4000,230,2000); p.sample(6999,230,2000);
  check(!p.on(),F("reset still requires delay"));
  p.sample(7000,230,2000); check(p.on(),F("reset delay completes"));
  p.sample(7010,179,2000); check(p.state==UNDERVOLTAGE,F("UV disconnects"));
  Protection q; connect(q); q.sample(3005,230,5001);
  check(q.state==OVERCURRENT && !q.on(),F("OC disconnects on first sample"));
  q.sample(3010,290,18000); check(q.state==OVERCURRENT,F("first fault cause retained"));
  Protection r; connect(r); r.sample(3005,250,5000);
  check(r.on(),F("threshold equality does not trip"));
  r.sample(3010,180,5000); check(r.on(),F("lower threshold equality does not trip"));
  r.sample(3015,230,0,false); check(r.state==SENSOR && !r.on(),F("invalid sensor disconnects"));
  check(!r.reset(),F("invalid sensor blocks reset"));
  Protection h; h.sample(0,184,2000); h.sample(4000,184,2000);
  check(!h.on(),F("restart voltage hysteresis"));
  h.sample(5000,230,4600); h.sample(9000,230,4600);
  check(!h.on(),F("restart current hysteresis"));
  h.sample(10000,230,2000); h.sample(12000,246,2000); h.sample(13000,230,2000);
  h.sample(15999,230,2000); check(!h.on(),F("unsafe recovery band restarts timer"));
  h.sample(16000,230,2000); check(h.on(),F("continuous safe recovery succeeds"));
  Protection wrap; wrap.sample(0xfffffff0UL,230,2000);
  wrap.sample((uint32_t)(0xfffffff0UL+2999UL),230,2000);
  check(!wrap.on(),F("millis rollover waits"));
  wrap.sample((uint32_t)(0xfffffff0UL+3000UL),230,2000);
  check(wrap.on(),F("millis rollover reconnects correctly"));
  Protection c; connect(c);
  check(!strcmp(cmd(c,"SET VMAX 240"),"OK") && c.settings.vmax==240 && !c.on(),F("PC setting applies and disconnects"));
  check(!strcmp(cmd(c,"SET IMAX 7000"),"OK") && c.settings.imax==7000,F("PC current setting"));
  check(!strcmp(cmd(c,"SET DELAY 5000"),"OK") && c.settings.delayMs==5000,F("PC delay setting"));
  check(!strcmp(cmd(c,"SET VMIN 190"),"OK") && c.settings.vmin==190,F("PC minimum voltage setting"));
  check(!strcmp(cmd(c,"SET VMAX 199"),"ERR RANGE") && c.settings.vmax==240,F("invalid range is atomic"));
  check(!strcmp(cmd(c,"SET VMIN 230"),"ERR RANGE") && c.settings.vmin==190,F("cross-field validation"));
  check(!strcmp(cmd(c,"SET IMAX -1"),"ERR NUMBER"),F("reject negative number"));
  check(!strcmp(cmd(c,"SET IMAX 5x"),"ERR NUMBER"),F("reject trailing junk"));
  check(!strcmp(cmd(c,"SET IMAX 99999999999"),"ERR RANGE"),F("reject integer overflow"));
  check(!strcmp(cmd(c,"SET IMAX "),"ERR NUMBER"),F("reject missing number"));
  check(!strcmp(cmd(c,"SET IMAX 5000 extra"),"ERR NUMBER"),F("reject extra token"));
  check(!strcmp(cmd(c,"ON"),"ERR COMMAND"),F("no force-on command"));
  check(!strcmp(cmd(c,"GET"),"STATUS"),F("status request"));
  Protection local; connect(local); Settings s=local.settings; s.imax=6000;
  check(local.configure(s) && !local.on(),F("local setting disconnects"));
  local.sample(3005,260,0); s.vmax=270; local.configure(s);
  check(local.state==OVERVOLTAGE,F("settings cannot clear a fault"));
  Serial.print(F("RESULT passed=")); Serial.print(passed);
  Serial.print(F(" failed=")); Serial.println(failed);
}
void loop() {}
