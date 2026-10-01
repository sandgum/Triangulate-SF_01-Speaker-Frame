# SF_01 ESP32 Audio Firmware

Bluetooth A2DP sink → EQ → Linkwitz-Riley crossover → two I²S buses → two PCM5102A DACs.

**Target: ESP32-WROVER-E-N16R8** (U28 on the SF_01 Compute Board).
A2DP is a Bluetooth *Classic* profile, so this cannot run on an ESP32-S3 —
the S3 is BLE-only. (The top-level project README still describes the audio
board as an "ESP32-S3 WROOM 2"; the schematic and BOM say WROVER-E, which is
the part that can actually do this.)

Built and verified against **ESP-IDF v5.5.1**.

## Signal chain

```
A2DP (SBC, 44.1 kHz)
   └─► ring buffer ─► render task
          ├─ volume (AVRCP absolute volume)
          ├─ EQ: up to 8 peaking / shelf biquads
          └─ Linkwitz-Riley 4th order split @ 2.8 kHz
               ├─ low  ─► woofer trim  ─► I2S0 ─► U14 ─► TPA3116 ×3 ─► 6 × 3.5" mid-bass
               └─ high ─► tweeter trim ─► I2S1 ─► U5  ─► TPA3130 ×2 ─► 4 × 0.75" tweeters
```

LR4 is two cascaded Butterworth sections per branch. Low and high sum to an
all-pass, so both ways stay in phase through the crossover and **no polarity
flip is needed** — unlike LR2. An `invert` command is provided anyway, because
real drivers in a real baffle do not always agree with the theory.

Both I²S units are clocked from the **APLL** so they stay frequency-locked.
Left on the default divided PLL they drift apart, which smears the
woofer/tweeter phase relationship exactly where the crossover needs it to be
stable. The DMA chains are preloaded with silence and enabled back to back so
the two DACs start within well under one sample of each other.

## Pin map

Taken from the KiCad netlist of `PCB V1.1/SF_01 Compute Board`, module U28.

| Signal | GPIO | U28 pin | Test point | Goes to |
|---|---|---|---|---|
| `WFR_BCK` | 25 | 10 | TP11 | U14 BCK |
| `WFR_LRCK` | 19 | 31 | TP18 | U14 LRCK |
| `WFR_DIN` | 23 | 37 | TP15 | U14 DIN |
| `WFR_SCK` | 26 | 11 | TP12 | U14 SCK — **held low** |
| `TWT_BCK` | 15 | 23 | TP8 | U5 BCK |
| `TWT_LRCK` | 13 | 16 | TP9 | U5 LRCK |
| `TWT_DIN` | 14 | 13 | TP13 | U5 DIN |
| `TWT_SCK` | 18 | 30 | TP16 | U5 SCK — **held low** |
| `DAC_EN` | 4 | 26 | TP17 | XSMT on **both** DACs |
| `DRIVER_MUTE` | 27 | 12 | TP14 | MUTE on all five amps |

### Why SCK is driven low

The PCM5102A's SCK (pin 12) is wired to the ESP32 on this board, but the
classic ESP32 can only emit an I²S master clock on GPIO 0/1/3 — neither 26 nor
18 qualifies. Firmware therefore holds both pins **low**, which is TI's
documented "SCK to GND" mode: the DAC runs its internal PLL off BCK
(PCM5102A datasheet §9.4.1). A *floating* SCK never locks, so this is not
optional — it is the firmware half of a hardware compromise.

## Power-up sequencing

Every delay below comes from a datasheet, not from taste.

**At reset, before firmware runs, the amplifiers are live and un-muted.**
`DRIVER_MUTE` is held low by the five 100 kΩ pull-downs R20/R27/R36/R46/R56,
GPIO27 is high-Z out of reset, and the amps' SDZ pins are strapped to FAULTZ —
so they enable themselves the moment PVCC rises, while the DACs' XSMT pin is
still floating. `audio_out_safe_state()` is therefore the **first** call in
`app_main`, before NVS or Bluetooth, to close that window as early as software
can. The ~200 ms ROM-bootloader window ahead of it cannot be closed in
firmware — see *Hardware notes*.

**Start-up** (`audio_out_unmute`):

