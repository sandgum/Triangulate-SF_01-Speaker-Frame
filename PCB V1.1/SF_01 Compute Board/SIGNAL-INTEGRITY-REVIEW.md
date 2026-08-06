# SF_01 Compute Board — Wiring, Signal Integrity & Current Capacity Review

**Date:** 2026-08-05 · **Companion to:** `DESIGN-REVIEW-R3.md`
**Scope:** every net traced schematic→PCB, trace width vs current, differential impedance from the stackup, per-IC datasheet checks.
**Load assumptions (your figures):** 6 × 30 W RMS woofers into 3.95 Ω · 4 × 10 W RMS tweeters into 6 Ω · 220 W RMS total.

---

## Headline

**Your HDMI is correct only by accident.** The trace geometry hits 100 Ω on the stackup JLCPCB will actually build, and misses it by 31 % on the stackup declared in your project file. They are two different stackups, and nothing in the file says which one you want.

**USB D+/D− is wrong on both.**

Everything else in the wiring is sound. Three amplifier output traces need widening, and the VSYS inner-layer routing depends on a copper weight you haven't specified.

---

## 1. Differential impedance — measured, not assumed

I measured the routed geometry directly from the PCB rather than trusting the netclass.

**Measured:** all four TMDS pairs and USB are `w = 0.2 mm`, on **F.Cu only, zero vias**, referenced to the solid In1.Cu ground plane.

| Pair | width | centre-to-centre | edge gap | dominant parallel run |
|---|---|---|---|---|
| DATA0 | 0.2 mm | 0.327 mm | **0.127 mm** | 23.9 mm |
| DATA1 | 0.2 mm | 0.327 mm | **0.127 mm** | 13.3 mm |
| DATA2 | 0.2 mm | 0.400 mm | **0.200 mm** | 9.6 mm |
| DATA3 | 0.2 mm | 0.327 mm | **0.127 mm** | 29.7 mm |
| USB D± | 0.2 mm | 0.450 mm | **0.250 mm** | 29.7 mm |

Now the two candidate stackups (IPC-2141 edge-coupled microstrip, t = 35 µm):

### Stackup A — what your file declares (0.1 mm prepreg, εr 4.5)

| Pair | Z₀ single-ended | Z_diff | target | verdict |
|---|---|---|---|---|
| DATA0/1/3 | 40.1 Ω | **68.8 Ω** | 100 Ω | **−31 %** ✗ |
| DATA2 | 40.1 Ω | **74.6 Ω** | 100 Ω | **−25 %** ✗ |
| USB D± | 40.1 Ω | **76.7 Ω** | 90 Ω | **−15 %** ✗ |

### Stackup B — what you're actually ordering: JLCPCB 4L / 1.6 mm / **1 oz inner**

Confirmed from JLCPCB's published stackup data (`jlcpcb_4L_1.6mm_outer1oz_inner1oz_NOREQ`):
**7628 prepreg 0.203 mm, εr 4.4, inner copper 0.030 mm.**

| Pair | Z₀ single-ended | Z_diff | target | verdict |
|---|---|---|---|---|
| DATA0/1/3 | 66.0 Ω | **97.2 Ω** | 100 Ω ±10 % | ✓ **on target** |
| DATA2 | 66.0 Ω | **107.4 Ω** | 100 Ω ±10 % | ✓ (at the edge) |
| USB D± | 66.0 Ω | **112.6 Ω** | 90 Ω ±15 % | **+25 %** ✗ |

Choosing 1 oz inner barely moves the prepreg (0.2104 → 0.203 mm) so **TMDS stays on target at ~97 Ω**, while inner copper roughly doubles (0.0152 → 0.030 mm). It's the right call on both counts.

### What this means

Your file declares a **0.1 mm prepreg**. Your actual build has **0.203 mm**. The design is correct for what you're ordering and wrong for what it documents.

That matters more than it sounds, because **JLCPCB has a 4-layer 1.6 mm stackup that matches your declared 0.1 mm almost exactly** — `JLC04161H-3313`, at 0.0994 mm prepreg. If you ever order controlled impedance and pick the option that matches your file, **TMDS drops to ~69 Ω and HDMI breaks.**

| JLCPCB 4L/1.6 mm stackup | prepreg | your TMDS |
|---|---|---|
| **7628** (default, incl. 1 oz inner at 0.203 mm) | 0.203–0.2104 mm | **97–98 Ω** ✓ |
| 3313 — *matches your declared 0.1 mm* | 0.0994 mm | **~69 Ω** ✗ |
| 1080 | 0.0764 mm | worse ✗ |

