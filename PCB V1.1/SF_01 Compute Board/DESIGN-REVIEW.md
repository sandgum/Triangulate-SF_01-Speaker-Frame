# SF_01 Compute Board (PCB V1.1) — Pre-Fabrication Design Review

**Project:** SF_01 Compute Board — KiCad 10.0, 5 hierarchical sheets, 4-layer PCB, 128 × 128 mm
**Date:** 2026-08-02
**Analyzers run:** `analyze_schematic.py`, `analyze_pcb.py --full --proximity`, `cross_analysis.py`, `analyze_emc.py`, `analyze_thermal.py`, `analyze_gerbers.py`
**Not run:** SPICE (no simulator installed), lifecycle audit (no distributor API keys), native KiCad ERC/DRC (`kicad-cli` not on PATH). See *Not Performed / Review Limits*.
**Datasheets used:** TPA3116D2/TPA3130D2 (SLOS708G), TS5A3159A (SCDS200F), TPS2041B (SLVS514P), LM1085 (SNVS038H), PCM5102A, WS2812B, SN74LVC1G07 — downloaded to `datasheets/` and read directly.

## Overview

Raspberry Pi CM4 carrier + 220 W class-D audio board. A 24 V barrel jack (VSYS) feeds five class-D amplifiers (3 × TPA3116D2 for six woofers, 2 × TPA3130D2 for four tweeters) and a linear regulator that makes 5 V for the CM4. An ESP32-WROVER-E drives two PCM5102A I²S DACs and a seven-switch TS5A3159A analog multiplexer that re-routes channels based on orientation. HDMI and USB-C to the CM4, MPU-6050 + BME680 sensors, four WS2812B LEDs.

**Verdict: do not order this revision.** Five blockers will prevent the board from working, and two of them (B1, B4) carry a risk of parts overheating or failing destructively. The audio signal chain, gain/master-slave configuration, and PCB routing are otherwise well executed.

## Critical Findings

