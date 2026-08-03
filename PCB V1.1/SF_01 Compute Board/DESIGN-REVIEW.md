# SF_01 Compute Board — Pre-Fabrication Design Review (Revision 2)

**Project:** SF_01 Compute Board — KiCad 10.0, 6 hierarchical sheets, 4-layer, 128 × 128 mm
**Date:** 2026-08-03 (supersedes the 2026-08-02 review)
**Analyzers run:** `analyze_schematic.py`, `analyze_pcb.py --full --proximity`, `cross_analysis.py`, `analyze_emc.py`, `analyze_thermal.py`, `analyze_gerbers.py`
**Not run:** SPICE (no simulator installed), lifecycle audit (no distributor API keys), native KiCad ERC/DRC (`kicad-cli` not on PATH)
**Datasheets read directly:** TPA3116D2/TPA3130D2 (SLOS708G), TPS54540 (SLVSC24), TS5A3159A (SCDS200F), TPS2041B (SLVS514P), LM1085 (SNVS038H), PCM5102A (SLAS859C), WS2812B, SN74LVC1G07

---

## Verdict

**Electrically ready to order.** Every blocker from the previous review is resolved, and one of them was my error. What remains is a short list of robustness improvements and two things to do at order time. Nothing here will stop the board working.

---

## Correction to the Previous Review

**Blocker B2 ("TPA3116D2 footprint missing its PowerPAD") was wrong.** I raised it as CRITICAL twice. It is not a defect.

The TPA3116D2 in the **DAD** package has its PowerPAD on the **top** of the package, for an external heatsink — not on the bottom for a PCB land. SLOS708G states it directly:

> "the TPA3116D2 does 2 × 50 W / 4 Ω with a small heat-sink attached to its **top side PowerPAD**"

§10.3 then specifies the EVM heatsink (ATS-TI 10 OP-521-C1-R1, a 14 × 25 × 50 mm extruded aluminium part) and notes it "has shown to be sufficient for continuous output power."

The thermal table confirms the split:

| Device | Package | θJA | Pad location |
|---|---|---|---|
| TPA3116D2 (U17–U19) | **DAD** | 14 °C/W | **Top side** — heatsink |
| TPA3130D2 (U20, U21) | **DAP** | 36 °C/W | Bottom side — PCB land |

So U17/U18/U19 having 32 pads and no bottom thermal land is **correct**, and U20/U21 having 33 pads with the extra one on GND is **also correct**. Both footprints are right. My apologies for the noise — the asymmetry between the two looked like a library sync error, and I should have checked the package variant before calling it.

---

## Previous Review Delta

| Status | Count |
|--------|-------|
| Fixed | 13 |
| Withdrawn (my error) | 1 |
| Still open | 6 |
| New this revision | 5 |

### Fixed

