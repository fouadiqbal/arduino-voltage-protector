# Low-voltage demonstrator wiring

Use an **Arduino Uno R3 (ATmega328P)** powered by USB. Every ground below connects to Uno GND. No mains wiring is included or permitted. The green LED is the protected-load stand-in; this is not an appliance switch.

| Component | Connection |
|---|---|
| 10 kohm voltage potentiometer | outer pins 5 V/GND, wiper A0 |
| 10 kohm current potentiometer | outer pins 5 V/GND, wiper A1 |
| MENU button | D10 to GND when pressed |
| PLUS button | D9 to GND when pressed |
| MINUS button | D7 to GND when pressed |
| RESET button | D6 to GND when pressed |
| Green load LED | D8 -> 330 ohm resistor -> anode; cathode GND |
| Red fault LED | D13 -> 330 ohm resistor -> anode; cathode GND |
| I2C LCD1602 SDA, SCL | A4, A5 respectively |
| I2C LCD1602 VCC, GND | 5 V, GND respectively |
| I2C LCD address | 0x27 (set the physical module to this address) |
| USB cable | Uno USB connector to computer |

Buttons use internal pull-ups and 30 ms software debounce. Press/release once for each increment; no auto-repeat. D0 and D1 remain available for the Uno USB serial interface. The two analog controls represent precomputed voltage/current magnitudes, not sampled AC waveforms.

The display uses a PCF8574 I2C backpack and the LiquidCrystal_I2C 2.0.0 library. Its physical module includes backlight/contrast circuitry; adjust its contrast trimmer. Display refresh is spread over loop passes, one character at a time, so sampling can continue between characters.

Conversion: `V = floor(ADC_A0 * 300 / 1023)` and `mA = floor(ADC_A1 * 20000 / 1023)`. Nominal knob values: A0=785 (~230 V), A1=102 (~1.994 A). These displayed numbers do not exist electrically: the physical signals stay within 0-5 V. At either ADC value >=1022, firmware declares a SENSOR fault. Open-circuit/floating sensor faults are not reliably detected.

`block_diagram.svg` shows the implemented system. `project_files/wokwi/diagram.json` contains the editable simulation circuit.
