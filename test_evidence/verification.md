# Verification record - 10 October 2026

Target: Arduino Uno R3, ATmega328P at 16 MHz. Arduino CLI 1.5.1, Arduino AVR core 1.8.8, avr-gcc 7.3.0-atmel3.6.1-arduino7 and LiquidCrystal_I2C 2.0.0.

The final firmware compiles to 9,194 bytes of flash and 920 bytes of static RAM. Shared protection tests: 37 pass / zero fail. Full-firmware integration checks: 19 pass / zero fail. AVR8js 0.21.0 executes the compiled instructions and models ADC, GPIO, timer, USART and I2C bus timing.

The final integration run observed D8 falling after 3.982 ms for the OV step and 0.878 ms for the OC step. The CSV, JSON and VCD files preserve input changes and output edges with timestamps derived from CPU cycles. These are simulated observations, not physical measurements or worst-case guarantees.

Wokwi visual checks confirmed normal operation, OV/OC trips, latched recovery, safe reset, local VMIN changes, computer settings and range rejection. See readme.md in this folder for the screenshot/log/capture index.

The LCD initializes before qualification at startup. Its refresh writes one character per loop so sampling can occur between characters. Physical ADC calibration, USB communication to a real board, real relay behavior and transient protection have not been tested. No mains circuit was built.