| Ref | Issue | Resolution — verified |
|---|---|---|
| B1 | LM1085 linear reg, 24 V → 5 V, needed 19 W in a 2.5 W package | Replaced by **U4 TPS54540** buck. FB 60.4 k/11.5 k × 0.8 V = **5.002 V**; RT 205 k → ~473 kHz; Type-II comp (10.7 k + 7.5 nF, 6.8 pF); 100 nF boot; CR2 catch diode correctly oriented (pads 1–4 GND anode, 5–8 SW cathode); thermal pad to GND |
| B3 | Mux on 3.3 V while its signal sits at 3.0 V DC | New **4V5_A** rail from U10 (LP5907-4.5). Headroom is now 1.5 V against the 1.27 V needed at 26 dB gain — the amplifier clips first, by 1.4 dB. Critically, **3V3_A was correctly split off** so the PCM5102As (3.46 V max) stay on their own rail |
| B4 | 220 µF **6.3 V tantalums** on the 24 V rail | BOM now specifies C47023115, 220 µF **35 V** aluminium electrolytic, D6.3 × 7.7 mm — fits the existing footprint |
| H1 | USB-C had sink Rd while sourcing VBUS | R7/R17 (56 k) added as Rp to VBUS; R3/R65 (Rd) lifted onto Q1's drain; **Q1 BSS138** gate on `USB_OTG_ID`, source GND. Verified: Rd grounded only in device mode, removed in host mode |
| H3 | Five 100 k resistors bridging VSYS to 3V3 | Now 100 k pull-ups from VSYS to SDZ, with SDZ tied to FAULTZ — TI's documented auto-recovery configuration (§7.3.9) |
| H4 | 33 V TVS on the 5 V HDMI rail; VSYS unprotected | D1 is now **SMAJ26A on VSYS**, and after the orientation fix, pin 1 (cathode) → VSYS, pin 2 (anode) → GND. Correct |
| H5 | MUTE driven by GPIO, exceeding the 10 V/ms slew limit | 100 k series on every MUTE pin (R30/R39/R50/R60/R76) plus 100 k pull-downs on the `DRIVER_MUTE` node |
| H7 | 25 V filter caps on nodes idling at 12 V | BOM now 50 V (C694504) |
| H8 | Tweeter inductors rated 1.2 A against 1.37 A RMS | C602032, 2.35 A rated / 3.5 A saturation |
| H11 | Min spacing 0.092 mm, below fab minimum | Now **0.127 mm**; DFM tier improved from "challenging" to **standard** |
| M12 | SYNC low-passed at 15.9 kHz, blocking the 400 kHz clock | 1 nF caps deleted, 10 k → **100 Ω**. Clean star from U17 to four slaves |
| M18 | MPU-6050 CLKIN and FSYNC floating | Both tied to GND |
| M21 | `WOOFER_MUTE`/`TWEETER_MUTE` aliasing one net | Single `DRIVER_MUTE` net |

### Accepted by the designer (no action)

WS2812Bs on 3.3 V (indication only), J1 land pattern, JST XH 3 A rating, barrel jack rating, no VSYS fuse, CM4 part-number mismatch.

---

## Critical Findings

**None.** No issue in this revision will prevent the board from functioning.

The list below is ordered by what I'd actually spend time on.

| # | Severity | Issue |
|---|---|---|
| 1 | WARNING | U4 `EN` floating — no programmable UVLO on the buck |
| 2 | WARNING | CR2 has zero thermal vias while dissipating ~1.1 W |
| 3 | WARNING | 3V3 rail load likely exceeds the CM4's 3.3 V output capability |
| 4 | SUGGESTION | U4 has 6 thermal vias against a 9 minimum |
| 5 | SUGGESTION | No local HF bypass at U4's VIN pin |
| 6 | SUGGESTION | Two courtyard overlaps between D6.3 electrolytics and 0603s |
| 7 | ORDER-TIME | Gerbers are stale; surface finish should be ENIG |

---

## 1 — U4 EN pin is floating (WARNING)

`U4.3 (EN)` is a single-pin net. Per SLVSC24, an internal pull-up current source means a floating EN **enables** the device, so it will start — but at the default input threshold of ~4.3 V.

Consequence: on power-up the converter begins switching while VIN is barely above 4 V, at near-maximum duty cycle, into 94 µF of output capacitance plus the CM4. TI's recommended practice is a two-resistor divider from VIN to EN setting UVLO for the actual input range — around 17–18 V for a 24 V design. A floating high-impedance pin adjacent to a 24 V switching node is also noise-susceptible; the EN threshold is ~1.2 V.

**Fix:** two resistors from VSYS to EN to GND. This is the single most worthwhile change left.

## 2 — CR2 thermal vias (WARNING)

`TV-001` reports **0 thermal vias** on CR2 against a 9 minimum.

Dissipation: at 3 A output with D = 5/24 = 0.208, the catch diode conducts 79% of each cycle. At ~0.45 V forward drop (a 30 A device run at 10% of rating), **P ≈ 0.79 × 3 × 0.45 ≈ 1.1 W**.

The PowerFLAT 5×6 tab is on pads 5–8, which are the **SW node** — so you can't simply flood it with copper without enlarging the switching node and worsening EMI. The practical approach is a modest via field under the tab into an isolated SW-net pour on In2, plus vias on the GND pads 1–4. At 3 A the part is well within rating; this is about keeping junction temperature sensible, not preventing failure.

