import SwiftUI

struct BandEditor: View {
    @EnvironmentObject private var controller: SpeakerController
    let index: Int

    private var band: EQBand { controller.config.bands[index] }

    /// Every edit writes the whole band, since the firmware's SET_EQ opcode
    /// carries type, frequency, Q and gain together.
    private func update(_ transform: (inout EQBand) -> Void) {
        var b = controller.config.bands[index]
        transform(&b)
        controller.config.bands[index] = b
        controller.send(.band(b))
    }

    var body: some View {
        Form {
            Section("Filter") {
                Picker("Type", selection: Binding(
                    get: { band.type },
                    set: { t in update { $0.type = t } })
                ) {
                    ForEach(EQType.allCases) { Text($0.label).tag($0) }
                }
                .pickerStyle(.segmented)
            }

            Section("Parameters") {
                ValueSlider(
                    title: "Frequency",
                    value: Binding(
                        get: { log10(max(20, band.frequency)) },
                        set: { l in update { $0.frequency = (pow(10, l)).rounded() } }),
                    range: log10(20)...log10(20000),
                    step: 0.01,
                    format: { hz in
                        let f = pow(10, hz)
                        return f >= 1000 ? String(format: "%.2f kHz", f / 1000)
                                         : String(format: "%.0f Hz", f)
                    })

                ValueSlider(
                    title: "Q",
                    value: Binding(get: { band.q }, set: { v in update { $0.q = v } }),
                    range: 0.2...8,
                    step: 0.05,
                    format: { String(format: "%.2f", $0) })

                ValueSlider(
                    title: "Gain",
                    value: Binding(get: { band.gainDB }, set: { v in update { $0.gainDB = v } }),
                    range: -18...18,
                    step: 0.5,
                    format: { String(format: "%+.1f dB", $0) })
                .disabled(band.type == .off)
            }

            if band.type == .off {
                Section {
                    Text("This band is bypassed. Choose a filter type to enable it.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                }
            }
        }
        .navigationTitle("Band \(index + 1)")
        .navigationBarTitleDisplayMode(.inline)
    }
}
