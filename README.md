# Triangulate-SF_01-Speaker-Frame

*AI Use: No AI was used to generate any journal entries at all. AI was only used for electronics review and firmware help*

## Description
The SF_01 is a 24-inch digital picture frame running Android on a Raspberry Pi CM4. The Pi pushes an image to an off-the-shelf IPS monitor from LG through HDMI, and has wireless and USB peripheral support for touch displays, keyboards, and other HIDs.

The SF_01 also has a total 220W RMS speaker system on board, with 6 3.5-inch bass-mid woofers, and 4 0.75-inch tweeterspointing outwards from all four corners.

The frame itself is 3D-printed, with a large stand secured by VESA mount screws driven into the back of the monitor.

The frame has a separate circuit on board for audio playback, with an ESP32-S3 WROOM 2 module at its heart. It decodes a stereo Bluetooth A2DP stream, performs DSP calculations on board, and outputs two different I2S signals (one for the bass-mid woofers, the other for the tweeters).

After going through a DAC, the audio signals are routed through a custom analog multiplexer circuit to all 10 drivers, dynamically switching stereo output between drivers depending on the orientation of the frame.

The ESP32 appears in a device's Bluetooth list as a speaker, and it also runs a GATT server that is controlled by a Bluetooth LE connection via a custom iPhone app. Equalisers and crossovers can be set in the app.

### Woofer Layout

```
(1) ---------- (2)
(3)            (4)
(5) ---------- (6)
```

### Tweeter Layout

```
(1) ---------- (2)
 |              |
(3) ---------- (4)
```

**Landscape Orientation:**
* Left Channel:
* Woofers 1, 3, 5
* Tweeters 1, 3

* Right Channel:
* Woofers 2, 4, 6
* Tweeters 2, 4
  
**Portrait Orientation:**

* Left Channel:
* Woofers 5, 6
* Tweeters 3, 4

* Right Channel:
* Woofers 1, 2
* Tweeters 1, 2

* **Centre Channel: (Average of Right and Left):**
* Woofers 3, 4

## Enclosure Images
<img width="1800" height="1018" alt="Render 1" src="https://github.com/user-attachments/assets/a04897f2-f62e-4ffe-8b50-f99c1e0dd5e2" />
<img width="1800" height="1018" alt="Render 2" src="https://github.com/user-attachments/assets/c4de3a8d-acc1-4b1d-9af0-68945ec476ff" />
<img width="1800" height="1018" alt="Render 3" src="https://github.com/user-attachments/assets/7c89f148-860e-4543-81ff-d67332316a94" />
<img width="1800" height="1018" alt="Render 4" src="https://github.com/user-attachments/assets/ee7f8f9e-6355-46c2-87b7-58cfd211c6b3" />
<img width="1800" height="1018" alt="Render 5" src="https://github.com/user-attachments/assets/a55f2150-8e86-4a01-9bbd-c5364927f5e2" />

## PCB Schematics

**Raspberry Pi Circuit**

<img width="1292" height="890" alt="image" src="https://github.com/user-attachments/assets/eeb535a9-7fc0-44c3-b733-ed6fc758e378" />


**DAC and Analog Multiplexer Circuit**

<img width="1296" height="891" alt="image" src="https://github.com/user-attachments/assets/aa34bc56-6bd7-462a-9fb6-6a11734fb8d1" />


**ESP32 WROVER E Circuit**

<img width="1293" height="889" alt="image" src="https://github.com/user-attachments/assets/d22475ee-5648-4cf5-9f6b-f99f37f399b9" />


**Subwoofer Amplifiers**

<img width="1292" height="887" alt="image" src="https://github.com/user-attachments/assets/b1649398-994f-46ee-9a5f-5992a8ac4938" />


**Tweeter Amplifiers**

<img width="1292" height="889" alt="image" src="https://github.com/user-attachments/assets/e087710a-4417-49f6-9dd1-b133999edc0a" />


**Environmental and Orientation Sensors**

<img width="1292" height="889" alt="image" src="https://github.com/user-attachments/assets/e14a7a73-9433-4164-a90a-611f6f2925ad" />

**Power Circuitry**

<img width="1295" height="887" alt="image" src="https://github.com/user-attachments/assets/a3cb31b8-37ba-4196-929a-0a12a4f34ea5" />