## 3 — 3V3 rail budget (WARNING)

Unchanged from the previous review and still worth knowing before bring-up. The 3V3 rail is sourced entirely from the CM4's 3.3 V output (`U1.84/86`), rated 300 mA per pin / **600 mA total**.

| Load | Estimate |
|---|---|
| ESP32-WROVER-E | 240 mA avg, **500 mA peak** on Wi-Fi TX |
| 4 × WS2812B | up to 240 mA at full white |
| 2 × PCM5102A DVDD | ~20 mA |
| MPU-6050 + BME680 | ~6 mA |
| LEDs, pull-ups | ~5 mA |

Roughly **500 mA typical**, over 600 mA on peaks. Overloading the CM4's internal regulator can brown out the module. If you see instability during Wi-Fi transmission with the LEDs bright, this is the cause. A dedicated 3.3 V LDO from SYS_5V for the ESP32 would decouple it. (Note the 600 mA figure came from a web source, not the CM4 PDF, which resisted text extraction — worth confirming.)

## 4–5 — Buck layout refinements (SUGGESTION)

**Thermal vias:** U4 has 6 against a 9 minimum. At 24 → 5 V, 3 A the TPS54540 dissipates roughly 1–1.5 W. Adding three or four more vias under the exposed pad is cheap insurance.

**Local bypass:** the nearest cap to U4's VIN is C101, a 10 µF 1210 at **6.36 mm**. There is no small ceramic directly at pin 2. A 100 nF 0603 placed hard against VIN/GND would meaningfully reduce high-frequency ringing on the switch node.

**Hot loop:** measured at **25.3 mm²** using C101 → U4 → CR2, which is right at the ≤25 mm² guideline rather than comfortably inside it. Pulling C101 closer to U4 would improve it. (The analyzer reported 377 mm² — see False Positives.)

## 6 — Courtyard overlaps (SUGGESTION)

| Pair | Overlap |
|---|---|
| C110 (220 µF, D6.3) ↔ C109 (100 nF, 0603) | 1.232 mm² |
| C82 (1 nF, 0603) ↔ C78 (220 µF, D6.3) | 1.279 mm² |

The new electrolytics are D6.3 × 7.7 mm cans. Their bodies encroach on adjacent 0603 courtyards. Since the 0603s are ~0.9 mm tall and the electrolytic body sits above its own seating plane, this is probably assemblable — but it's tight for pick-and-place and rework. Worth nudging apart if you have room.

## 7 — Order-time items

**Re-export the gerbers.** `production/SF_01_Compute_Board.zip` was written at 21:34; the PCB was last modified at 21:35. The package is one minute stale. The archive itself is otherwise complete and correct — I extracted and analysed it: all 11 layers present, both drill files present, layers aligned, 128.0 × 128.0 mm, 1936 vias at 0.4 mm.

**Specify ENIG, not HASL.** The stackup declares `HAL lead-free`. You have seven TS5A3159A in **DSBGA, 0.5 mm pitch, 1.4 × 0.9 mm**. HASL's uneven surface is a poor match for that package.

---

## Component Summary

| Type | Count |
|---|---|
| Capacitors | 187 |
| Resistors | 76 |
| ICs | 28 |
| Inductors | 21 |
| Connectors | 14 |
| LEDs | 6 |
| Switches | 5 |
| Diodes | 3 |
| Transistors | 1 |
| **Total** | **341** |

Nets 372 · Sheets 6 · Unique parts 77 · PCB footprints 342 (341 + one logo graphic)

**Schematic ↔ PCB sync: exact.** Every schematic reference has a footprint and vice versa; the only PCB-side extra is a `G***` graphic with no schematic counterpart.

**Sourcing:** MPN coverage in schematic symbol properties is 1/77, which triggers the `SS-001` pre-fab gate. In practice this is satisfied outside the schematic — `BoM.csv` carries verified LCSC part numbers for all 341 designators. If you want the gate to clear, the MPNs need writing into the symbol properties.