**Actions:**

1. **Correct the stackup in the file to 0.203 mm prepreg / εr 4.4 / 1 oz inner.** Right now the file describes the one build that would break HDMI.
2. **Take the default 7628 stackup.** Do not select 3313 or 1080.
3. **Fix USB.** At `s = 0.25 mm` you need **w ≈ 0.32 mm** for 90 Ω; you have 0.2 mm. USB D+/D− also mixes **0.2 mm and 0.25 mm widths along the same pair** (11 segments each) — an impedance step mid-run. Make it one width.
4. **Make DATA2 match the others** — 0.327 mm centre-to-centre, not 0.4 mm. It's 10 Ω off its siblings.

USB 2.0 high-speed has ~500 ps edges; at ~6.7 ps/mm the critical length is ≈ 37 mm and your run is 60 mm, so the pair is electrically long and the mismatch is real, not academic. That said, a +27 % mismatch over 60 mm will most likely still enumerate and run — treat it as "fix before you respin", not "will not work".

### Length matching — excellent, no action

| Pair | P | N | skew |
|---|---|---|---|
| DATA0 | 70.46 mm | 70.46 mm | **0.00 mm / 0.0 ps** |
| DATA1 | 70.29 mm | 70.29 mm | **0.00 mm / 0.0 ps** |
| DATA2 | 70.28 mm | 70.29 mm | 0.01 mm / 0.1 ps |
| DATA3 | 70.56 mm | 70.58 mm | 0.02 mm / 0.1 ps |
| USB D± | 60.48 mm | 61.70 mm | 1.22 mm / 8.2 ps |

Inter-pair spread across all four TMDS lanes is 0.30 mm (2 ps). Every pair single-layer, zero vias, over an unbroken plane. This is genuinely very good routing.

---

## 2. Amplifier output traces — three necks, real

Post-filter outputs to the JST connectors are fine: **1.0–4.0 mm**, ≤14 °C rise at 2.76 A. No action.

The problem is between the amplifier pins and the filter inductors. Every woofer output carries the full **2.76 A RMS** (30 W into 3.95 Ω), and several have a long **0.4 mm** section in series with the wide parts:

| Net | total | at 0.4 mm | ΔT at 2.76 A, 1 oz outer |
|---|---|---|---|
| `Net-(U19-OUTNR)` | 15.8 mm | **10.85 mm (69 %)** | **≈ 63 °C** |
| `Net-(U18-OUTNR)` | 13.4 mm | **8.90 mm (67 %)** | **≈ 63 °C** |
| `Net-(U18-OUTNL)` | 32.7 mm | 8.11 mm (25 %) | ≈ 63 °C over that section |
| `Net-(U17-OUTPR)` | 19.7 mm | 5.95 mm (30 %) | ≈ 63 °C over that section |
| `Net-(U19-OUTPR)` | 5.8 mm | 4.00 mm (69 %) | ≈ 63 °C over that section |

A 3 mm section doesn't help if there's a 0.4 mm neck in series. IPC-2221 for 2.76 A at 10 °C rise on 1 oz outer wants **≥ 1.22 mm**; at a relaxed 20 °C, **≥ 0.85 mm**.

A short 0.4 mm escape at the pad itself is unavoidable — the HTSSOP pad is 0.41 mm wide. But 4–11 mm of it is not a pad escape, it's unwidened routing. Everything else on these nets is already 1–4 mm, so this reads as an oversight.

**Action:** widen to ≥ 1.2 mm as soon as the trace clears the pin escape. This sits under five Class D amplifiers that are already the hottest thing on the board.

Tweeter outputs at 1.29 A are fine at the widths used.

---

## 3. VSYS current capacity — depends on a number you haven't specified

At sustained full output: 220 W / 0.85 efficiency / 24 V ≈ **10.8 A**.

VSYS copper distribution:

| Layer | total | dominant widths |
|---|---|---|
| In2.Cu | 646.6 mm | **6.0 mm (422 mm)**, 4.0 mm (193 mm) |
| F.Cu | 416.7 mm | 1.5 mm (142 mm), 3.0 mm (94 mm), 4.0 mm (73 mm) |
| B.Cu | 25.4 mm | 4.0 mm |

Temperature rise if a single 6 mm In2.Cu run carried the whole load:

| inner copper | at 8 A | at 10.8 A |
|---|---|---|
| **1 oz (0.030 mm)** — what you're ordering | **51 °C** | 100 °C |
| 0.5 oz (0.0152 mm) — JLCPCB default | **123 °C** | far worse |