## PCB Layout

**Top Layer**

<img width="940" height="955" alt="image" src="https://github.com/user-attachments/assets/9f1931e9-d1a4-4741-917e-42fca7e2b5d6" />


**Inner Layer 1 (GND Plane)**

<img width="934" height="956" alt="image" src="https://github.com/user-attachments/assets/ccf0d40b-5bc5-47df-995b-38f2ba099dbf" />


**Inner Layer 2**

<img width="930" height="952" alt="image" src="https://github.com/user-attachments/assets/0256759b-2b11-46ce-b2ee-0fe89f66f1b4" />


**Bottom Layer**

<img width="931" height="952" alt="image" src="https://github.com/user-attachments/assets/1070fa23-319b-423e-8027-019df26495dd" />


**3D Model**

<img width="1007" height="899" alt="image" src="https://github.com/user-attachments/assets/6ec79e9c-0179-4767-9893-c2a0e2132698" />

<img width="877" height="940" alt="image" src="https://github.com/user-attachments/assets/ed8b4db7-03dc-4408-ac84-b82dd87cbe03" />

<img width="835" height="897" alt="image" src="https://github.com/user-attachments/assets/24bedc9b-3685-4c5f-b0e3-7fb6877c32f6" />


## Bill Of Materials

Quantities are **per board**. Prices are LCSC unit pricing at low volume and drop at quantity. Everything except the Compute Module, drivers and panel is sourced from LCSC.

### Board Components