---

## Power Tree

```
J2 barrel jack 24 V
 └── VSYS ── D1 SMAJ26A TVS (Vbr 28.9 V)  [no fuse — accepted]
      ├── U17/U18/U19 TPA3116D2   PVCC/AVCC   2×50 W @ 4 Ω
      ├── U20/U21     TPA3130D2   PVCC/AVCC   2×15 W @ 8 Ω
      ├── 10 × 220 µF/35 V bulk + 100 nF/1 nF/10 nF HF
      ├── 5 × 100 k → SDZ/FAULTZ (auto-recovery pull-ups)
      └── U4 TPS54540 buck, 473 kHz, 6.8 µH, CR2 catch diode
           └── SYS_5V (5.002 V, 2 × 47 µF)
                ├── U1 CM4  +5V ×6 pins
                ├── U27 TPS2041B ──► VBUS (USB-C, host mode only)
                ├── U6  RT9742   ──► HDMI_5V ──► J1.18
                ├── U11 LP5907-3.3 ──► 3V3_A ──► U5/U14 AVDD+CPVDD
                ├── U10 LP5907-4.5 ──► 4V5_A ──► 7 × TS5A3159A V+
                └── U8 74LVC1G07
      
CM4 3.3 V output ──► 3V3 ──► ESP32, WS2812B ×4, DAC DVDD ×2,
                              MPU-6050, BME680, LEDs   [see finding 3]
```

Rail pin counts: VSYS 76 · SYS_5V 32 · 3V3 53 · 3V3_A 17 · 4V5_A 11 · VBUS 12 · HDMI_5V 8

---

## Analyzer Verification

### Amplifier configuration — verified against SLOS708G Table 1

All five devices are consistent and correct:

| Ref | R to GVDD | R to GND | Mode | Gain |
|---|---|---|---|---|
| U17 | 100 k (R18) | 20 k (R19) | **Master** | 26 dB |
| U18 | 47 k (R28) | 75 k (R29) | Slave | 26 dB |
| U19 | 47 k (R37) | 75 k (R38) | Slave | 26 dB |
| U20 | 47 k (R47) | 75 k (R48) | Slave | 26 dB |
| U21 | 47 k (R57) | 75 k (R58) | Slave | 26 dB |

Every divider totals ≥ 100 kΩ as §7.3.5 requires. PLIMIT tied to GVDD on all five with 1 µF — power limiting disabled per Table 3. Bootstrap 220 nF/50 V on all 20 outputs (§7.3.6 asks ≥ 16 V). Input coupling 1 µF against 30 kΩ input impedance = 5.3 Hz corner, explicitly endorsed by Table 2. INN pins AC-grounded through matching 1 µF — correct single-ended configuration per §7.3.7.

### Multiplexer — verified against SCDS200F §5

Ball map confirmed: A1 = NO, A2 = IN, B1 = GND, B2 = V+, C1 = NC, C2 = COM. All seven V+ pins on 4V5_A, all seven IN pins on `ORIENTATION_SWITCH` from ESP32 IO32.

Topology resolves to two coherent states — one giving L/L/mono/mono/R/R across the six woofers, the other an alternating L-R arrangement, with R72/R73 acting as a passive mono summer.

Control levels: VIH is 2.4 V min in both the 3.3 V and 5 V electrical tables, so the ESP32's 3.3 V drive has 0.9 V margin at V+ = 4.5 V.

### High-speed routing — excellent

| Pair | P | N | Delta | Vias |
|---|---|---|---|---|
| TMDS DATA0 | 70.46 mm | 70.46 mm | **0.00 mm** | 0 |
| TMDS DATA1 | 70.29 mm | 70.29 mm | **0.00 mm** | 0 |
| TMDS DATA2 | 70.28 mm | 70.29 mm | **0.01 mm** | 0 |
| TMDS DATA3 | 70.56 mm | 70.58 mm | **0.02 mm** | 0 |
| USB D+/D− | 60.48 mm | 61.70 mm | 1.22 mm | 0 |

