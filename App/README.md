# SF01Control — iOS EQ remote

SwiftUI app that tunes the SF_01 speaker's DSP over BLE while music streams to
it over A2DP.

* `SF01Protocol.swift` — wire format, mirroring `Firmware/.../main/ble_ctl.h`
* `SpeakerController.swift` — CoreBluetooth client, auto-reconnect, write coalescing
* `ContentView.swift` / `BandEditor.swift` — the UI

## Build and install

```bash
export DEVELOPER_DIR=/Applications/Xcode-beta.app/Contents/Developer
xcodebuild -project App/SF01Control.xcodeproj -scheme SF01Control \
  -configuration Debug -destination 'id=<device-udid>' \
  -allowProvisioningUpdates build
xcrun devicectl device install app --device <device-udid> <path-to>/SF01Control.app
```

Signing is automatic against team `LAFKSHQ45W`.

**First install needs a one-time trust step on the phone:** Settings → General →
VPN & Device Management → Developer App → Trust. On a free Apple developer
account the provisioning profile also expires after 7 days, after which the app
must be rebuilt and reinstalled.

## Equaliser screen

`EQScreen` shows a log-frequency response plot with a draggable handle per
active band — sideways for frequency, up and down for gain — over a scrollable
row of gain knobs and a Freq/Q/Gain trio for the selected band.

`EQMath.swift` re-implements the same RBJ cookbook filters the firmware runs in
`dsp.c`, so the drawn curve *is* the response you hear rather than an
approximation of it. The plot also overlays the Linkwitz-Riley crossover
branches as dashed lines, so EQ moves can be read against where the two ways
actually split.

Knobs track a **vertical drag** rather than true circular motion: on a phone
your finger covers the knob, and chasing an angle under your own thumb is
miserable. Frequency knobs ride a log scale so they feel even across the band.

## Design notes

**Write coalescing.** A slider drag emits values far faster than BLE should
carry them, and the radio is shared with the audio link. Commands are coalesced
per control and flushed on a 50 ms tick, so a drag costs at most 20 writes per
second and the final value always lands.

**Echo guard.** The speaker notifies every change back. During a drag those
echoes arrive a beat late and would yank the knob backwards, so remote updates
are ignored for 500 ms after a local write. Changes made elsewhere — the UART
shell, AVRCP volume, the mute sequencer — still appear as soon as the user lets
go of the control.
