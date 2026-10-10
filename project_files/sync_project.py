"""Refresh Wokwi and test copies from the canonical Arduino source."""
from pathlib import Path
import shutil
ROOT = Path(__file__).resolve().parents[1]
firmware = ROOT / 'src' / 'VoltageProtector'
wokwi = ROOT / 'project_files' / 'wokwi'
wokwi.mkdir(parents=True, exist_ok=True)
shutil.copyfile(firmware / 'VoltageProtector.ino', wokwi / 'sketch.ino')
shutil.copyfile(firmware / 'Protection.h', wokwi / 'Protection.h')
shutil.copyfile(firmware / 'Protection.h', ROOT / 'project_files/tests/selftest/Protection.h')
print('Wokwi and test sources synchronized.')