Intra-pair matching is essentially perfect and inter-pair spread across all four TMDS lanes is 0.30 mm. Every pair is routed on a single layer with **zero vias**. USB's 1.22 mm delta is well inside the USB 2.0 high-speed budget.

### Ground plane

**In1.Cu carries zero tracks** and holds 15,410 mm² of GND zone fill — it is a solid, unbroken ground plane directly beneath F.Cu across 0.1 mm of prepreg. Since F.Cu carries 4,058 of the 5,077 track segments, the overwhelming majority of signals have an ideal return path. GND totals 1,348 vias.

---

## PCB Layout Analysis

**Board:** 128 × 128 mm, 4 layers, 1.6 mm, HASL lead-free, 343 footprints all on F.Cu, 329 SMD / 12 THT, **routing 100% complete, 0 unrouted**.

**Stackup:** F.Cu 35 µm / 0.1 mm prepreg / In1.Cu 35 µm / 1.24 mm core / In2.Cu 35 µm / 0.1 mm prepreg / B.Cu 35 µm. Note this declares 1 oz on inner layers; most fabs supply 0.5 oz inner as standard on 4-layer 1.6 mm — confirm when ordering if you're relying on inner-layer current capacity.

**Track usage:** F.Cu 4058, B.Cu 561, In2.Cu 458, In1.Cu 0.

**Vias:** 1936, all 0.8/0.4 mm, 0.2 mm annular ring — uniform and well within fab capability.

**VSYS routing:** 1089 mm total, 4–6 mm on the trunk, 278 vias. The sub-1 mm segments are stubs to 100 k resistors and decoupling, not current paths. Well sized for the ~8 A this board can draw.

**DFM:** tier **standard**, min track 0.2 mm, min spacing 0.127 mm, min drill 0.4 mm, min annular ring 0.2 mm. One "violation" — board exceeds 100 × 100 mm, which is a pricing tier note, not a manufacturability problem.

**Edge clearance** (accepted by designer): C6, C7 at 0.10 mm; C17, C19 at 0.15 mm; C115, C119 at 0.17 mm; SW4, U5 at 0.18 mm; J1 courtyard overhanging 2.54 mm (intentional for the right-angle HDMI); L8 by 0.05 mm.

**Not present:** fiducials (0), test points (0/358 nets), mounting holes (the four 2.7 mm holes are CM4 standoffs).

---

## Thermal Analysis

`analyze_thermal.py` assessed only one component and returned a score of 100/100 — it could not derive dissipation for the amplifiers or the buck, so the numbers below are hand-calculated from datasheet θJA figures.

| Device | P_diss (full output) | θJA | Notes |
|---|---|---|---|
| TPA3116D2 ×3 | ~11 W each at 2×50 W/4 Ω | 14 °C/W (DAD) | **Heatsink required** — TI's EVM uses a 14×25×50 mm extrusion on the top PowerPAD and calls it sufficient for continuous output |
| TPA3130D2 ×2 | ~3.3 W each at 2×15 W/8 Ω | 36 °C/W (DAP) | 47 and 42 thermal vias — above the 28 minimum. Datasheet rates 2×15 W with no heatsink on a *single-layer* board, so a 4-layer with this via count is comfortable |
| TPS54540 | ~1–1.5 W | — | 6 thermal vias (9 recommended) |
| CR2 | ~1.1 W | — | **0 thermal vias** |
| U10/U11 LDOs | <40 mW | — | Negligible |

At realistic listening levels the picture is far easier — music's crest factor means average dissipation is a small fraction of these figures. The 11 W per TPA3116D2 is a sustained-sine-at-clipping number.

**Heatsink clearance:** nearest neighbours to U17–U21 are 0603 passives at ~6.2–6.5 mm from each package centre. The HTSSOP-32 body is ~11 × 6.1 mm, so you have roughly 3 mm of clear board around each package edge. Check your chosen heatsink's base footprint against that. Also confirm whether the top PowerPAD is electrically live before using a bare metal heatsink spanning devices — use an insulating thermal interface pad if in doubt.

---

## EMC / Cross-Domain