| # | Severity | Issue | Section |
|---|----------|-------|---------|
| B1 | CRITICAL | VR1 (LM1085-5.0) drops 24 V → 5 V linearly; needs ~19 W, package allows 2.5 W | [B1](#b1) |
| B2 | CRITICAL | TPA3116D2 footprint has no thermal pad — U17/U18/U19 have no heat path or substrate ground | [B2](#b2) |
| B3 | CRITICAL | Analog mux runs on 3.3 V but the audio it switches sits at 3.0 V DC — clips at ~4 W | [B3](#b3) |
| B4 | CRITICAL | Ten 220 µF **6.3 V** tantalums specified on the 24 V rail | [B4](#b4) |
| B5 | CRITICAL | WS2812B LEDs powered from 3.3 V; datasheet minimum is 3.5 V | [B5](#b5) |
| H1 | WARNING | USB-C has sink CC resistors but sources VBUS — host mode cannot work | [H1](#h1) |
| H2 | WARNING | 3V3 rail load exceeds the CM4's 3.3 V output capability | [H2](#h2) |
| H3 | WARNING | Five 100 kΩ resistors bridge VSYS (24 V) straight to the 3V3 rail | [H3](#h3) |
| H4 | WARNING | 33 V TVS on the 5 V HDMI rail; 24 V input has no protection at all | [H4](#h4) |
| H5 | WARNING | MUTE driven by a GPIO — exceeds the datasheet's 10 V/ms slew limit | [H5](#h5) |
| H6 | WARNING | SDZ hard-tied to 3V3: no shutdown control, amps live during power-up | [H6](#h6) |
| H7 | WARNING | 25 V output-filter caps on nodes that idle at 12 V and swing to 24 V | [H7](#h7) |
| H8 | WARNING | Tweeter output inductors rated 1.2 A; channel needs 1.37 A RMS | [H8](#h8) |
| H9 | WARNING | Speaker connectors rated 3 A; a 50 W/4 Ω channel draws 3.5 A RMS | [H9](#h9) |
| H10 | WARNING | HASL surface finish with a 0.5 mm-pitch WCSP part | [H10](#h10) |
| H11 | WARNING | Default net class clearance is 0.01 mm — DRC is not protecting inner layers | [H11](#h11) |
| M12 | WARNING | SYNC network is a 16 kHz low-pass on a several-hundred-kHz clock | [M12](#m12) |
| M13 | WARNING | Exported gerbers in `production/` predate the last PCB edit | [M13](#m13) |

---

## Your Three Flagged Items — Resolved

**1. TPS2041B (U27) wiring direction.** Your note assumed *IN from connector VBUS, OUT feeds the board*. The schematic is the **opposite**: `U27.5 (IN) ← SYS_5V`, `U27.1 (OUT) → VBUS`. The board **sources** 5 V out the USB-C port.

That direction is intentional and correct — SW6 (DPDT) is a USB host/device mode switch: one pole grounds `USB_OTG_ID` (host) while the other pulls `EN` low to turn the switch on. TPS2041B's enable is active-low, which the symbol correctly names `~{EN}`. Pin numbering matches SLVS514P Table 5-1 (SOT-23: GND=2, OC=3, EN=4, IN=5, OUT=1). **No change needed to the switch itself** — but see [H1](#h1), because the CC resistors contradict it.

**2. LP5907 (U11) EN pin.** `U11.1 (IN)` and `U11.3 (EN)` are both on **SYS_5V**. EN is tied to VIN, so the 3V3_A rail is enabled. ✓ Correct, no change needed.

**3. VSYS / HDMI_5V relationship.** They are separate rails. `SYS_5V → U6 (RT9742 load switch) → HDMI_5V → J1 pin 18`. The board **sources** +5 V into the HDMI connector, which is correct for an HDMI source device. ✓ No change needed.

**Bonus:** the "10KGND" label overlap on the ESP32Wrover sheet is still there. Cosmetic only, as you noted.

---

## Blockers

### <a name="b1"></a>B1 — CRITICAL: VR1 (LM1085-5.0) cannot make 5 V from 24 V

`J2 (barrel jack) → VSYS → VR1.3 (INPUT)`, `VR1.2/VR1.4 (OUTPUT) → SYS_5V`. VSYS is 24 V (confirmed by the amplifier rail, the SMAJ33CA selection, and your JOURNAL: *"more than 200W of power at 24V"*).

SYS_5V feeds the CM4 (`U1` pins 77/79/81/83/85/87), the TPS2041B USB switch, the RT9742 HDMI switch, the LP5907, and U8.

| Quantity | Value | Source |
|---|---|---|
| θJA, TO-263 (KTT) | **40.6 °C/W** | SNVS038H §6.4 |
| T_J max | 125 °C | SNVS038H §6.3 |
| P_max @ 25 °C ambient | (125−25)/40.6 = **2.46 W** | P = (T_J−T_A)/θJA |
| P_max @ 45 °C (inside a frame) | **1.97 W** | |
| Dropout | 24 − 5 = **19 V** | |
| ⇒ Max load current | **129 mA** (104 mA at 45 °C) | 2.46 W / 19 V |

A CM4 alone draws 0.6–1.5 A on its 5 V input. At 1 A this regulator must dissipate **19 W** — roughly 8× what the package can shed even with the 40 thermal vias you placed under it. It will current-limit and thermally shut down; the CM4 will not boot.

Secondary: LM1085-**5.0** has a 25 V absolute-maximum input-to-output differential (SNVS038H §6.1). You're at 19 V, but a 24 V adapter's no-load voltage plus a barrel-jack hot-plug transient can exceed that, and there is no TVS on VSYS ([H4](#h4)).

**Fix:** replace VR1 with a 24 V → 5 V synchronous buck rated ≥3 A (e.g. TPS54540, MP2315/MP1584 class). This is a layout change, not a drop-in.

### <a name="b2"></a>B2 — CRITICAL: the TPA3116D2 footprint is missing its PowerPAD

Verified directly in `SF_01 Compute Board.kicad_pcb`:

| Ref | Part | Pads placed | Pad 33 (thermal) |
|---|---|---|---|
| U17, U18, U19 | TPA3116D2 (2 × 50 W) | **32** | **absent** |
| U20, U21 | TPA3130D2 (2 × 15 W) | 33 | present, 5.37 × 10.6 mm, net GND, 47 / 42 thermal vias |

Both reference the same footprint name, but the two library files differ:

- `PCB Components/TPA3116D2DAD/SOP65P810X115-32N.kicad_mod` → **32 pads, no thermal pad**
- `PCB Components/TPA3130D2DAPR/SOP65P810X120-32N.kicad_mod` → 65 pads (32 signal + thermal pad + embedded via array) ✓

The HTSSOP-32 PowerPAD slug is the device's **only** meaningful heat path and its substrate ground reference. With no land, no mask opening and no paste aperture, the three highest-power devices on the board — 150 W of the claimed 220 W — have nothing to solder to and no thermal path. They will hit the 150 °C thermal-shutdown latch at a few watts of dissipation. The gerber analysis confirms it: `heatsink_apertures: 0`.

**Fix:** add the exposed pad to the TPA3116D2 footprint per TI's land pattern (same 5.37 × 10.6 mm as the TPA3130D2 one you already have), assign it to GND, add a thermal via array, then **Tools → Update Footprints from Library** and re-fill zones.

### <a name="b3"></a>B3 — CRITICAL: the orientation multiplexer will clip at ~4 W

This is the board's headline feature and it has a supply-rail mismatch.

- TPA3116D2/TPA3130D2 input pins are **"biased to 3 Vdc"** (SLOS708G, Input Impedance section).
- TS5A3159A recommended analog signal range is **0 to V+**, and V+ here is `3V3_A` = **3.3 V** (SCDS200F §6.3).

The signal chain is `PCM5102A OUT → 470 Ω → 2.2 nF → 1 µF (C157/C158/C188/C190) → mux node`. The 1 µF is a DC block, so the DC level on every mux node is set by the amplifier input bias: **3.0 V, on a 3.3 V rail. 0.3 V of headroom.**

At 26 dB gain (20 ×), 0.3 V peak of input → ~4.2 Vrms out → **~4.4 W into 4 Ω**, on a channel rated for 50 W. Above that the switch pass-gates turn off and the internal ESD diode to V+ conducts, hard-clipping the positive half-cycle and injecting audio-frequency current into `3V3_A` — which also supplies AVDD/CPVDD on both PCM5102A DACs.

This affects **all ten channels**, not just the switched ones: `WOOFER_2`, `WOOFER_5`, `TWEETER_2` and `TWEETER_3` reach their amplifiers directly, but a switch COM pin still hangs on each of those nets, so its ESD diode clamps them too.

Ron also degrades: at V+ = 3 V the peak on-resistance is 1.6 Ω typ / 2 Ω max, versus 0.8/1.1 Ω at 4.5 V (SCDS200F §6.5–6.6).

**Fix:** move `V+` (pin B2) on all seven switches from `3V3_A` to `SYS_5V`. Max V+ is 5.5 V, so 5 V is fine, and 3.0 V ± 1.9 V then fits comfortably. Confirm VIH in the 5-V electrical table for the ESP32's 3.3 V control drive — the 3.3 V table specifies VIH = 2.4 V, so there should be margin, but check it. Alternative: add a second AC-coupling stage after the switch and re-bias the mux nodes to V+/2.

**Wiring itself is correct** — I verified the ball map against SCDS200F §5 (A1=NO, A2=IN, B1=GND, B2=V+, C1=NC, C2=COM). The topology resolves to: one state gives L / L / mono / mono / R / R across the six woofers, the other gives an alternating L-R arrangement. The 1 kΩ links R72 (W3↔W6) and R73 (W1↔W3) act as a passive mono summer in the first state; in the second state R72 bridges an L node to an R node and will add measurable channel crosstalk. Worth reviewing once B3 is fixed.

### <a name="b4"></a>B4 — CRITICAL: 6.3 V tantalums on a 24 V rail

`BoM.csv` line: **`6TPE220MAZB` — 220 µF ±20 % 6.3 V Tantalum, qty 10.**

The ten 220 µF capacitors (C51, C54, C78, C83, C104, C110, C133, C139, C161, C167) are **all on VSYS (24 V)** — 3.8 × their rated voltage. Tantalums fail short and can ignite. This is a safety issue, not just a reliability one.

Same class of error nearby:
- **C3** (10 µF, on VSYS) — the BoM's 10 µF part is `CL05A106MQ5NUNC`, 6.3 V X5R 0402.
- **C41** ("10 µF Tantalum", on VSYS) — the only tantalum near that value in the BoM is 10 V.
- **C40** (10 µF, on SYS_5V) — 6.3 V part on a 5 V rail; no derating margin and severe DC-bias capacitance loss.

**Fix:** all VSYS bulk must be ≥ 50 V (aluminium electrolytic or 50 V MLCC). Use ≥ 16 V on the 5 V rail.

### <a name="b5"></a>B5 — CRITICAL: WS2812B on 3.3 V

D2, D5, D6, D7 have `VDD → 3V3`. The WS2812B datasheet gives **VDD = +3.5 V to +5.3 V** in Absolute Maximum Ratings, with electrical characteristics specified at VDD = 4.5–5.5 V. At 3.3 V they will be dim, colour-shifted, or dead, and the data-reshaping stage is unreliable.

**Fix:** move VDD to 5 V — and note this then requires a level shifter on `NEOPIXEL_DIN`, because the logic-high threshold becomes 0.7 × 5 V = 3.5 V while the ESP32 only drives 3.3 V. A 74LVC1G07 (already in your BOM) with a pull-up to 5 V works, or use a 3.3 V-native addressable LED.

---

## High-Severity Findings

### <a name="h1"></a>H1 — USB-C: sink CC resistors on a port that sources VBUS

R3 and R65 (5.1 kΩ) are Rd pulldowns on CC1/CC2 — that declares the port a **sink (UFP)**. But SW6 also enables U27 to push 5 V onto VBUS in the "host" position.

- In host mode a DFP must present **Rp** (56 kΩ to 5 V, or 22 kΩ to 3.3 V for default USB current), not Rd. With only Rd fitted, an attached device sees Rd–Rd, no attachment is detected, and orientation/enumeration fails.
- Plug in a charger while in host mode and both sides drive VBUS.
- In device mode U27 is off, but the TPS2041B has an OUT→IN body diode, so an external VBUS back-feeds SYS_5V and back-drives VR1's output.

**Fix:** switch the CC resistors with the mode (needs a 3PDT or a small FET), or drop the VBUS source path and keep the port device-only for `rpiboot`.

### <a name="h2"></a>H2 — 3V3 rail exceeds the CM4's 3.3 V output

The 3V3 rail is sourced from `U1.84/86 (CM4_3.3V OUTPUT)`. The CM4 datasheet rates these at 300 mA per pin, **600 mA total** (figure obtained via web search — the CM4 PDF resisted text extraction, so treat as needing your confirmation).

Load on 3V3: ESP32-WROVER-E (~240 mA average, 500 mA peak during Wi-Fi TX) + 4 × WS2812B (up to 60 mA each = 240 mA) + 2 × PCM5102A DVDD + MPU-6050 + BME680 + 2 LEDs + pull-ups ≈ **500 mA typical, over 600 mA on peaks**. Overloading the CM4's internal regulator can brown out the module itself.

**Fix:** add a dedicated 3.3 V regulator from SYS_5V for the ESP32 and LEDs; leave the CM4's 3.3 V for its own I/O reference.

### <a name="h3"></a>H3 — Five 100 kΩ resistors tie 24 V to the 3.3 V rail

R16, R26, R35, R45, R55: pad 1 = **VSYS**, pad 2 = **3V3** — confirmed in both the schematic netlist and `pcb.json` pad-net data.

These were clearly meant to be SDZ pull-ups to VSYS, but SDZ is already hard-wired to 3V3, so they only inject (24 − 3.3)/100 k × 5 = **1.04 mA from the 24 V rail into the 3.3 V rail**. When VSYS is present but the CM4 is off, the 3V3 rail is pulled up through an effective 20 kΩ from 24 V into the ESD structures of the ESP32, MPU-6050 and BME680 (whose VDD absolute maximum is the lowest of the group).

**Fix:** delete all five.

### <a name="h4"></a>H4 — Protection is on the wrong rail

- **D1 (SMAJ33CA, 33 V standoff / 53.3 V clamp) sits across HDMI_5V to GND.** On a 5 V rail it never conducts until 36 V — it provides no protection.
- **VSYS (24 V, ~8 A) has no TVS, no fuse and no reverse-polarity protection.** TPA3116D2 absolute maximum PVCC is 30 V, recommended maximum 26 V, with a self-clearing over-voltage fault above 27 V (SLOS708G §6.1/§6.3, Table 4).

**Fix:** put a ~26–28 V unidirectional TVS (SMBJ26A/SMBJ28A) plus a fuse on VSYS at the barrel jack, and a 5–6 V TVS on HDMI_5V. Note SMAJ33CA is unsuitable for VSYS too — its 53.3 V clamp is far above the amplifiers' 30 V absolute maximum.

### <a name="h5"></a>H5 — MUTE exceeds the datasheet slew-rate limit

SLOS708G §6.1 Absolute Maximum Ratings: *"Slew rate, maximum — AM0, AM1, AM2, MUTE, SDZ, MODSEL: 10 V/ms"*, footnote (2): *"100 kΩ series resistor is needed if maximum slew rate is exceeded."*

ESP32 IO27 drives all five MUTE pins directly with nanosecond edges — roughly five orders of magnitude past the limit. R20/R27/R36/R46/R56 (100 kΩ) are pull-**downs** to GND, not series elements.

**Fix:** add a 100 kΩ resistor in series with each amplifier's MUTE pin.

### <a name="h6"></a>H6 — SDZ hard-tied to 3V3

All five SDZ pins go straight to the 3V3 rail. Consequences:
- No way to shut the amplifiers down; they idle at ~30 mA each on the 24 V rail whenever the jack is plugged in.
- SLOS708G §7.3.9: *"hold the SD pin low at power-up until the signals at the inputs are stable"* to avoid nuisance DC-detect faults and power-on pop. That can't be done here.

**Fix:** route SDZ to a spare ESP32 GPIO (IO34/IO35/IO5 are free) with a pull-down.

### <a name="h7"></a>H7 — Output filter capacitor voltage rating

BoM: `CL10B684KA8VPNC` — 680 nF **25 V** X7R 0603, for the twenty class-D output filter caps.

MODSEL is tied to GND → **BD modulation**, so each output idles at 50 % duty and the post-LC node sits at PVCC/2 = **12 V DC**, swinging 0–24 V at full output. That is 96 % of the part's rating at peak, and a 0603 X7R at 12 V bias retains only a fraction of its nominal capacitance. Nominal LC corner (10 µH + 680 nF) is 61 kHz; at ~40 % effective capacitance it moves to ~95 kHz, degrading switching-residue attenuation.

**Fix:** 50 V parts, and consider 0805/1206 to reduce DC-bias capacitance loss.

### <a name="h8"></a>H8 — Tweeter output inductors under-rated

BoM `ANR4020T100M`: 10 µH, **1.2 A rated, 1.7 A saturation**. TPA3130D2 at 2 × 15 W into 8 Ω is 1.37 A RMS / ~1.94 A peak per channel — over both limits.

The woofer inductors (`PSPMAA0805-100M-ANP`, 6 A / 10 A sat) are correctly sized. ✓

### <a name="h9"></a>H9 — Connector current ratings

- Speaker connectors J5–J14: `S2B-XH-A` rated **3 A**. A 50 W/4 Ω channel is 3.5 A RMS; even the specified 30 W Peerless driver is 2.7 A RMS.
- Barrel jack `RAPC10U`: verify its rating against the ~8 A your JOURNAL expects at full power.

### <a name="h10"></a>H10 — HASL finish with a 0.5 mm-pitch WCSP

PCB stackup specifies `copper_finish: "HAL lead-free"`. `TS5A3159AYZPR` is DSBGA/WCSP-6, 1.4 × 0.9 mm at 0.5 mm pitch — HASL's uneven surface causes opens and bridges on parts this fine.

**Fix:** order with **ENIG**.

### <a name="h11"></a>H11 — DRC is not protecting your inner layers

`.kicad_pro` Default net class: `clearance: 0.01` mm, and `min_clearance`, `min_track_width`, `min_via_annular_width` are all `0.0`.

Your `SF_01 Compute Board.kicad_dru` has good JLCPCB rules, but they are scoped:
- track↔track: 0.127 mm outer / 0.09 mm inner ✓
- track↔pad: 0.2 mm ✓
- pad/via ↔ pad/via: 0.127 mm — but **`(layer outer)` only**

So **inner-layer via↔via and via↔pad clearances fall back to the 0.01 mm net class value**, on a board with 2000 vias. The PCB analyzer already measured a track spacing of **0.092 mm**, below JLCPCB's 0.1 mm advanced-process minimum.

**Fix:** set the Default net class clearance to ≥ 0.127 mm, re-run DRC, and resolve what it finds before exporting.

---

## Medium-Severity Findings

### <a name="m12"></a>M12 — The SYNC network low-passes the amplifier clock

Gain/master-slave is configured **correctly** (see Positive Findings), so U17 is the master driving `AMP_CLK_SYNC` into four slaves. But each slave's SYNC pin has a 10 kΩ series resistor (R25/R44/R49/R59) plus a 1 nF shunt to GND (C127/C128/C140/C168) — an RC low-pass at **15.9 kHz**. The TPA31xx sync clock is in the hundreds of kHz. The slaves will not lock, and unsynchronised class-D amps sharing a supply produce audible beat-frequency whine.

**Fix:** remove the 1 nF caps and reduce the series resistors to a small damping value (or a direct connection).

### <a name="m13"></a>M13 — The exported fab package is stale

| File | Modified |
|---|---|
| `SF_01 Compute Board.kicad_pcb` | 2026-07-27 **20:56:00** |
| `production/SF_01_Compute_Board.zip` and gerbers | 2026-07-27 **20:45:56** |

The gerbers predate the last PCB edit by 10 minutes. **Re-export** after making the fixes above.

### M14 — PCM5102A SCK is on pins that can't produce an MCLK

`WFR_SCK` = ESP32 IO26, `TWT_SCK` = IO18. The classic ESP32 can only emit an I²S master clock on GPIO0, GPIO1 or GPIO3 (CLK_OUT1–3). Neither DAC will receive an SCK.

**Good news: this is fixable in firmware.** The PCM5102A runs from an internal PLL off BCK when SCK is grounded, so configure IO26 and IO18 as outputs driven permanently low. No board change needed — but confirm it before relying on it.

### M15 — Board-edge clearance violations

| Ref | Distance to edge |
|---|---|
| C6, C7 | 0.10 mm |
| C17, C19 | 0.15 mm |
| C115, C119 | 0.17 mm |
| SW4, U5 | 0.18 mm |
| L8 | courtyard overhangs 0.05 mm |
| J1 | courtyard overhangs 2.54 mm — likely intentional for a right-angle HDMI, confirm |

JLCPCB wants ≥ 0.3 mm copper-to-edge; components want ≥ 1 mm from a routed edge. Also, `F.SilkS` extends to 129.19 × 133.6 mm on a 128 × 128 mm outline — silkscreen hangs off the board and will be clipped.

### M16 — PCM5102A supply domains can be independently absent

`DVDD → 3V3` (from the CM4), `AVDD`/`CPVDD → 3V3_A` (from LP5907 off SYS_5V). The split itself matches TI's typical application (3.3 V + 3.3 VA) ✓, but here 3V3 disappears whenever the CM4 is off while 3V3_A stays up — a sequencing exposure through the DACs' internal ESD diodes. Same issue for the mux: it stays powered from 3V3_A while its control line comes from an unpowered ESP32.

### M17 — BOM part-selection errors

| Ref(s) | Schematic value | BoM part | Problem |
|---|---|---|---|
| R8–R11 | 470 Ω resistor (`R_0603` footprint) | `BLM18PG471SN1D` | That's a 470 Ω @ 100 MHz **ferrite bead**, ~0 Ω at audio. It is the series R in TI's 470 Ω + 2.2 nF PCM5102A output filter — substituting a bead removes the filter and leaves the DAC driving a bare capacitance. |
| R14 | 220 Ω | `MPZ1608S221ATA00` | Also a ferrite bead; won't damp the WS2812B data line. |
| U8, U9 | 74LVC1G07 | qty **0**, no price | Not sourced. |
| U1 | CM4102008 (8 GB) | CM4102016 (16 GB) | Inconsistent between schematic and BoM. |
| J1 | `STEWART_SS-53200-001` footprint | `HDMI 19PIN 043 SHOU HAN` | Different manufacturer — verify the land patterns actually match before ordering. |

Analyzer finding **SS-001** (pre-fab blocker): MPN coverage is **1 / 64 unique parts**. The KiCad symbols carry almost no MPN properties, so the `production/` BOM cannot be used for assembly as-is.

### M18 — Unused pins and missing no-connects

`total_no_connects: 0` and 133 single-pin nets. Most (114) are unused CM4 GPIOs, which is fine — but with no NC flags, ERC output will be unusable. Genuine ones worth addressing:

- **U22 (MPU-6050): CLKIN (pin 1) and FSYNC (pin 11) float.** InvenSense specifies tying unused CLKIN and FSYNC to GND.
- **U22 INT (pin 12) goes nowhere.** `MPU_INT` is a single-pin net — the IMU cannot interrupt the ESP32, so orientation changes must be polled. Given orientation switching is the board's core feature, consider wiring it.
- J15 SBU1/SBU2, J1 pin 14 (Utility), D7 DOUT — all fine to leave floating.
- U15.A1 (NO), U16.C1 (NC), U26.A1 (NO) float by design — those are the unused throws of the mux. ✓

### M19 — ESD and decoupling placement

- **U7 (USB-C ESD array) has no ground via within 3 mm.** For a TVS, ground-path inductance is the dominant parasitic — roughly 37.5 V of overshoot per nH during an 8 kV strike. U2 and U3 have only one ground via each.
- Decoupling flagged as too far from U2, U3 and U9.
- HDMI **HPD (pin 19) has no ESD protection** — CR1 covers SDA, SCL and CEC only.

### M20 — Manufacturing and test

- **No fiducials** on a board with 315 SMD parts including a 0.5 mm-pitch WCSP. Add three on F.Cu.
- **No test points** (0 / 358 nets). At minimum add VSYS, SYS_5V, 3V3, 3V3_A and GND.
- **No board mounting holes.** The four 2.7 mm holes are the CM4 standoffs; the board itself has no mounting provision.
- 14 untented vias in pads, including the thermal pads of U20/U21 — specify tented/plugged vias so solder doesn't wick through and void the thermal joint.
- Board is 128 × 128 mm, above JLCPCB's 100 × 100 mm pricing tier. DFM tier assessed as "challenging".

### M21 — Naming

`WOOFER_MUTE` and `TWEETER_MUTE` are two global labels on the **same** net (analyzer finding LB-001). KiCad exported it to the PCB as `TWEETER_MUTE`. Schematic and PCB agree — this is cosmetic, not a sync bug — but it hides the fact that woofers and tweeters cannot be muted independently.

---

## Positive Findings

1. **Amplifier gain and master/slave configuration is exactly right.** Verified against SLOS708G Table 1: U17 = 100 kΩ to GVDD / 20 kΩ to GND → **Master, 26 dB**; U18/U19/U20/U21 = 47 kΩ to GVDD / 75 kΩ to GND → **Slave, 26 dB**. All dividers total ≥ 100 kΩ as §7.3.5 requires.
2. **PLIMIT tied to GVDD** on all five amps = power limiting disabled, per Table 3. *(The analyzer's five PP-001 "no DC path to a power rail" errors on GVDD are false positives — GVDD is an internally generated output, not a supply input.)*
3. **GVDD decoupling:** 1 µF on all five, matching §7.3.5's "X5R ceramic 1 µF".
4. **Bootstrap capacitors:** 220 nF / 50 V on all twenty outputs — §7.3.6 asks for 220 nF rated ≥ 16 V. ✓
5. **Input coupling:** 1 µF against 30 kΩ input impedance = 5.3 Hz corner. Table 2 explicitly endorses 1 µF where −3 dB at 20 Hz is acceptable.
6. **INN pins AC-grounded through matching 1 µF caps** — the correct single-ended configuration per §7.3.7.
7. **PCM5102A application circuit matches TI's reference**, including the split 3.3 V / 3.3 VA supply arrangement and the 470 Ω + 2.2 nF output network.
8. **Multiplexer wiring is correct** against SCDS200F §5 (A1=NO, A2=IN, B1=GND, B2=V+, C1=NC, C2=COM), and the two orientation states resolve to sensible channel maps.
9. **TPS2041B pinout and active-low enable** match SLVS514P Table 5-1, correctly paired with SW6.
10. **VSYS power routing is well done** — 4 mm and 6 mm trunks carry 96 % of the length; the only sub-1 mm segments are stubs to 100 kΩ resistors and decoupling caps.
11. **Ground:** a single GND zone spans all four copper layers with 1427 stitching vias.
12. **TPA3130D2 thermal pads done properly** — 47 and 42 thermal vias, above the 28 minimum.
13. **Routing is 100 % complete**, 0 unrouted nets, and gerber layers are complete and aligned.
14. Vias are uniform at 0.8 / 0.4 mm with a 0.2 mm annular ring — comfortably within fab capability.

---

## False Positives / Reviewer Overrides

| Finding | Verdict |
|---|---|
| **PP-001 × 5** — "GVDD has no DC path to a power rail" | **False positive.** GVDD is an internally generated gate-drive output, correctly bypassed with 1 µF and correctly shorted to PLIMIT. |
| **VM-001** — 5 V/3.3 V crossing on `__unnamed_7` | **False positive.** That's `RUN_PG` into the 74LVC1G07 input; the LVC family is 5 V tolerant. Worth checking VIH margin, but not a domain-crossing error. |
| **NT-001 × 114** on U1 | Expected — unused CM4 GPIOs. Add no-connect flags for clean ERC, but not defects. |
| **CP-002 × 10, PM-001 × 20** (courtyard overlaps ≤ 0.6 mm²) | Mostly 0603 passives with slightly generous courtyards. Only D1↔C1 (0.615 mm²) and D1↔VR1 (0.484 mm²) are worth a look. |
| **PS-002 "VSYS plane split: 207 islands"** | Artifact — VSYS is routed as tracks, not a zone, so the connectivity graph counts per-layer runs. Routing is complete. |
| **EMC GP-001 × 44, RP-001 × 20** | Real in aggregate but over-reported. The actionable subset is: add stitching vias next to layer transitions on `AMP_CLK_SYNC`, `SYS_5V`, `HDMI_5V` and the I²S clocks, and pull `WFR_SCK`/`TWT_BCK`/`TWT_DIN` back from the board edge (currently 0.06–0.08 mm). |
| **SU-001** "adjacent signal layers" | Genuine but inherent to a 4-layer board where In1/In2 carry routing. The GND zone covers all four layers, which mitigates it. |
| **TWEETER_MUTE vs WOOFER_MUTE** | Checked for a schematic/PCB sync bug — there isn't one. Single net, two aliases. |

---

## Not Performed / Review Limits

- **SPICE simulation not run** — no simulator installed (`ngspice`, `ltspice`, `xyce` all absent). The LC filter, Zobel and input high-pass values were checked analytically instead.
- **`analyze_thermal.py` ran but produced 0 findings (score SKIPPED)** — it could not derive per-component dissipation. The LM1085 and TPA3116 thermal analysis in B1/B2 is hand-calculated from datasheet θJA figures.
- **Lifecycle/obsolescence audit not performed** — no distributor API credentials, and MPN coverage is only 1/64 anyway.
- **Native KiCad ERC and DRC not run** — `kicad-cli` is not on PATH. **You should run both yourself before ordering**, especially DRC after fixing H11. Expect many false "power pin not driven" ERC warnings from missing PWR_FLAGs and from the CM4 symbol's 3.3 V output being typed as `power_in`.
- **CM4 datasheet could not be text-extracted** (CID-encoded PDF). The 300 mA/pin, 600 mA total figure in H2 came from a web search rather than direct datasheet reading — please confirm it against the PDF.
- **`RUN_PG` / `GLOBAL_EN` power-button circuit not fully verified.** SW1 connects GLOBAL_EN to U8's open-drain output, which is gated by RUN_PG. As drawn, pressing the button appears to do nothing while the Pi is running (output floating) and to hold GLOBAL_EN low while it's off. GLOBAL_EN also has no external pull-up. I could not confirm the CM4's pin definitions from the datasheet — **verify this circuit against the CM4IO reference schematic before fab.**
- **Bootstrap and output-filter capacitor voltage ratings for parts not listed in `BoM.csv`** could not be checked.
- **No prior design review found** in the project, so no delta section.
- **PCB V1 not compared** against V1.1.

---

## Suggested Order of Work

1. **B1** — swap VR1 for a buck converter (drives the biggest layout change; do it first).
2. **B2** — fix the TPA3116D2 footprint, update from library, re-fill zones.
3. **B3** — move the mux V+ to SYS_5V.
4. **B4** — re-spec all VSYS bulk capacitors to ≥ 50 V.
5. **B5** — WS2812B to 5 V plus a level shifter.
6. **H1–H11** — CC resistors, 3V3 regulator, delete R16/R26/R35/R45/R55, fix protection, MUTE series resistors, SDZ control, 50 V filter caps, tweeter inductors, connectors, ENIG.
7. **M12** — remove the SYNC low-pass caps.
8. **H11 then DRC** — raise the default clearance to 0.127 mm and run DRC to completion.
9. Add fiducials, test points and mounting holes; populate MPNs; re-export gerbers (**M13**).