**Ordering 1 oz inner was the right call** — it roughly halves the rise and takes the 0.5 oz case (which was genuinely not adequate) off the table.

Two honest caveats on the remaining numbers:
- VSYS is a **mesh**, not one trace — 278 vias tie F.Cu, In2.Cu and B.Cu together, so current splits across layers and the single-trace figure above is pessimistic. I did not model the actual current division.
- 10.8 A is continuous-sine-at-clipping, which is not a real operating condition. Music crest factor (10–12 dB) puts average draw nearer **1–2 A**, where none of this matters at all.

**Verdict:** fine for real use. If you want margin under sustained torture-test conditions, add VSYS copper on F.Cu/B.Cu in parallel with the In2 trunk rather than widening In2 further.

Other rails are comfortable: 3V3 (0.6 A, min 0.2 mm → 6 °C), VBUS (0.5 A, 0.25 mm), 3V3_A and 4V5_A negligible.

---

## 4. Wiring checks against datasheets

**Schematic ↔ PCB connectivity is exact.** I compared name-independent connectivity partitions across 1283 pins — every net's pin membership matches, except C40/C41 pad swap (non-polarized 1 µF, harmless) and U20/U21 thermal pads (expected).

**PCM5102A (U5, U14) — configuration correct.** Both identical and both standard:

| pin | net | meaning |
|---|---|---|
| DEMP (10) | GND | de-emphasis off ✓ |
| FLT (11) | GND | normal latency filter ✓ |
| FMT (16) | GND | **I²S format** ✓ |
| XSMT (17) | DAC_EN | soft-mute under control ✓ |
| DVDD (20) | 3V3 | digital ✓ |
| AVDD/CPVDD (8, 1) | 3V3_A | analog on its own rail ✓ |

Analog and digital supplies correctly split — AVDD/CPVDD on 3V3_A from U11, DVDD on 3V3. That's the right call and it's done properly.

**Carried forward, still open:** SCK (pin 12) goes to `WFR_SCK` / `TWT_SCK` rather than GND. The classic ESP32 can only emit an I²S master clock on GPIO 0/1/3. If firmware can't drive these, **drive both pins low** — PCM5102A then runs its internal PLL from BCK, which is the documented SCK-to-GND mode. A *floating* SCK will not lock. This is firmware-fixable but it must actually be done.

**Amplifier land pattern verified** — 16 pads/side, 0.65 mm pitch, 7.44 mm span, correct CCW numbering.

---

## 5. Surface finish — OSP is a reasonable choice

You said you're ordering OSP. That's fine, and better than the HASL your file still declares.

My earlier ENIG recommendation was about avoiding **HASL's uneven surface** under your 0.5 mm-pitch DSBGA muxes. OSP is **planar**, so it solves that problem too, and it's cheaper than ENIG.

Real caveats, in order of how much they'll bite you:

1. **OSP is consumed by reflow.** Pads that go through the oven come out with degraded coating. Your 12 THT parts (barrel jack, 10 JST headers, UART header) get hand-soldered *after* reflow, onto pads that have already been through it. Usually fine with decent flux — occasionally annoying.
2. **Shelf life.** Roughly 6–12 months sealed, and the clock speeds up once the bag is open. Assemble reasonably promptly. If JLCPCB does your assembly, this is a non-issue.
3. **Rework is harder** — reworking a DSBGA on an OSP pad that's already seen reflow is worse than on ENIG.
4. **No good for probing.** OSP oxidizes, so bare-pad test points read unreliably. You have zero test points, so this costs you nothing now — but it's a reason to add a few and consider ENIG if you ever want to bring this board up on a bench.

For a prototype assembled soon after fab, OSP is the right economic call. Update the stackup's `copper_finish` field from `HAL lead-free` so the file reflects it.

---

## 5a. Revision 2 — after your fixes (19:38 build)