125 EMC findings, risk score 0. The substantive ones:

**Switching harmonics (SW-001, EE-002):** U4 at 500 kHz puts 117 harmonics in the 30–88 MHz band. Inherent to any buck; mitigation is loop area, which is already at the guideline.

**Input filter (EF-001):** input LC corner at 0.11 MHz gives f_sw/f_c = 4.7×, marginally under the recommended ≥5×. Minor.

**Clock routing (CK-001/2/3):** `AMP_CLK_SYNC` is 172 mm on an outer layer and passes near J12. Since it now actually carries a 400 kHz clock, keep the 100 Ω damping and add ground stitching vias at its 4 layer transitions.

**Decoupling (DC-001):** flagged as too far on U2 and U3 (HDMI ESD arrays) and moderately far on U9, U16, U26.

**ESD grounding (ES-002):** U7 (USB-C ESD array) has no ground via within 3 mm; U2 and U3 have one each. For a TVS, ground inductance is the dominant parasitic — roughly 37.5 V of overshoot per nH during an 8 kV strike. Adding a second via at each is cheap.

**Cross-domain:** `RP-002` reports SYS_5V crossing a VSYS plane gap and `WFR_SCK` crossing a 3V3 gap; `PS-002` reports plane splits on VSYS (195 islands), 3V3 (5), 3V3_A (3) and 4V5_A (2). See False Positives — VSYS is routed as tracks, not a zone.

---

## False Positives / Reviewer Overrides

| Finding | Verdict |
|---|---|
| **B2 — TPA3116D2 "missing thermal pad"** (my previous review) | **Withdrawn.** DAD is a top-side PowerPAD package. Both footprints are correct |
| **SW-003 — "hot loop 377 mm²"** | Substantially wrong. The analyzer picked C104, a bulk electrolytic 84 mm away on the amplifier side. Measured against the actual input ceramic (C101), the loop is **25.3 mm²** |
| **PP-001 ×5 — GVDD "no DC path to a power rail"** | GVDD is an internally generated gate-drive output, correctly bypassed with 1 µF and correctly tied to PLIMIT per Table 3 |
| **VM-001 — `ORIENTATION_SWITCH` 4.5 V/3.3 V crossing** | TS5A3159A VIH is 2.4 V min in both electrical tables; 3.3 V drive has 0.9 V margin |
| **VM-001 — `__unnamed_6` 5 V/3.3 V crossing** | `RUN_PG` into a 74LVC input; the LVC family is 5 V tolerant with VIH = 2.0 V at VCC 4.5–5.5 V |
| **SU-001 ×3 — "adjacent signal layers"** | Based on layer *type* declarations. In1.Cu carries zero tracks and is a solid GND plane; In2 carries only 458 segments. Not a real stackup problem |
| **PS-002 — VSYS "195 islands"** | VSYS is routed as tracks, not a zone, so the connectivity graph counts per-layer runs. Routing is 100% complete |
| **GP-001 ×46, RP-001 ×22** | Over-reported. The actionable subset is stitching vias at layer transitions on `AMP_CLK_SYNC`, `SYS_5V`, `HDMI_5V` and the I²S clocks |
| **GR-001/GR-003 on `production/`** | The extracted gerber folder was deleted; only the zip remains. Analysing the zip contents directly shows a complete, aligned, correct package |
| **NT-001 ×114 on U1** | Unused CM4 GPIOs. Add no-connect flags for clean ERC, but not defects |
| **Schematic "duplicate" refs (U1, SW6, U12…)** | Multi-unit symbols, not annotation errors |
| **thermal.json score 100/100** | Only one component assessed; the analyzer could not derive amplifier or converter dissipation. Disregard — see the hand-calculated table above |

---

## Still Open (previously reported, not addressed)

These are carried forward for completeness; none is a blocker.

