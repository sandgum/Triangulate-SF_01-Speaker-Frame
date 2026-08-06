# SF_01 Compute Board — Pre-Fabrication Review (Revision 3)

**Date:** 2026-08-05 · **Supersedes:** `DESIGN-REVIEW.md` (Rev 2, 2026-08-03)
**Board:** KiCad 10, 6 hierarchical sheets, 4-layer, 128 × 128 mm, 341 components / 343 footprints

**What's new in this revision:** Rev 2 could not run KiCad's native ERC and DRC (`kicad-cli` was not on PATH). It is available at `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli`, so **both ran for the first time**. That closes the single largest gap in the previous review.

---

## Verdict

**One real electrical defect, one ordering blocker, three cheap cleanups.** Nothing in the layout will stop the board working.

The headline: most of what DRC screams about is not real. Of 370 violations, **6** are genuine geometry and **1** is an actual electrical derating error found in the schematic, not the layout.

---

## Corrections to my own findings during this review

I raised two "critical shorts" during the session and then disproved both. Recording them so they don't get re-raised later.

| Claim | Verdict |
|---|---|
| "SW-node vias are shorted to the GND plane in the shipped gerbers" | **Withdrawn.** The stale fill was real *in the `.kicad_pcb`*, but the Fabrication Toolkit has `AUTO FILL: true` and refills at export. I checked the shipped `In1_Cu`/`In2_Cu` gerbers directly: a proper clearance ring exists at **0.601–0.606 mm** radius around each SW via (0.4 via radius + 0.2 clearance). The artwork was correct. |
| "RUN_PG is shorted to the GND plane" | **Withdrawn.** The three flagged segments were created in the 18:35 reroute, *after* the last zone fill — they do not exist in the 18:17 backup that produced the gerbers. A refill clears them. KiCad showing nothing wrong on screen was correct. |

The lesson worth keeping: **a zone-clearance error reading exactly `0.0000 mm` is almost always a stale fill, not a short.** Refill before believing it.

The one genuine short — a VSYS track overlapping a GND via by 0.106 mm at (185.8, 143.8) — was found and removed during this session.

---

## Blockers

### 1. C100, C101, C129 are 25 V parts on the 24 V rail — REAL

The schematic value is `10uF/25V`. All three sit on **VSYS**, which is the 24 V input rail.

This is the leftover half of a fix that was only half-applied. Your own README already flags it:

> "Was 10uF/25V 0603 on the 24V rail. 10uF/50V does not exist in 0603 — the footprint has to go to 1210"

The **footprint** moved to 1210. The **value string never did.** Consequences:

- **No derating margin.** 24 V nominal on a 25 V part is 96 % of rating. Standard practice is 2×.
- **Transients exceed the rating outright.** D1 is an SMAJ26A: V_br 28.9 V, V_c 42.1 V. Anything the TVS lets through is above 25 V by design.
- **DC bias collapse.** A 25 V X7R 1210 at 24 V typically loses 60–80 % of its rated capacitance. You are not getting 10 µF; you may be getting 2–4 µF.

**The parts you actually costed are correct** — `GRM32ER71H106KA12L`, 10 µF **50 V** X7R 1210 (LCSC C77102). Only the schematic string is stale, and `production/SF_01_Compute_Board_bom.csv` currently exports `10uF/25V`.

**Fix:** change the value on C100/C101/C129 to `10uF/50V` before regenerating the BOM.

### 2. Production BOM has no LCSC part numbers

Every row of `production/SF_01_Compute_Board_bom.csv` has an **empty `LCSC Part #` column**:

```
Designator,Footprint,Quantity,Value,LCSC Part #
"C1, C106, ... C9",0603,34,100nF,
"C10, C11, C35, C36, C71",0603,5,2.2nF,
```

Assembly cannot be sourced from this file. Bare-board fab is unaffected.

The data exists — `BoM.csv` at the repo root has verified LCSC numbers for all 341 designators. It just isn't in the symbol properties, which is also what trips the `SS-001` gate (MPN coverage 1/77 unique parts).

