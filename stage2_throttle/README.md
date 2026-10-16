# Stage 2 - Throttle Simulation

ESP32 sketch that simulates a throttle pedal with a potentiometer. The throttle position is
shown live on a 128x64 OLED as a semicircular gauge, and a red/green LED pair fades from
green (idle) through yellow to red (full throttle) using PWM.

## Features

- 12-bit ADC reading with full 0-3.3 V range (11 dB attenuation)
- Exponential moving average (EMA) filter to smooth out ADC noise
- Deadzone at the bottom of the travel, since the ADC never reads exactly 0
- Hardware PWM (LEDC) colour blending: green -> yellow -> red
- OLED dashboard at 20 FPS: gauge arc with ticks, needle, percentage and zone (LOW / MID / HIGH)
- Non-blocking timing with `millis()`; display, LEDs and serial logging run at independent rates

## Hardware

| Component | ESP32 pin |
|---|---|
| Potentiometer wiper | GPIO 34 (ADC1_CH6) |
| Red LED (with resistor) | GPIO 25 |
| Green LED (with resistor) | GPIO 26 |
| SSD1306 OLED 128x64, I2C address `0x3C` | SDA 21, SCL 22 |

Board: ESP32 DOIT DevKit V1. The potentiometer's outer legs go to 3.3 V and GND.

## How it works

1. **Filtering:** each raw reading is blended with the previous value,
   `smoothed = 0.2 * raw + 0.8 * smoothed`. The filter is seeded with the first reading so
   it does not ramp up from zero at boot.
2. **Mapping:** readings below the deadzone (80) are clamped to 0, then the 0-4095 range is
   mapped to 0-100 %.
3. **LED colour:** from 0 to 50 % the red channel rises while green stays full (green ->
   yellow); from 50 to 100 % green falls while red stays full (yellow -> red).
4. **Gauge:** the arc runs from 180° to 360° around a pivot at the bottom centre of the
   screen, and the needle angle is `180° + pct * 1.8°`.

Serial output (115200 baud, every 250 ms):

```
ADC:2048  PCT:50%  Zone:MID
```

## Build

With [PlatformIO](https://platformio.org/), from this folder:

```
pio run -t upload
pio device monitor
```

Libraries (installed automatically): Adafruit SSD1306, Adafruit GFX.

The sketch uses the LEDC API from Arduino-ESP32 core 2.x (`ledcSetup` / `ledcAttachPin`).
On core 3.x these calls were replaced by `ledcAttach` / `analogWrite`.