- **SDZ has no software control.** Now correctly pulled to VSYS with FAULTZ auto-recovery, but the ESP32 cannot hold SDZ low at power-up as §7.3.9 recommends for avoiding DC-detect faults and pop. A spare GPIO (IO34/IO35/IO5 are free) would do it.
- **PCM5102A SCK on IO26/IO18.** The classic ESP32 can only emit an I²S master clock on GPIO0/1/3. Fixable in firmware — drive both pins low, which is equivalent to TI's SCK-to-GND PLL mode.
- **MPU-6050 INT unconnected.** Orientation changes must be polled. A design choice, but notable given orientation switching is the board's headline feature.
- **GLOBAL_EN has no pull-up**, and the SW1/U8/`RUN_PG` power-button circuit still looks non-functional as drawn — pressing the button appears to do nothing while the Pi is running. I could not confirm the CM4's pin definitions from its datasheet (CID-encoded PDF). Worth comparing against the CM4IO reference schematic.
- **No fiducials, test points or mounting holes.**
- **Inner-layer copper weight** — confirm 1 oz vs the more common 0.5 oz default if you rely on it.

---

## Not Performed / Review Limits

- **SPICE simulation not run** — `ngspice`, `ltspice` and `xyce` are all absent. Filter, Zobel, feedback-divider and compensation values were checked analytically instead.
- **`analyze_thermal.py` produced no useful output** — it assessed one component and could not derive power dissipation. All thermal figures above are hand-calculated from datasheet θJA.
- **Lifecycle/obsolescence audit not performed** — no distributor API credentials, and MPNs live in `BoM.csv` rather than schematic properties, so the script has nothing to read.
- **Native KiCad ERC and DRC not run** — `kicad-cli` is not on PATH. **Run both yourself before ordering.** Expect many false "power pin not driven" ERC warnings from missing PWR_FLAGs and from the CM4 symbol's 3.3 V output being typed as `power_in`.
- **CM4 datasheet could not be text-extracted.** The 300 mA/pin, 600 mA total figure in finding 3 came from a web search, not the PDF.
- **Differential pair detection returned nothing** in the schematic analyzer — the TMDS nets use `DATA0P`/`DATA0N` naming rather than the suffixes it matches. I measured the pairs directly from PCB net lengths instead.
- **Impedance control not verified.** The TMDS pairs are length-matched beautifully, but I cannot confirm the trace geometry hits 100 Ω differential without the fab's stackup. If you specified controlled impedance, confirm with the fab.

---

## Positive Findings

1. **Power architecture is now correct.** The TPS54540 design is textbook — feedback lands on 5.002 V, compensation is proper Type-II, the catch diode is correctly oriented, and the thermal pad is grounded.
2. **HDMI TMDS routing is excellent** — 0.00–0.02 mm intra-pair matching, 0.30 mm inter-pair spread, zero vias, referenced to a solid plane.
3. **In1.Cu is an unbroken ground plane** with no routing on it — the best arrangement available on 4 layers.
4. **All five amplifiers are consistently and correctly configured** for 26 dB with proper master/slave assignment, verified against Table 1.
5. **The USB-C dual-role circuit is genuinely clever** — three components reusing the existing mode switch, and it satisfies Type-C requirements in both directions.
6. **SDZ/FAULTZ auto-recovery** matches TI's documented topology.
7. **Routing is 100% complete** with zero unrouted nets, and the schematic and PCB are exactly in sync.
8. **Fabrication package is complete and aligned** — 11 layers, both drill files, correct dimensions.
9. **DFM improved from "challenging" to "standard"** between revisions.
10. **VSYS power distribution** is properly sized at 4–6 mm with 278 vias.

---

## Ordering Notes

- 4 layers, 128 × 128 mm, 1.6 mm
- **Surface finish: ENIG** (not HASL — you have 0.5 mm-pitch DSBGA)
- Min track 0.2 mm, min spacing 0.127 mm, min drill 0.4 mm — standard capability
- 1936 vias at 0.4 mm; specify tented vias so solder doesn't wick through the 10 via-in-pad instances
- Stencil required — 1211 paste apertures, 0.98 SMD ratio
- Confirm inner-layer copper weight
- **Re-export gerbers first**

**Before you click order:** run ERC and DRC in KiCad, and add the two EN divider resistors if you want the buck to have a proper UVLO.
