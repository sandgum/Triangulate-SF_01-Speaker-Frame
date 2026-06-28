# Triangulate-SF_01-Speaker-Frame

## Description
The SF_01 is a 24-inch digital picture frame running Android on a Raspberry Pi CM4. The Pi pushes an image to an off-the-shelf IPS monitor from LG through HDMI, and has wireless and USB peripheral support for touch displays, keyboards, and other HIDs.

The SF_01 also has a total 220W RMS speaker system on board, with 6 3.5-inch bass-mid woofers, and 4 0.75-inch tweeterspointing outwards from all four corners.

The frame itself is 3D-printed, with a large stand secured by VESA mount screws driven into the back of the monitor.

The frame has a separate circuit on board for audio playback, with an ESP32-S3 WROOM 2 module at its heart. It decodes a stereo Bluetooth A2DP stream, performs DSP calculations on board, and outputs two different I2S signals (one for the bass-mid woofers, the other for the tweeters).

After going through a DAC, the audio signals are routed through a custom analog multiplexer circuit to all 10 drivers, synamically switching stereo output between drivers depending on the orientation of the frame.

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
<img width="1392" height="957" alt="image" src="https://github.com/user-attachments/assets/4e6586a7-b9e4-4d58-a416-6197ec918669" />

**DAC and Analog Multiplexer Circuit**
<img width="1357" height="931" alt="image" src="https://github.com/user-attachments/assets/e05115d2-ed6b-49c4-a719-acbef690f496" />

**ESP32 WROVER E Circuit**
<img width="1283" height="882" alt="image" src="https://github.com/user-attachments/assets/e32255ee-b3b8-48e8-a572-b41222ff93d8" />

**Subwoofer Amplifiers**
<img width="1365" height="938" alt="image" src="https://github.com/user-attachments/assets/40cbeb7e-c0ed-413e-9cf4-61133022d3c8" />

**Tweeter Amplifiers**
<img width="1311" height="902" alt="image" src="https://github.com/user-attachments/assets/c3568b26-137d-4eb5-a159-f3c34045f032" />

**Environmental and Orientation Sensors**
<img width="1282" height="882" alt="image" src="https://github.com/user-attachments/assets/045d2cf5-7e64-4bd4-9890-eaebdc6919cf" />

## PCB Layout

**Top Layer**

<img width="908" height="927" alt="image" src="https://github.com/user-attachments/assets/e3b37884-7fcc-41e7-af7b-bc0f2b765942" />

**Inner Layer 1 (GND Plane)**

<img width="915" height="926" alt="image" src="https://github.com/user-attachments/assets/474cf403-76ba-4cf7-9a2d-6c7099cceecf" />

**Inner Layer 2**

<img width="911" height="925" alt="image" src="https://github.com/user-attachments/assets/62ca37e4-bd6b-466e-a23b-0c6188ee2246" />

**Bottom Layer**

<img width="912" height="925" alt="image" src="https://github.com/user-attachments/assets/83138e29-fd7b-44f4-b36e-6536562548ec" />

**3D Model**

<img width="869" height="947" alt="image" src="https://github.com/user-attachments/assets/33ebc589-4067-4d1e-99b0-52d9e8d46e8b" />
<img width="935" height="984" alt="image" src="https://github.com/user-attachments/assets/038b8b9a-c50b-41e3-82f6-1065b8220d1d" />
<img width="769" height="933" alt="image" src="https://github.com/user-attachments/assets/10bf3d57-32da-43fd-b0f4-873b48c9d47c" />

## Bill Of Materials
*Coming soon*

## Assembly
*Coming soon*