**Gerbers confirmed current.** Verified by content, not timestamp: the new `F_Cu.gtl` contains the 0.32 mm aperture and the new 0.5/0.75 mm widths. (The zip's mtime is 4 s *older* than the `.kicad_pcb` because the Fabrication Toolkit saves the board after exporting — not staleness.)

### USB — overshot the target

Width is now **0.32 mm across 96 %** of both traces. But the **centre-to-centre spacing stayed at 0.45 mm**, so widening the copper shrank the edge gap from 0.25 mm → **0.130 mm**. Tighter coupling pulls differential impedance *down*:

| | width | c-to-c | edge gap | Z_diff | target 90 Ω ±15 % |
|---|---|---|---|---|---|
| before | 0.20 mm | 0.45 mm | 0.250 mm | 112.6 Ω | +25 % ✗ |
| **now** | 0.32 mm | 0.45 mm | **0.130 mm** | **76.3 Ω** | **−15 %**, just outside ✗ |
| **target** | 0.32 mm | **0.57 mm** | 0.250 mm | **87.9 Ω** | ✓ |

It passed through 90 Ω and came out the other side. My earlier note said "w ≈ 0.32 mm **at s = 0.25 mm**" — the gap was part of the spec. **Keep 0.32 mm and open centre-to-centre to 0.57 mm.**

(There's also a 13.4 mm stretch where the pair separates to 1.425 mm c-to-c → 102.8 Ω. Worth tightening for consistency, secondary to the above.)

### Amplifier outputs — improved, not finished

Measuring *contiguous* runs below 0.6 mm (the HTSSOP pad is 0.41 mm, so ≲1.2 mm is an unavoidable pad escape):

| Net | longest narrow run | width | verdict |
|---|---|---|---|
| `Net-(U18-OUTNL)` | **8.58 mm** | 0.40 / 0.50 | still a real neck |
| `Net-(U18-OUTPR)` | 5.41 mm | 0.40 | still a real neck |
| `Net-(U18-OUTNR)` | 4.18 mm | 0.40 | still a real neck |
| `Net-(U17-OUTNR)` | 3.90 mm | 0.40 | still a real neck |
| `Net-(U17-OUTPL)` | 3.79 mm | 0.40 | still a real neck |
| `Net-(U17-OUTPR)` | 3.52 mm | 0.40 | still a real neck |
| `Net-(U19-OUTPR)` | 3.36 mm | 0.40 | still a real neck |
| `Net-(U19-OUTNR)` | 3.30 mm | **0.35** | **narrower than before** |
| `Net-(U19-OUTPL)` | 2.96 mm | 0.40 | still a real neck |
| `Net-(U17-OUTNL)` | 2.27 mm | 0.40 | acceptable |
| `Net-(U18-OUTPL)` | 2.25 mm | 0.40 | acceptable |
| `Net-(U19-OUTNL)` | 1.63 mm | 0.40 | fine |

You widened the middle sections (new 0.5 / 0.75 mm apertures confirm it) but the necks nearest the amplifier pins are largely unchanged, and `U19-OUTNR` picked up a 3.3 mm run at 0.35 mm. A 3–8 mm neck at 0.4 mm still sets the limit for the whole path.

**Target: ≥1.2 mm within ~1 mm of the pad escape.**

---

## 5b. ESP32 strapping pins — verified correct

Checked against **ESP32-WROVER-E Datasheet v2.3, Table 4 (p.13) and Table 6 (p.14)**. This was the highest-risk unverified item — a wrong strap means the module never boots, and neither DRC nor ERC would see it.

| Pin | Datasheet default | Your connection | Resulting level | Verdict |
|---|---|---|---|---|
| GPIO0 (25) | Pull-**up** = 1 | SW3 boot button only | **1** → SPI Boot Mode | ✓ |
| GPIO2 (24) | Pull-**down** = 0 | floating | 0 (Table 6: "any value" when GPIO0=1) | ✓ |
| **MTDI / GPIO12 (14)** | Pull-**down** = 0 | floating | **0 → VDD_SDIO = 3.3 V** | ✓ **critical, correct** |
| MTDO / GPIO15 (23) | Pull-**up** = 1 | TWT_BCK → U5.13 (DAC input, high-Z) | 1 → U0TXD printing on | ✓ |
| GPIO5 (29) | Pull-**up** = 1 | floating | 1 | ✓ |

**MTDI is the one that matters.** Your N16R8 has 3.3 V flash and PSRAM (Table 1). Had GPIO12 been pulled high, the internal LDO would come up at 1.8 V and the module would fail to access flash. It's floating, the internal pull-down wins, VDD_SDIO = 3.3 V. Correct.

MTDO sharing with the DAC bit clock is safe because PCM5102A BCK is a high-impedance input — the internal pull-up still reads 1 at reset.

Pin numbering in your symbol matches Table 3 exactly (pin 4 = SENSOR_VP/GPIO36, 14 = IO12/MTDI, 23 = IO15/MTDO, 29 = IO5).

**Minor:** GPIO0 relies on the internal ~45 kΩ pull-up alone. Espressif's reference designs add an external 10 kΩ. Functional as-is; the external resistor is noise insurance on a boot-critical pin.

**Also noted:** SENSOR_VP (36), SENSOR_VN (39), IO34 and IO35 are all floating and unused. Table 3 confirms these are **input-only** with no internal pull-up/pull-down. Harmless if firmware never reads them — and they're the free pins for the SDZ control the earlier review wanted.

---

## 5c. Datasheet audit of the parts that can actually destroy something

### ⚠ L21 — the buck inductor saturates at full load

**This is the one real "blow something up" finding.** Everything verified from TPS54540 SLVSBX7B:

| Parameter | Datasheet equation | Your parts | Result |
|---|---|---|---|
| f_sw | `92417 / R_T(kΩ)^0.991` | R66 = 205 kΩ | **473 kHz** ✓ |
| V_out | `0.8 × (1 + R75/R74)` | 60.4 k / 11.5 k, V_ref = 0.8 V (p.5) | **5.002 V** ✓ |
| Duty | V_out/V_in | 5.0 / 24 | 0.208 |
| Ripple | `V_out(1−D)/(L·f)` | L = 6.8 µH | **1.23 A p-p** |

The converter design is correct. **The inductor is not.**

Your schematic labels L21 **"6.8uH/9A"**, but the part in the BOM is `VLS6045EX-6R8M` — **3.6 A rated, 4.7 A saturation**. The label and the part disagree by a factor of two and a half.

Now the load. SYS_5V feeds the CM4 on six pins (77/79/81/83/85/87), plus U27→VBUS, U6→HDMI_5V, and both LP5907 LDOs:

| Scenario | I_load | I_peak (= I + ripple/2) | vs 3.6 A rated | vs 4.7 A sat |
|---|---|---|---|---|
| CM4 idle + peripherals | 1.50 A | 2.12 A | ok | ok |
| CM4 typical active | 2.50 A | 3.12 A | ok | ok |
| CM4 2.5 A + VBUS + HDMI | 3.50 A | 4.12 A | **over rated** | ok |
| CM4 3 A + VBUS 0.5 + HDMI 0.5 + LDOs | 4.10 A | **4.72 A** | **over rated** | **SATURATES** |

The TPS54540 is a 5 A part on a 42 V-capable input — the IC is comfortable. The inductor is the limit. When it saturates, inductance collapses, ripple current spikes, and you get current-limit chatter or switch damage. Peak current at full load lands *exactly* on the 4.7 A saturation knee with zero margin.

**Fix:** a 6.8 µH inductor rated ≥ 5 A continuous with ≥ 7 A saturation, in a 6×6 mm or 7×7 mm footprint. Your schematic label already says 9 A — source a part that matches it.

### MPU-6050 (U22) — verified correct

| Pin | Connection | Requirement | Verdict |
|---|---|---|---|
| AD0 (9) | R69 10 k → **GND** | sets I²C address | **0x68** ✓ |
| REGOUT (10) | C72 = **100 nF** → GND | 0.1 µF required | ✓ |
| CPOUT (20) | C71 = **2.2 nF** → GND | 2.2 nF required | ✓ |
| CLKIN (1) | GND | tie off when unused | ✓ |
| FSYNC (11) | GND | tie off when unused | ✓ |
| VDD (13) / VLOGIC (8) | 3V3 / 3V3 | 2.375–3.46 V | ✓ |

Charge-pump and regulator caps are both correct values — a common thing to get wrong, and it's right here. INT (12) remains unconnected, so orientation changes must be polled (carried forward from Rev 2, design choice).

### BME680 (U23) — verified correct, including the mode trap

| Pin | Connection | Meaning | Verdict |
|---|---|---|---|
| **CSB (2)** | R71 10 k → **3V3** | **CSB high = I²C mode** | ✓ **correct** |
| SDO (5) | R70 10 k → 3V3 | address select | **0x77** ✓ |
| SDI (3) / SCK (4) | SDA / SCL | I²C bus | ✓ |
| VDD (8) / VDDIO (6) | 3V3 / 3V3 | | ✓ |

CSB is the trap on this part — floating leaves it in SPI mode and I²C silently never works. You pulled it high. Correct.

**No I²C address collision:** MPU-6050 at 0x68, BME680 at 0x77. Clear.

### L21 replacement — selected and added to both BOMs

**`MWSA0603S-6R8MT` (Sunlord, LCSC C408449)** — 6.8 µH ±20 %, **5.0 A rated / 6.0 A saturation**, 48 mΩ, 7 × 6.6 mm molded, $0.1007, 24 k stock.

| | old (VLS6045EX-6R8M) | new (MWSA0603S-6R8MT) |
|---|---|---|
| Rated current | 3.6 A | **5.0 A** (+39 %) |
| Saturation | 4.7 A | **6.0 A** (+28 %) |
| DCR | 36 mΩ | 48 mΩ |
| Body | 6.0 × 6.0 × 4.5 mm | 7.0 × 6.6 mm |

Against the 4.72 A worst-case peak, saturation margin goes from **0 %** to **27 %**.

**No PCB change required.** The 7.0 × 6.6 mm body needs a 3.50 × 3.30 mm half-extent; the existing `L_6.3x6.3_H3` courtyard allows 3.75 × 3.40 mm, and the nearest neighbour (R13) is 4.38 mm away. It drops into the existing land pattern.

*Caveat:* those pads are generic (1.5 × 2.4 mm at ±2.75 mm) and were never an exact match for the VLS6045 either. Check terminal overlap when you place it — for hand assembly a little overhang is workable, arguably easier to solder.

Alternates, both also 5 A/6 A class: `APH0630T6R8M` (C5349706, $0.061) or `FXL0630-6R8-M` (C167221, $0.085, 125 k stock — **cjiang, the same manufacturer as your other 20 inductors**).

Note: parts of this class top out around 5 A/6 A at 6.8 µH in this size. 7 A saturation would mean going to a 7 × 7 mm or larger body.

### Remaining ICs — audit results

**TPS2041B (U27) ✓ correct.** SLVS514P Table 5-1, SOT-23 column: `EN` pin 4, *"logic low turns on power switch"*. Your symbol declares `~{EN}` — matches. Full pin map matches too (OUT 1, GND 2, OC 3, EN 4, IN 5). OC is an open-drain output correctly pulled up through R12. SW6 drives EN low to enable VBUS.

**LP5907 ×2 (U10, U11) ✓ correct.** IN = SYS_5V, EN tied to IN (permanently enabled — valid), pin 4 NC is right for the MFX variant, OUT to 4V5_A / 3V3_A. Worth noting U10 runs 5.0 V → 4.5 V, i.e. **500 mV of headroom**; LP5907 dropout is ~120 mV typical at 250 mA and the seven muxes draw milliamps, so it's comfortable.

**TPA3116D2 (U17, representative of U17–U21) ✓ correct.** All four bootstrap capacitors are paired to the right output: BSNL–C58–OUTNL, BSPL–C57–OUTPL, BSNR–C56–OUTNR, BSPR–C55–OUTPR. PLIMIT tied to GVDD with 1 µF (power limiting disabled). SDZ and FAULTZ tied together with 100 k to VSYS (auto-recovery). MODSEL and AM0/AM1/AM2 to GND. AVCC plus all four PVCC pins on VSYS. Inputs single-ended with INN AC-grounded through matching 1 µF. Consistent with the Rev 2 Table 1 verification.

**74LVC1G07 (U8, U9) — supply domains correct, U8 topology still questionable.** U8 VCC = SYS_5V (5 V), U9 VCC = 3V3 — both right for their signals. But U8's connections are `A` (pin 2) ← CM4 pin 92 and `Y` (pin 4) → SW1, the power button node. The CM4 drives the buffer *into* the button rather than the button driving the Pi. This is the same "power button appears non-functional as drawn" item from Rev 2 and it remains unresolved — worth comparing against the CM4IO reference schematic.

**RT9742SNGV (U6) — variant confirmed, pin numbering NOT confirmed.** The ordering table (DS9742-10 p.2) confirms `RT9742SNGV` is the **SOT-23-3, 0.5 A, no-discharge** variant, and the SOT-23-3 functional block diagram (p.3) shows only VIN, VOUT and GND — no EN, no FLG. So a 3-pin always-on switch is the correct part, and your 3-pin symbol is right in principle.

**What I could not verify:** which physical pin number is which. The pin-configuration diagram on page 1 is a graphic, text extraction returns only fragments (`GND`, `3`, `2`), and I cannot render PDF pages (no poppler). Your symbol assumes 1 = VIN, 2 = VOUT, 3 = GND. **If VIN and GND are transposed this part dies on power-up**, so it's worth 30 seconds of your eyes on that diagram.

### Not worth checking, and you were right

BSS138 (Q1) is G/S/D in SOT-23 across every manufacturer that makes the part — the generic "SOT-23 MOSFET pinout is ambiguous" warning doesn't apply to a part this standardised. SP3010-04UTG is a symmetric passive TVS array; a channel-mapping error degrades protection, it can't damage anything. I flagged both from a generic rule rather than judgement.

---

## 5d. FINAL VERIFICATION — 20:39 build

**KiCad native DRC: 125 violations, all silkscreen/text warnings. Zero clearance, zero shorting, zero solder-mask-bridge, zero unconnected.** Verified with `kicad-cli --severity-all --schematic-parity`; none of the copper checks are in the ignored list.

### Fixed and confirmed

| Item | Verification |
|---|---|
| **Power button** | SW1.1 → GPIO3 (U1.56), SW1.2 → GND, **R77 10 k pull-up** to 3V3, U8 deleted. **R78 220 Ω** on RUN_PG → TP1 (the datasheet's specified reset resistor), TP2 on GLOBAL_EN for force-off. Exactly right. |
| **Mux phantom pads** | Converted to **F.Paste-only, 0.3 mm**, no copper/mask. DRC noise gone *and* the DSBGA now has paste apertures it never had. |
| **USB D±** | 0.32 mm at 0.649 mm c-to-c, 0.329 mm gap → **92.7 Ω** over the 28.8 mm run (target 90 ±15 %). In spec. |
| **C100/C101/C129** | Now `10uF/50V` in the schematic. |
| **Gerbers** | Current — verified by content: 0.32 mm copper aperture present, 0.3 mm paste aperture present, paste flashes 1211 → 1254 (+43, matching the new mux apertures). |

### Still open

**1. Amplifier output necks — unchanged.** Nine woofer outputs still have a contiguous sub-0.6 mm run carrying 2.76 A:

| Net | run | width | ΔT |
|---|---|---|---|
| `Net-(U18-OUTNL)` | 8.58 mm | 0.40 | ~62 °C |
| `Net-(U18-OUTPR)` | 5.41 mm | 0.40 | ~62 °C |
| `Net-(U18-OUTNR)` | 4.18 mm | 0.40 | ~62 °C |
| `Net-(U17-OUTNR)` | 3.90 mm | 0.40 | ~62 °C |
| `Net-(U17-OUTPL)` | 3.79 mm | 0.40 | ~62 °C |
| `Net-(U17-OUTPR)` | 3.52 mm | 0.40 | ~62 °C |
| `Net-(U19-OUTPR)` | 3.36 mm | 0.40 | ~62 °C |
| `Net-(U19-OUTNR)` | 3.30 mm | **0.35** | **~78 °C** |
| `Net-(U19-OUTPL)` | 2.96 mm | 0.40 | ~62 °C |

**2. Stackup still declares 0.1 mm prepreg / εr 4.5 / `HAL lead-free`.** Documentation only — but JLCPCB's `JLC04161H-3313` option *matches* 0.1 mm and would put TMDS at ~69 Ω. **At order time take the default 7628 stackup with 1 oz inner** (0.203 mm prepreg → 97 Ω).

**3. L21 value string still reads `6.8uH/9A`.** The BOM correctly specifies MWSA0603S-6R8MT; only the schematic label is stale — and that label is what caused the original mismatch.

**4. Duplicate via** still at (133.7, 134.6) — one `holes_co_located` warning.

**5. Mux paste aperture sizing — worth a look before you cut a stencil.**

| | value |
|---|---|
| Pad (0.265 mm circle) | 0.0552 mm² |
| Aperture (0.30 mm square) | 0.0900 mm² → **1.63× (63 % over-print)** |
| Web between apertures | **0.20 mm** (industry minimum ~0.25 mm) |
| Area ratio @ 0.1 mm stencil | 0.75 ✓ |
| Area ratio @ 0.12 mm stencil | 0.62 (marginal) |

On a balled DSBGA the package already carries its own solder, so 63 % extra paste at 0.5 mm pitch is a bridging risk on a joint you cannot inspect. **0.27 mm square** would give 1.32× over-print, a 0.23 mm web and AR 0.67 at 0.1 mm — a better balance. Specify a **0.1 mm stencil**, not 0.12 mm.

**6. Six schematic-parity items — all benign, unchanged:** U20/U21 pad 33 (thermal pads, no schematic pin) and C40/C41 pad swap (non-polarised 1 µF).

---

## 5e. SIGN-OFF — 21:09 build

**KiCad native DRC: 0 errors.** 124 violations, every one silkscreen or text. 0 unconnected items. 6 schematic-parity items, all previously dispositioned as benign (U20/U21 thermal pads, C40/C41 non-polarised swap).

| Check | Result |
|---|---|
| DRC errors | **0** |
| Unconnected | **0** |
| Mux paste apertures | **42/42**, 6 per mux — 0.26 mm, over-print 1.22×, web 0.240 mm, **AR 0.649 @ 0.1 mm stencil** |
| USB D± | 92.7 Ω (target 90 ±15 %) |
| TMDS | ~97 Ω on the 7628 / 1 oz-inner build |
| Duplicate vias | 0 (1945 total) |
| Gerber package | 11 layers + both drill files, 1973 PTH hits |
| Power button | SW1→GPIO3/GND, R77 10 k pull-up, R78 220 Ω reset to TP1, TP2 on GLOBAL_EN |

### Correction to earlier revisions of this document

Every "≈62 °C" figure I quoted for the amplifier output necks came from **IPC-2221**, which is derived for a trace in still air with no adjacent copper plane. This board is 4-layer with a solid ground plane 0.203 mm below F.Cu. Recomputed from first principles (p = I²ρ/wt ≈ 10.9 W/m, conductance to plane ≈ 1.19 W/m·K) the rise is **≈9 K**, and IPC-2152 — which unlike 2221 accounts for planes — lands in the same range.

**Realistic rise on the remaining necks is 10–20 °C, not 62 °C.** They are a workmanship item for a future revision, not a thermal hazard. Eight nets retain a 3–5 mm run at 0.40 mm (one at 0.35 mm); `U18-OUTNL` was improved from 8.58 mm to 4.80 mm.

### Accepted / carried forward

- Amplifier output necks (above) — accepted
- `L21` value string still reads `6.8uH/9A`; the BOM correctly specifies MWSA0603S-6R8MT
- Stackup declares 0.1 mm prepreg / εr 4.5 / `HAL lead-free` — documentation only, superseded by the order settings below
- 6 benign parity items
- SS-001 MPN coverage gate — not applicable, parts are ordered by hand from `BoM.csv`

### Order settings

| Field | Value |
|---|---|
| Layers / thickness | 4 / 1.6 mm |
| Outer copper | 1 oz |
| **Inner copper** | **1 oz** |
| **Impedance control** | **No** — the default build is 7628 at 0.203 mm, which is what puts TMDS at 97 Ω |
| Surface finish | OSP |
| Via covering | Tented |
| Stencil | Yes, **0.1 mm** |

Do **not** enable impedance control and select `JLC04161H-3313` — it matches the stale 0.1 mm in the file and would put TMDS at ~69 Ω.

---

## 6. What I could not verify

**Correction to Revision 1 of this document:** I previously wrote that the datasheets were "image-only scans" because `pdftotext` returned nothing. **That was wrong** — `pdftotext` was never installed, and I suppressed the error. The PDFs are fully text-based (the PCM5102A has 504 embedded font objects). Resolved by installing `pypdf`; datasheet extraction now works, and the ESP32 verification in §5b is cited directly from the PDF.

**Still outstanding:**

| Ref | Part | Status |
|---|---|---|
| Q1 | BSS138 | **datasheet still missing** — SOT-23 pinout variant (GDS vs GSD) unverified |
| U22 | MPU-6050 | present, not yet audited |
| U23 | BME680 | present, not yet audited |
| U6 | RT9742SNGV | present, not yet audited |
| CR1 | SP3010-04UTG | present, not yet audited |
| CR2 | STPS30M60DJF | present, not yet audited |

**Also not done:** SPICE (no simulator installed), and I did not model VSYS current division across layers.

---

## Priority list

| # | Item | Severity |
|---|---|---|
| 1 | **Take the default 7628 stackup — never 3313 or 1080.** 3313 matches your declared 0.1 mm and would drop TMDS to ~69 Ω | **High** |
| 2 | Correct the declared stackup to **0.203 mm prepreg / εr 4.4 / 1 oz inner** so the file stops describing the one build that breaks HDMI | **High** |
| 3 | Widen the five 0.4 mm amplifier-output necks to ≥ 1.2 mm | **Medium** |
| 4 | Fix USB D± — single width, ~0.32 mm for 90 Ω | **Medium** |
| 5 | Make DATA2 spacing match the other three pairs | **Low** |
| 6 | Update `copper_finish` to OSP | **Low** |
| 7 | Drive PCM5102A SCK low in firmware if the ESP32 can't emit MCLK | **Firmware** |
| — | ~~Specify 1 oz inner copper~~ | **Done — you're ordering it** |