**Fix:** write the LCSC numbers into the schematic symbol properties, or hand-merge them into the production CSV before uploading.

---

## Real but minor — cheap to fix

### 3. U8 pad 1 clearance — 0.123 mm

A GND track passes **0.123 mm** from U8 pad 1. Two DRC errors, at (188.56, 144.35).

This one is genuine geometry, not fill-dependent. It is below your 0.2 mm rule *and* below the 0.127 mm minimum spacing your DFM tier claims — so it may draw a fab query.

Electrical risk is low: U8 pad 1 is the **NC** pin on the SN74LVC1G07 (1=NC, 2=A, 3=GND, 4=Y, 5=VCC). Even a bridge does nothing. Treat as yield hygiene.

### 4. Duplicate co-located vias

Two GND vias occupy the exact same coordinate, **(133.7, 134.6)**. The drill file will contain two hits at one location. Most fabs handle it; some flag it. Delete one.

### 5. The mux footprints carry 42 phantom pads — this is why DRC is unusable

Each of the seven `TS5A3159A` footprints (U12, U13, U15, U16, U24, U25, U26) contains **12 pads where it should contain 6**:

```
pad ''   at (-0.25,-0.5)  <no net>      pad 'A1' at (-0.25,-0.5)  TWEETER_1
pad ''   at (-0.25, 0  )  <no net>      pad 'B1' at (-0.25, 0  )  GND
pad ''   at (-0.25, 0.5)  <no net>      pad 'C1' at (-0.25, 0.5)  TWEETER_4
...                                      ... (etc.)
```

Six unnamed, netless duplicates sit exactly on top of the six real balls — same position, same 0.265 mm size, same `F.Cu`/`F.Mask` layers.

**The artwork is unaffected.** Identical geometry drawn twice produces identical copper and identical mask. Your schematic is wired correctly and every real ball carries the right net — you were right about that.

**But they cost you your DRC.** They generate:

| Error class | Total | Phantom (mux) | Real |
|---|---|---|---|
| `solder_mask_bridge` | 101 | 101 | 0 |
| `shorting_items` | 57 | 57 | 0 |
| `clearance` | 53 | 48 | 5 |

**~206 of 370 violations are noise from this one defect.** A real short hiding in that list would be invisible — which is exactly what happened with the VSYS/GND via earlier today.

**Fix:** delete the six unnamed pads from the footprint (or re-import a clean `Texas_DSBGA-6_0.9x1.4mm_Layout2x3_P0.5mm`), then re-run DRC and read it properly.

---

## Verified correct — checked, no action

**Schematic ↔ PCB cross-reference is clean.** I compared connectivity *partitions* (name-independent — comparing net names alone produces 356 false mismatches because the analyzer calls anonymous nets `__unnamed_N` while KiCad calls them `Net-(U14-CAPP)`). Across 1283 pins the only differences are:

- **C40 / C41 pads swapped** vs the schematic. Both are 1 µF 0603 ceramics — non-polarized, electrically identical either way. Cosmetic desync only.
- **U20 / U21 pad 33** has no schematic pin. Expected: it's the thermal pad.

Everything else maps exactly.

**Amplifier land pattern verified.** U17–U21 on `SOP65P810X115-32N`: 16 pads per side, 0.65 mm row pitch, 7.44 mm lead span, 1.57 × 0.41 mm pads, correct counter-clockwise numbering (1–16 down the left, 17–32 up the right). Textbook HTSSOP-32.

**U20/U21 thermal pad present and grounded** — 5.372 × 10.600 mm, pad 33 on GND, 0.249 mm clear of the signal pads. Note the footprint is *named* `TPA3116D2DAD` (DAD) but carries the extra pad, so it is functioning as a DAP land pattern. Worth confirming pad 33's dimensions against the DAP land pattern in SLOS708G §11 if you want certainty — I could not text-extract that page.

**C119–C122 are correctly grounded.** The kicad-happy analyzer reported their centre node floating; it is wrong. There is a GND symbol at (220.98, 43.18) on `audioCircuit.kicad_sch`, sitting exactly at the midpoint between C119 (x=213.36) and C120 (x=227.33). KiCad's own parity check agrees. False positive.