| Designators | Qty | Value | Part Number | Manufacturer | Package | Description | Unit (USD) | Ext. (USD) | LCSC |
|---|---|---|---|---|---|---|---|---|---|
| C1, C4, C5, C7, C9, C14, C17, C19, C23, C26, C28, C31, C34, C37, C38, C39, C42, C43, C45, C47, C50, C53, C72, C74, C80, C81, C106, C109, C135, C138, C163, C166, C185, C186, C189 | 35 | 100nF | CL10B104KB8NNNC | Samsung Electro-Mechanics | 0603 | 100nF ±10% 50V X7R 0603 | 0.0142 | 0.50 | [C1591](https://www.lcsc.com/product-detail/C1591.html) |
| C49, C52, C61, C62, C67, C68, C79, C82, C92, C93, C94, C95, C107, C108, C119, C120, C121, C122, C136, C137, C149, C150, C151, C152, C164, C165, C177, C178, C179, C180 | 30 | 1nF | CL10C102JB8NNNC | Samsung Electro-Mechanics | 0603 | 1nF ±5% 50V C0G 0603 | 0.0076 | 0.23 | [C163508](https://www.lcsc.com/product-detail/C163508.html) |
| C2, C8, C12, C13, C40, C41, C44, C46, C48, C75, C76, C77, C102, C103, C105, C131, C132, C134, C157, C158, C159, C160, C162, C188, C190 | 25 | 1uF | CL10B105KA8NNNC | Samsung Electro-Mechanics | 0603 | 1uF ±10% 25V X7R 0603 | 0.0218 | 0.55 | [C29936](https://www.lcsc.com/product-detail/C29936.html) |
| C24, C63, C64, C69, C70, C96, C97, C98, C99, C123, C124, C125, C126, C153, C154, C155, C156, C181, C182, C183, C184 | 21 | 10nF | GRM1885C1H103JA01D | muRata | 0603 | 10nF ±5% 50V C0G 0603 | 0.0547 | 1.15 | [C85973](https://www.lcsc.com/product-detail/C85973.html) |
| C55, C56, C57, C58, C84, C85, C86, C87, C111, C112, C113, C114, C141, C142, C143, C144, C169, C170, C171, C172 | 20 | 220nF | CL10B224KB8NNNC | Samsung Electro-Mechanics | 0603 | 220nF ±10% 50V X7R 0603 | 0.0121 | 0.24 | [C64705](https://www.lcsc.com/product-detail/C64705.html) |
| C59, C60, C65, C66, C88, C89, C90, C91, C115, C116, C117, C118, C145, C146, C147, C148, C173, C174, C175, C176 | 20 | 680nF | CGA3E3X5R1H684KT0Y0N | TDK | 0603 | 680nF ±10% 50V X5R 0603 | 0.1052 | 2.10 | [C694504](https://www.lcsc.com/product-detail/C694504.html) |
| C10, C11, C35, C36, C71 | 5 | 2.2nF | CL10B222KB8NNNC | Samsung Electro-Mechanics | 0603 | 2.2nF ±10% 50V X7R 0603 | 0.0061 | 0.03 | [C16033](https://www.lcsc.com/product-detail/C16033.html) |
| C20, C21, C29, C30 | 4 | 2.2uF | CL10A225KA8NNNC | Samsung Electro-Mechanics | 0603 | 2.2uF ±10% 25V X5R 0603 | 0.0231 | 0.09 | [C57895](https://www.lcsc.com/product-detail/C57895.html) |
| C6 | 1 | 22uF | CL10A226MP8NUNE | Samsung Electro-Mechanics | 0603 | 22uF ±20% 10V X5R 0603 | 0.0825 | 0.08 | [C86295](https://www.lcsc.com/product-detail/C86295.html) |
| C187 | 1 | 6.8pF | CC0603BRNPO9BN6R8 | YAGEO | 0603 | 6.8pF 50V NP0 0603 | 0.0038 | 0.00 | [C519114](https://www.lcsc.com/product-detail/C519114.html) |
| C130 | 1 | 7.5nF | GRM1885C1H752JA01D | muRata | 0603 | 7.5nF ±5% 50V C0G 0603 | 0.0156 | 0.02 | [C907758](https://www.lcsc.com/product-detail/C907758.html) |
| C100, C101, C129 | 3 | 10uF/50V | GRM32ER71H106KA12L | muRata | 1210 | 10uF ±10% 50V X7R 1210 | 0.3758 | 1.13 | [C77102](https://www.lcsc.com/product-detail/C77102.html) |
| C191, C192 | 2 | 47uF | GRM32ER61C476KE15L | muRata | 1210 | 47uF ±10% 16V X5R 1210 | 0.4273 | 0.85 | [C77101](https://www.lcsc.com/product-detail/C77101.html) |
| C51, C54, C78, C83, C104, C110, C133, C139, C161, C167 | 10 | 220uF | RVT35V220M6X8 | JIERR | SMD,D6.3xL7.7mm | 220uF ±20% 35V aluminium electrolytic 105C | 0.1003 | 1.00 | [C47023115](https://www.lcsc.com/product-detail/C47023115.html) |
| C3 | 1 | 10uF | EEEFTH100UAR | Panasonic | SMD,D4xL5.8mm | 10uF ±20% 50V aluminium electrolytic 105C | 0.1811 | 0.18 | [C242129](https://www.lcsc.com/product-detail/C242129.html) |
| C15, C16, C18, C22, C25, C27, C32, C33 | 8 | 10uF | EEEFTH100UAR | Panasonic | SMD,D4xL5.8mm | 10uF ±20% 50V aluminium electrolytic 105C | 0.1811 | 1.45 | [C242129](https://www.lcsc.com/product-detail/C242129.html) |
| C73 | 1 | 22uF | EEEFN1E220R | Panasonic | SMD,D4xL5.8mm | 22uF ±20% 25V aluminium electrolytic 105C | 0.1796 | 0.18 | [C494818](https://www.lcsc.com/product-detail/C494818.html) |
| R16, R18, R20, R26, R27, R30, R35, R36, R39, R45, R46, R50, R55, R56, R60, R76 | 16 | 100K | FRC0603J104TS | FOJAN | 0603 | 100kOhm ±5% 100mW 0603 | 0.0019 | 0.03 | [C2907088](https://www.lcsc.com/product-detail/C2907088.html) |
| R4, R12, R67, R68, R69, R70, R71, R77 | 8 | 10K | CRCW060310K0FKEA | VISHAY | 0603 | 10kOhm ±1% 100mW 0603 | 0.0057 | 0.05 | [C844918](https://www.lcsc.com/product-detail/C844918.html) |
| R21, R22, R23, R24, R31, R32, R33, R34, R40, R41, R42, R43, R51, R52, R53, R54, R61, R62, R63, R64 | 20 | 3.3R | FRC0805J3R3TS | FOJAN | 0805 | 3.3Ohm ±5% 125mW 0805 | 0.0037 | 0.07 | [C2907317](https://www.lcsc.com/product-detail/C2907317.html) |
| R15 | 1 | 3.3R | FRC0603F3R30TS | FOJAN | 0603 | 3.3Ohm ±1% 100mW 0603 | 0.0025 | 0.00 | [C2930016](https://www.lcsc.com/product-detail/C2930016.html) |
| R25, R44, R49, R59 | 4 | 100R | FRC0603F1000TS | FOJAN | 0603 | 100Ohm ±1% 100mW 0603 | 0.0021 | 0.01 | [C2906981](https://www.lcsc.com/product-detail/C2906981.html) |
| R28, R37, R47, R57 | 4 | 47K | FRC0603F4702TS | FOJAN | 0603 | 47kOhm ±1% 100mW 0603 | 0.0023 | 0.01 | [C2907042](https://www.lcsc.com/product-detail/C2907042.html) |
| R29, R38, R48, R58 | 4 | 75K | FRC0603F7502TS | FOJAN | 0603 | 75kOhm ±1% 100mW 0603 | 0.0025 | 0.01 | [C2907067](https://www.lcsc.com/product-detail/C2907067.html) |
| R5, R6, R72, R73 | 4 | 1K | FRC0603F1001TS | FOJAN | 0603 | 1kOhm ±1% 100mW 0603 | 0.0014 | 0.01 | [C2907002](https://www.lcsc.com/product-detail/C2907002.html) |
| R1, R2 | 2 | 2.2K | FRC0603F2201TS | FOJAN | 0603 | 2.2kOhm ±1% 100mW 0603 | 0.0023 | 0.00 | [C2907005](https://www.lcsc.com/product-detail/C2907005.html) |
| R3, R65 | 2 | 5.1K | FRC0603F5101TS | FOJAN | 0603 | 5.1kOhm ±1% 100mW 0603 | 0.0023 | 0.00 | [C2907044](https://www.lcsc.com/product-detail/C2907044.html) |
| R7, R17 | 2 | 56K | 0603WAF5602T5E | UNI-ROYAL | 0603 | 56kOhm ±1% 100mW 0603 | 0.0081 | 0.02 | [C23206](https://www.lcsc.com/product-detail/C23206.html) |
| Q1 | 1 | BSS138 | BSS138 | Shikues | SOT-23 | N-channel logic-level MOSFET 50V 0.2A | 0.0191 | 0.02 | [C112239](https://www.lcsc.com/product-detail/C112239.html) |
| R19 | 1 | 20K | FRC0603F2002TS | FOJAN | 0603 | 20kOhm ±1% 100mW 0603 | 0.0020 | 0.00 | [C2907011](https://www.lcsc.com/product-detail/C2907011.html) |
| R8, R9, R10, R11 | 4 | 470R | FRC0603F4700TS | FOJAN | 0603 | 470Ohm ±1% 100mW 0603 | 0.0027 | 0.01 | [C2907041](https://www.lcsc.com/product-detail/C2907041.html) |
| R14, R78 | 2 | 220R | 0603WAF2200T5E | UNI-ROYAL | 0603 | 220Ohm ±1% 100mW 0603 | 0.0010 | 0.00 | [C22962](https://www.lcsc.com/product-detail/C22962.html) |
| R66 | 1 | 205K | FRC0603F2053TS | FOJAN | 0603 | 205kOhm ±1% 100mW 0603 | 0.0008 | 0.00 | [C5126104](https://www.lcsc.com/product-detail/C5126104.html) |
| R75 | 1 | 60.4K | 0603WAF6042T5E | UNI-ROYAL | 0603 | 60.4kOhm ±1% 100mW 0603 | 0.0009 | 0.00 | [C23089](https://www.lcsc.com/product-detail/C23089.html) |
| R74 | 1 | 11.5K | FRC0603F1152TS | FOJAN | 0603 | 11.5kOhm ±1% 100mW 0603 | 0.0009 | 0.00 | [C2930057](https://www.lcsc.com/product-detail/C2930057.html) |
| R13 | 1 | 10.7K | FRC0603F1072TS | FOJAN | 0603 | 10.7kOhm ±1% 100mW 0603 | 0.0025 | 0.00 | [C5153968](https://www.lcsc.com/product-detail/C5153968.html) |
| L1, L2, L3, L4, L5, L6, L7, L8, L9, L10, L11, L12 | 12 | 10uH/10A | SMDRH104R-100MT | cjiang | SMD,10.4x10.3mm | 10uH ±20% 3.8A rated / 5.6A sat / 35mOhm | 0.1916 | 2.30 | [C9935](https://www.lcsc.com/product-detail/C9935.html) |
| L13, L14, L15, L16, L17, L18, L19, L20 | 8 | 10uH/3A | FHD4020S-100MT | cjiang | SMD,4x4mm | 10uH ±20% 2.35A rated / 3.5A sat / 190mOhm | 0.1102 | 0.88 | [C602032](https://www.lcsc.com/product-detail/C602032.html) |
| L21 | 1 | 6.8uH/6.6A | AAPS0650M6R8F | Coilank | SMD,6.6x6.4mm | 6.8uH ±20% 6.6A rated / 7A sat / 25.4mOhm molded | 0.9665 | 0.97 | [C49261484](https://www.lcsc.com/product-detail/C49261484.html) |
| U4 | 1 | TPS54540DDAR | TPS54540DDAR | Texas Instruments | SOIC-8-EP | 42V 5A step-down converter | 1.4638 | 1.46 | [C95286](https://www.lcsc.com/product-detail/C95286.html) |
| CR2 | 1 | STPS30H100DJF-TR | STPS30H100DJF-TR | ST | PowerFLAT5x6-8 | 100V 30A Schottky | 1.3985 | 1.40 | [C2969851](https://www.lcsc.com/product-detail/C2969851.html) |
| U10 | 1 | LP5907MFX-4.5 | LP5907MFX-4.5/NOPB | Texas Instruments | SOT-23-5 | 4.5V LDO | 0.5581 | 0.56 | [C529554](https://www.lcsc.com/product-detail/C529554.html) |
| U11 | 1 | LP5907MFX-3.3 | LP5907MFX-3.3 | Texas Instruments | SOT-23-5 | 3.3V LDO | 0.1343 | 0.13 | [C23380874](https://www.lcsc.com/product-detail/C23380874.html) |
| U17, U18, U19 | 3 | TPA3116D2DAD | TPA3116D2DADR | Texas Instruments | HTSSOP-32-6.1mm | 2x50W@4ohm Class D amplifier | 1.0619 | 3.19 | [C50144](https://www.lcsc.com/product-detail/C50144.html) |
| U20, U21 | 2 | TPA3130D2DAPR | TPA3130D2DAPR | Texas Instruments | HTSSOP-32-EP-6.1mm | 2x15W@8ohm Class D amplifier | 0.6314 | 1.26 | [C95206](https://www.lcsc.com/product-detail/C95206.html) |
| U12, U13, U15, U16, U24, U25, U26 | 7 | TS5A3159AYZPR | TS5A3159AYZPR | Texas Instruments | WCSP-6(1.4x0.9) | SPDT analog switch | 0.3971 | 2.78 | [C2651909](https://www.lcsc.com/product-detail/C2651909.html) |
| U5, U14 | 2 | PCM5102A | PCM5102APWR | Texas Instruments | TSSOP-20 | I2S stereo DAC | 1.3577 | 2.72 | [C107671](https://www.lcsc.com/product-detail/C107671.html) |
| U27 | 1 | TPS2041B | TPS2041BDBVR | Texas Instruments | SOT-23-5 | 500mA current-limited power switch | 0.3173 | 0.32 | [C51386](https://www.lcsc.com/product-detail/C51386.html) |
| U6 | 1 | RT9742SNGV | RT9742SNGV | RICHTEK | SOT-23-3 | 500mA high-side switch | 0.4386 | 0.44 | [C3235509](https://www.lcsc.com/product-detail/C3235509.html) |
| U9 | 1 | 74LVC1G07 | SN74LVC1G07DBVR | Texas Instruments | SOT-23-5 | Open-drain buffer 1.65-5.5V | 0.1109 | 0.11 | [C7829](https://www.lcsc.com/product-detail/C7829.html) |
| TP1–TP21, TP23–TP25 | 24 | TestPoint | — | — | 3.0x3.0mm pad / D2.5mm pad / D2.0mm plated hole | Bare copper - nothing to buy. 3 top, 4 plated through-hole, 17 on the bottom layer | 0.0000 | 0.00 | — |
| U28 | 1 | ESP32-WROVER-E-N16R8 | ESP32-WROVER-E-N16R8 | ESPRESSIF | SMD,31.4x18mm | WiFi/BT module 16MB flash 8MB PSRAM | 5.7127 | 5.71 | [C529589](https://www.lcsc.com/product-detail/C529589.html) |
| U22 | 1 | MPU-6050 | MPU-6050 | TDK InvenSense | QFN-24-EP(4x4) | 6-axis IMU | 6.3857 | 6.39 | [C24112](https://www.lcsc.com/product-detail/C24112.html) |
| U23 | 1 | BME680 | BME680 | Bosch | LGA-8 | Environmental sensor | 8.0629 | 8.06 | [C125972](https://www.lcsc.com/product-detail/C125972.html) |
| U2, U3, U7 | 3 | TPD4E02B04DQA | TPD4E02B04DQAR | Texas Instruments | USON-10(1x2.5) | 4-channel ESD array | 0.0996 | 0.30 | [C106794](https://www.lcsc.com/product-detail/C106794.html) |
| CR1 | 1 | SP3010-04UTG | SP3010-04UTG | Littelfuse | UFDFN-10(1x2.5) | 4-channel TVS array | 0.2544 | 0.25 | [C126837](https://www.lcsc.com/product-detail/C126837.html) |
| D1 | 1 | SMAJ26A | SMAJ26A | GOODWORK | DO-214AC(SMA) | 26V unidirectional TVS 400W Vbr 28.9V Vc 42.1V@9.5A | 0.0333 | 0.03 | [C2848697](https://www.lcsc.com/product-detail/C2848697.html) |
| D2, D5, D6, D7 | 4 | WS2812B | WS2812B-V6 | Worldsemi | SMD5050-4P | Addressable RGB LED | 0.1009 | 0.40 | [C52917433](https://www.lcsc.com/product-detail/C52917433.html) |
| D3, D4 | 2 | LED | YLED0603B | YONGYUTAI | 0603 | Blue LED 2.6-3.2V | 0.0058 | 0.01 | [C19171394](https://www.lcsc.com/product-detail/C19171394.html) |
| J1 | 1 | SS-53200-001 | HDMI 19PIN 043 | SHOU HAN | SMD | HDMI-A receptacle right angle | 0.1365 | 0.14 | [C2858275](https://www.lcsc.com/product-detail/C2858275.html) |
| J5, J6, J7, J8, J9, J10, J11, J12, J13, J14 | 10 | Speaker out | S2B-XH-A(LF)(SN) | JST | TH Right Angle P=2.5mm | 2-position 2.5mm header 3A | 0.0615 | 0.62 | [C157931](https://www.lcsc.com/product-detail/C157931.html) |
| J15 | 1 | USB_C_Receptacle_USB2.0_16P | TYPE-C 16PIN 2MD(073) | SHOU HAN | SMD | USB-C receptacle 16P | 0.0707 | 0.07 | [C2765186](https://www.lcsc.com/product-detail/C2765186.html) |
| J2 | 1 | Barrel_Jack | RAPC10U | Switchcraft | - | Power barrel jack 1.93/5.75mm | 17.0122 | 17.01 | [C3095802](https://www.lcsc.com/product-detail/C3095802.html) |
| J3 | 1 | UART | PZ254V-11-04P | XFCN | TH P=2.54mm | 4-position 2.54mm header | 0.0254 | 0.03 | [C2691448](https://www.lcsc.com/product-detail/C2691448.html) |
| SW1 | 1 | POWER | KSC931JLFS | C&K | SMD-4P,6.2x6.2mm | Tactile switch SPST IP67 J-lead | 2.0503 | 2.05 | [C221769](https://www.lcsc.com/product-detail/C221769.html) |
| SW2, SW3, SW4 | 3 | Tactile | KMR211GLFS | C&K | SMD | Tactile switch SPST-NO | 0.6377 | 1.91 | [C221675](https://www.lcsc.com/product-detail/C221675.html) |
| SW6 | 1 | SW_DPDT_x2 | JS202011JAQN | C&K | SMD | Slide switch DPDT 300mA@6V, right-angle | 1.2032 | 1.20 | [C221664](https://www.lcsc.com/product-detail/C221664.html) |
| U1 | 1 | CM4102008 | CM4102008 | Raspberry Pi | - | Compute Module 4 | 124.9100 | 124.91 | — |

**Board subtotal: $197.63 per board**

### Off-Board / Mechanical

| Qty | Item | Part Number | Manufacturer | Description | Unit (USD) | Ext. (USD) |
|---|---|---|---|---|---|---|
| 6 | Woofer | PLS-P830985 | Peerless by Tymphany | 3.95ohm full-range 30W driver | 18.74 | 112.45 |
| 4 | Tweeter | ND16FA-6 | Dayton Audio | Neodymium dome tweeter | 12.47 | 49.88 |
| 1 | Display | 24U411A-B | LG | 23.8in IPS FHD 120Hz panel | 74.53 | 74.53 |

**Off-board subtotal: $236.86**

### Items Needing Attention

| Designators | Status | Note |
|---|---|---|
| C100, C101, C129 | CHANGED + FOOTPRINT | Was 10uF/25V 0603 on the 24V rail. 10uF/50V does not exist in 0603 - the footprint has to go to 1210 |
| C191, C192 | CHANGED + FOOTPRINT | TPS54540 output bulk. 47uF does not exist in 0603 or 0805 at >=16V - footprint has to go to 1210 |
| L21 | CHANGED - NO PCB CHANGE | Now AAPS0650M6R8F (Coilank), 6.6A rated / 7A sat / 25.4mOhm. Replaced VLS6045EX-6R8M which saturated at full load (4.7A sat vs a 4.72A peak). 48% saturation margin now, and half the DCR. Body 6.6x6.4mm fits the existing 7.50x6.80mm courtyard so the land pattern is unchanged |
| CR2, R8-R11, R13, SW1, SW6 | CHANGED - RESTOCK | All five originals hit 0 stock on LCSC. CR2 -> STPS30H100DJF-TR (C2969851, same PowerFLAT 5x6 family, 100V, lowest Vf of the in-stock options). R8-R11 -> FRC0603F4700TS (C2907041). R13 -> FRC0603F1072TS (C5153968, only 1k in stock; plain 10K works as a fallback). SW1 -> KSC931JLFS (C221769, only 18 in stock). SW6 -> JS202011JAQN (C221664) which is RIGHT-ANGLE where the original was VERTICAL, so verify the land pattern |
| U8 | DELETED | U8 buffered CM4 RUN_PG into the GLOBAL_EN button, forming a feedback loop that repeatedly power-cycled the module. Removed. SW1 now goes straight to GPIO3/GND as a soft power button (`dtoverlay=gpio-shutdown`), with R77 as pull-up and C74 as debounce |
| U9 | CHANGED - FOOTPRINT | Footprint is SOT-23-5, same as U10/U11/U27; symbol pin numbering unchanged (1=NC 2=A 3=GND 4=Y 5=VCC). MUST stay LVC. Budget alt C7394020 (UMW) |
| R77, R78, C74, TP1, TP2 | NEW | Power-button circuit. R77 10K pulls GPIO3 to 3V3, C74 100nF debounces it, R78 220R feeds TP1 for CM4 reset (220R is the value the CM4 datasheet specifies), TP2 exposes GLOBAL_EN for a hard force-off |
| D2, D5, D6, D7 | KEEP - RAIL ISSUE | Datasheet minimum VDD is 3.5V and these sit on the 3.3V rail. See review B5 |
| J1 | VERIFY | Footprint is drawn for the Stewart SS-53200-001 but the sourced part is SHOU HAN - confirm the land patterns match before ordering |
| J5, J6, J7, J8, J9, J10, J11, J12, J13, J14 | KEEP - MARGINAL | Rated 3A but a 50W/4ohm woofer channel draws 3.5A RMS. See review H9 |
| J2 | KEEP - VERIFY RATING | Confirm its current rating against the ~8A this board can draw at full output |
| U1 | NOT ON LCSC | Source from an approved Raspberry Pi reseller. Schematic says CM4102008 but the old BOM said CM4102016 - pick one |

## Total: $434.49 USD per complete unit
