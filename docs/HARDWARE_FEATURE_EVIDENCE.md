# Reference feature hardware evidence

Date: 2026-09-11. Target: connected Huangshan Pi SF32LB52, current installed
English eBadge firmware. This inspection does not verify the new Chinese pages.

## Observed on the device

NSH enumerated adc0, buttons, config0, fb0, gpio0..2, i2c0..1, input0,
lcd0, lsm6dsl0, mmcsd0, pwm0, rtc0, spi1, timer0, ttyACM0, ttyS0,
watchdog0 and basic console/null/random devices.

`ls /dev/audio` returned `stat failed: 2`; no audio endpoint is exposed by
the currently running firmware. This does not mean the board lacks audio HW.

The included `lsm6dsl_reader` application ran against /dev/lsm6dsl0 and
returned these real samples (field names retained, units NOT inferred):

```text
time:0   x:-98  y:-2  z:-967 m_x:-30380 m_y:-14226 m_z:-13424 temp:29
time:326 x:-123 y:-12 z:-966 m_x:2170   m_y:-3780  m_z:840    temp:30
time:651 x:-121 y:-19 z:-977 m_x:2380   m_y:-4620  m_z:910    temp:30
time:977 x:-117 y:-17 z:-986 m_x:2590   m_y:-4620  m_z:910    temp:30
```

This proves sample delivery, not shake recognition, axis calibration, a
validated physical-unit conversion, sample frequency or reliable error handling.
The inspected lsm6dsl_sensor_read implementation calls register reads without
checking their return values. A successful high-level ioctl alone is therefore
not sufficient evidence that every sample is valid. Resolve this before using
the data for automatic actions. Shake cooldown remains 1500 ms in project scope.

Serial opening caused visible SFBL/NSH reboots. The reader was a temporary
diagnostic, not a background feature to leave running alongside the UI.

## Battery and charging

Boot explicitly reports missing ADC calibration data and use of defaults
(ratio 1068, offset 822). The sf32lb_adc driver selects ADC_CHAN_VBAT and
passes an adc_mv value to the ADC upper half; trigger and nonblocking read
are possible integration paths. The actual measurement scaling, calibration,
battery presence and charge-state interpretation are not yet validated.
No battery percentage, low-battery threshold or charging icon is implemented
from these observations. No charging/protection registers were changed.

## RGB lighting

Board manufacturer documentation identifies the RGB LED as WS2812B-2020:
https://wiki.lckfb.com/zh-hans/hspi-sf32lb52/hardware/board.html

Pinmux routes PA32 to GPTIM2_CH1. The inspected sf32lb PWM driver generates
ordinary continuous PWM; it does not provide a WS2812 GRB frame encoder or
timed DMA stream. Do not treat /dev/pwm0 as a working RGB color API. Implement
and validate the actual WS2812 transport and supply requirements first.

## Audio

Manufacturer documentation states an onboard MEMS microphone and Class-D PA,
with an external speaker connector. NuttX nxplayer sources exist, but neither
that source directory nor the physical amplifier proves playable audio on the
installed firmware. A board audio lower half, playback test and connected
speaker confirmation remain required.

## Next integration gates

1. Verify IMU units, configuration and read-error propagation; then wire a
   worker-to-UI event queue and test shake/cooldown on hardware.
2. Verify battery measurement against a known reference and obtain real charge
   state without modifying battery protection.
3. Port the WS2812 transport and verify actual color output.
4. Bring up audio lower half and test real playback before enabling controls.

All other requirements in REFERENCE_FEATURES.md remain open as previously stated.