**ERC's 131 `pin_not_connected` are not defects** — 113 are unused CM4 GPIOs, 7 ESP32, 3 MPU-6050. Add no-connect flags if you want a clean ERC; nothing is broken.

---

## Open items carried forward from Rev 2

Unchanged and still worth your attention. None is a blocker.

| Item | Status |
|---|---|
| **U4 EN floating** — no programmable UVLO on the buck | Still open. ERC confirms (`U4` pin_not_connected ×1). Still the single most worthwhile change left: two resistors VSYS→EN→GND, UVLO ~17–18 V. |
| **CR2 thermal vias** | **Addressed** this session — 12 vias + a 4 mm-wide SW pour on B.Cu. Note this enlarges the switching node; it is a deliberate thermal-vs-EMI trade. |
| **3V3 rail budget** | Still open. ~500 mA typical from the CM4's 600 mA output; ESP32 Wi-Fi TX peaks push it over. |
| **No fiducials** (FD-001) | Still open, and it matters more than it looks — you have 0.5 mm-pitch DSBGA. Most assemblers want ≥3 per SMD side. |
| **Surface finish** | Specify **ENIG, not HASL**. The stackup still declares `HAL lead-free`. 0.5 mm-pitch DSBGA on HASL is a poor match. |
| Courtyard overlaps C110/C109, C82/C78 | Still open, probably assemblable. |
| SDZ has no software control; MPU-6050 INT unconnected; PCM5102A SCK pin choice | Still open, all firmware-workable. |

**One new item:** the mux DSBGA pads have **no paste apertures** (`F.Cu` + `F.Mask` only — 42 pads). This is the KiCad BGA library default and is defensible for a balled DSBGA, but confirm your assembler is happy placing them dry.

---

## Ordering checklist

- 4 layers, 128 × 128 mm, 1.6 mm
- **Surface finish: ENIG**
- Min track 0.2 mm · min spacing 0.127 mm · min drill 0.4 mm — but fix U8's 0.123 mm first
- Tented vias (10 via-in-pad instances)
- Stencil required — 1211 paste apertures
- Confirm inner-layer copper weight (1 oz declared; 0.5 oz is the common default)
- **Re-export gerbers last**, after every fix

**Before you click order:**
1. Save the file — the current refill is unsaved.
2. Fix C100/C101/C129 to 50 V. ← the only one that affects hardware
3. Populate LCSC numbers if you want assembly.
4. Delete the 42 phantom mux pads, then re-run DRC and confirm it comes back near-zero.
5. Fix U8 clearance, remove the duplicate via.
6. Export gerbers once, at the end.

---

## What ran, and what didn't

**Ran:** KiCad native ERC (210 violations) · KiCad native DRC with schematic parity (370 violations, 0 unconnected, 6 parity) · `analyze_schematic.py` · `analyze_pcb.py --full --proximity` · `cross_analysis.py` · `analyze_emc.py` (125 findings) · `analyze_thermal.py` · `analyze_gerbers.py` · direct gerber geometry verification · connectivity-partition cross-reference · raw S-expression spot-checks on mux/amplifier footprints and zone fills.

**Not run:**
- **SPICE** — `ngspice`, `ltspice`, `xyce` all absent. Filter/divider/compensation values checked analytically only.
- **Lifecycle/obsolescence audit** — no distributor API credentials, and MPNs are not in symbol properties.
- **`analyze_thermal.py` returned nothing useful** (score 100/100, one component assessed). Rev 2's hand-calculated thermal table still stands: ~11 W per TPA3116D2 at full sine, heatsink required.
- **Impedance not verified** — TMDS pairs are beautifully length-matched (0.00–0.02 mm intra-pair) but I cannot confirm 100 Ω differential without the fab's stackup.
- **U20/U21 thermal pad dimensions** not confirmed against the DAP drawing — datasheet page would not text-extract.

**Caveat:** the `.kicad_pcb` was edited three times mid-review (18:17 → 18:35 → unsaved refill). All DRC figures above are from the **18:35** save. Re-run DRC after saving.
