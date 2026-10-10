# Test evidence

Recorded on 10 October 2026. Wokwi screenshots and logs are browser simulation results. CSV, VCD and timing plots come from the separately executed compiled-AVR integration runner. No physical hardware measurement is claimed.

## Wokwi screenshots

| File | Demonstration |
|---|---|
| [01_normal_operation.png](screenshots/01_normal_operation.png) | Safe input, LCD readings and green load LED ON |
| [02_over_voltage.png](screenshots/02_over_voltage.png) | 270 V equivalent causes OV and load OFF |
| [03_latched_after_recovery.png](screenshots/03_latched_after_recovery.png) | Safe 230 V input does not automatically clear OV |
| [04_over_current.png](screenshots/04_over_current.png) | About 6 A causes OC and load OFF |
| [05_local_settings.png](screenshots/05_local_settings.png) | MENU and PLUS change VMIN from 180 to 181 V |
| [06_computer_settings.png](screenshots/06_computer_settings.png) | Serial commands set IMAX=4000 mA and DELAY=5000 ms |
| [07_invalid_setting.png](screenshots/07_invalid_setting.png) | SET VMAX 0 returns ERR RANGE |
| [08_computer_reset.png](screenshots/08_computer_reset.png) | Safe serial RESET reconnects after qualification |

[First Wokwi serial run](wokwi_serial_log.txt) and [screenshot recapture run](wokwi_serial_log_second_run.txt) is exported from the actual browser serial-monitor output. It contains repeated periodic status lines; command replies and transitions appear between them. See the [saved Wokwi project](https://wokwi.com/projects/477495301282380801).

## Automated logs

- [unit_tests.txt](unit_tests.txt): 37 protection checks, zero failures.
- [integration.txt](integration.txt): 19 compiled-firmware ADC/GPIO/button/serial checks, zero failures.
- [source_hashes.json](source_hashes.json): SHA-256 of the final tested firmware sources.

## Timing captures

- [timing_capture.csv](timing_capture.csv): input and D8 events, with microsecond timestamps.
- [timing_events.json](timing_events.json): the same events in structured form.
- [timing_capture.vcd](timing_capture.vcd): waveform format for GTKWave or another VCD viewer; timescale 1 us.
- [timing_fault_response.png](timing_fault_response.png): fault step versus D8 falling edge.
- [timing_full_run.png](timing_full_run.png): output timeline for the entire integration run.

OV delay in this run: **3.982 ms**. OC delay: **0.878 ms**. The voltage and current signals are independently injected stand-ins. Timestamps come from simulated CPU cycles at 16 MHz. The values are observations of these two test phases, not worst-case bounds. Real sensors, electrical transients and relay delays are not modeled.

The emulator models I2C transaction timing and acknowledges the LCD at 0x27. The LCD pixels are verified visually in Wokwi. Startup includes the LCD library initialization before safe-input qualification. Display characters are subsequently written one at a time, with protection checked between loop passes.

Run the commands in the root README to reproduce the logs and plots. `plot_timing.py` reads the actual recorded event file.