| Step | Wait | Source |
|---|---|---|
| Amps muted, DACs muted, SCK low | — | first thing in `app_main` |
| I²S enabled, silence flowing | 20 ms | PCM5102A resyncs if LRCK:BCK is invalid >4 LRCK periods (§9.3.2); 20 ms ≈ 880 periods |
| `XSMT` high → soft un-mute | 104 samples (2.4 ms @ 44.1 k) + 10 ms | 1 dB/sample ramp, §9.3.3 |
| `DRIVER_MUTE` low → amps live | — | TPA31xx MUTE, SLOS708G |

**Shutdown** is the exact reverse — amps go Hi-Z first (~2 µs), *then* the DAC
ramps down, so the mute ramp is never audible.

`XSMT` must be driven by a plain GPIO edge: the part needs t_r/t_f < 20 ns
(§8.7), and a 1→0 edge slower than 6 ms puts it into external-undervoltage
mode instead of digital mute. **Do not add an RC to that net.**

Clocks keep running with silence whenever no stream is playing, so the DAC
PLLs stay locked and resume is click-free.

## Bring-up wiring (devboard)

Power the devboard from **USB only**. Never connect devboard `VIN` or `3V3` to
any PCB rail — the PCB makes its own, and tying them together back-feeds
`3V3_A` (3.9 V abs max on the DACs) and pushes current backwards through
U11's body diode into `SYS_5V` and the CM4.

Connect **GND plus the ten signals** in the table above, devboard GPIO → PCB
test point. GND is not optional; without it the I²S lines have no reference.

## Build and flash

```bash
. ~/esp/esp-idf/export.sh
idf.py -p /dev/cu.usbserial-10 flash monitor
```

## Tuning

The UART console (115200) accepts live commands; `save` persists to NVS.

It runs in **dumb-terminal mode** — no line editing, history or tab completion,
and it behaves identically under `idf.py monitor`, `screen` and a plain script.
esp_console normally probes for VT100 support by emitting `ESC[5n` and treating
any 4 bytes received within 500 ms as the reply, so a script that starts talking
early — or a user typing during boot — convinces it a smart terminal is attached
and it then floods the UART with escape sequences nothing will answer.
`shell.c` calls `linenoiseSetDumbMode(1)` to take the predictable path.

| Command | Meaning |
|---|---|
| `status` | Show crossover, EQ, trims, output state |
| `xo <hz>` | Crossover frequency |
| `eq <band> <off\|peak\|lowshelf\|highshelf> <freq> <Q> <dB>` | Set one of 8 bands |
| `gain <wfr\|twt\|master> <dB>` | Per-way and master trim |
| `delay <samples>` | Time-align the tweeter |
| `invert <0\|1>` | Tweeter polarity |
| `vol <0-127>` | Volume (also driven by AVRCP) |
| `save` / `load` / `defaults` | NVS persistence |
| `mute` / `unmute` | Force output state |

Defaults: 2.8 kHz crossover, tweeter −3 dB, master −3 dB for EQ headroom.
These are a safe starting point, not a measured tuning.

A cubic soft-clipper sits at the output so EQ boost degrades gracefully
instead of hard-clipping.

## BLE control service

The controller runs in **dual mode**: BR/EDR carries the A2DP audio, BLE carries
an EQ/crossover control service for the iOS app in `App/SF01Control`. The
coexistence manager shares the one radio; control traffic is small and
infrequent, which is the case that coexists cleanly with a streaming link.

| | UUID |
|---|---|
| Service | `5F010000-A1B2-4C3D-8E4F-5F0100000001` |
| `CONFIG` — read, notify | `5F010001-A1B2-4C3D-8E4F-5F0100000001` |
| `COMMAND` — write | `5F010002-A1B2-4C3D-8E4F-5F0100000001` |

`CONFIG` is a 78-byte little-endian snapshot of the whole DSP state, re-published
on any change. A publisher task polls the rendered blob every 200 ms rather than
hooking each writer, so changes made from the UART shell or by AVRCP volume show
up on the phone too. `COMMAND` takes short opcode frames, at most 9 bytes, so
they fit even at the 23-byte default ATT MTU. Both formats are specified in
`main/ble_ctl.h`, and `App/SF01Control/SF01Protocol.swift` mirrors them — change
one, change the other. The version byte is checked on every read, so a mismatch
surfaces as an explicit error rather than as odd values.

Adding BLE grew the binary from 821 KB to 1.07 MB; the 2 MB partition still has
49 % free.

## UI tones

Three synthesised tones: a rising C-E-G-C arpeggio at startup, a rising
two-note on Bluetooth connect, and its inversion on disconnect.

They are **generated, not stored** — no WAV assets, no flash budget (the whole
feature costs about 1.6 KB), and they follow whatever sample rate A2DP
negotiated without resampling.

Rendering happens **inside the audio render task, in place of the stream**,
rather than being pushed through the ring buffer. That avoids extra buffering
and any pacing logic — the I2S clock paces it — and the tone still passes
through the normal DSP chain, so the crossover splits it properly across the
woofers and tweeters instead of landing on one way only. Stream audio arriving
during a tone is dropped rather than queued, so playback resumes live instead
of a few hundred ms behind.

`chime.c` uses a 1024-point interpolated sine table rather than `sinf()` per
sample: four overlapping voices with three harmonics each is twelve sines per
frame, and calling `sinf()` that often at 44.1 kHz would take a serious bite
out of the CPU the DSP needs. An underrun during the startup tone is exactly
the wrong first impression.

Tones need the analog chain live, which at boot or between streams it is not,
so `do_chime()` un-mutes with the usual sequence, plays, waits for the tail to
clear the DMA chain, then **restores the previous mute state** — a tone never
leaves the amplifiers hot. Measured windows, amps live:

| tone | duration |
|---|---|
| startup | 926 ms |
| connect | 518 ms |
| disconnect | 528 ms |

Play any of them from the shell:

```
chime                 startup tone
chime connect
chime disconnect
```

Because they run through the DSP, tones follow the master volume and EQ. At
volume 0 they are silent, which is the intended behaviour.

## LED bass visualiser

The devboard's onboard LED pulses with the low bass. `dsp_bass_level()` taps
the woofer branch **after its gain**, so the LED follows what the drivers
actually receive — volume and EQ included — then band-limits to ~120 Hz with a
24 dB/oct filter. Tapping the woofer branch raw would not work: it runs all the
way up to the crossover, so the LED would just track overall loudness.

The envelope uses a 4 ms attack and 180 ms release, so it snaps to a kick and
decays smoothly instead of flickering at the waveform rate. `led_viz.c` drives
the pin with **PWM, not on/off**, and raises the level to the power 2.2 because
perceived brightness goes roughly as duty^(1/2.2) — without that the pulse
blows out to full almost immediately.

Bass rarely approaches full scale after gain scaling, and how close it gets
depends on volume, so the task normalises against a slowly decaying peak
(~5 s) with a floor. The LED uses its whole range at any listening level
without amplifying near-silence into flicker. When the output is muted it
falls back to a dim 4-second breath.

Default pin is **GPIO2**, where essentially every generic ESP32 devkit puts its
user LED, and which is unconnected on the SF_01 board (U28 pin 24) so driving
it there is harmless. GPIO2 is a strapping pin, but this initialises long after
boot, by which point it is an ordinary output.

If your board's LED is elsewhere, find it without a rebuild:

```
led               show the current pin
led test 100      hold it at full brightness
led 5 al          move to GPIO5, active low
led auto          resume the visualiser
```

The real SF_01 board has no plain LED — it has a WS2812B on `NEOPIXEL_DIN`
(GPIO33, via R14). Driving that is not implemented.

## Hardware notes

**Mute is not fail-safe at boot.** The 100 kΩ *series* resistors
(R30/R39/R50/R60/R76) are correct and required — TI note (2) on the SLOS708G
abs-max table calls for exactly 100 kΩ when the driver exceeds the 10 V/ms
slew limit on MUTE/SDZ, which any GPIO does. The issue is the five 100 kΩ
*pull-downs* on the `DRIVER_MUTE` node: they make "un-muted" the default
state during reset and the ROM bootloader window.

To make the amps mute-by-default in hardware, replace one of the pull-downs
with a pull-**up** to 3V3 — or remove several and fit a ~10 kΩ pull-up. An
internal ESP32 pull-up cannot do this: at ~45 kΩ against the existing 20 kΩ
pull-down network it only reaches ≈1.0 V, below the 2 V V_IH.

**PSRAM is deliberately disabled.** The WROVER-E-N16R8 has 8 MB, nothing here
needs it, and leaving it off keeps this binary bootable on a plain WROOM-32
devboard for bring-up.
