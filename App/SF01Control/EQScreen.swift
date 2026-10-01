import SwiftUI

struct EQScreen: View {
    @EnvironmentObject private var controller: SpeakerController
    @State private var selected = 0

    private var cfg: SF01Config { controller.config }
    private var band: EQBand { cfg.bands[min(selected, cfg.bands.count - 1)] }

    /// SET_EQ carries type, frequency, Q and gain together, so every edit
    /// writes the whole band.
    private func update(_ i: Int, _ transform: (inout EQBand) -> Void) {
        guard i < controller.config.bands.count else { return }
        var b = controller.config.bands[i]
        transform(&b)
        controller.config.bands[i] = b
        controller.send(.band(b))
    }

    private func bind(_ i: Int, _ path: WritableKeyPath<EQBand, Double>) -> Binding<Double> {
        Binding(get: { controller.config.bands[i][keyPath: path] },
                set: { v in update(i) { $0[keyPath: path] = v } })
    }

    private func freqLabel(_ f: Double) -> String {
        f >= 1000 ? String(format: "%.1fk", f / 1000) : String(format: "%.0f", f)
    }

    var body: some View {
        ScrollView {
            VStack(spacing: 18) {
                EQGraphView(bands: cfg.bands,
                            sampleRate: Double(cfg.sampleRate),
                            crossoverHz: cfg.crossoverHz,
                            selected: $selected) { i, f, db in
                    update(i) {
                        $0.frequency = min(max(f, 20), 20_000).rounded()
                        $0.gainDB = (db * 2).rounded() / 2
                    }
                }
                .padding(.horizontal)

                Text("Drag a handle — sideways for frequency, up and down for gain. Dashed lines show the crossover.")
                    .font(.caption2)
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
                    .padding(.horizontal)

                bandRow
                Divider().padding(.horizontal)
                selectedBand
            }
            .padding(.vertical)
        }
        .navigationTitle("Equaliser")
        .navigationBarTitleDisplayMode(.inline)
    }

    // MARK: all bands

    private var bandRow: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text("BANDS")
                .font(.caption2.weight(.semibold))
                .foregroundStyle(.secondary)
                .padding(.horizontal)

            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 16) {
                    ForEach(cfg.bands) { b in
                        Knob(label: "\(b.index + 1) · \(freqLabel(b.frequency))",
                             caption: b.type == .off ? "off"
                                                     : String(format: "%+.1f", b.gainDB),
                             value: bind(b.index, \.gainDB),
                             range: -18...18,
                             accent: bandColor(b.index),
                             isSelected: b.index == selected,
                             isEnabled: b.type != .off,
                             onSelect: { selected = b.index })
                        .onTapGesture { selected = b.index }
                    }
                }
                .padding(.horizontal)
            }
        }
    }

    private func bandColor(_ i: Int) -> Color {
        let hues: [Double] = [0.00, 0.08, 0.14, 0.33, 0.48, 0.58, 0.72, 0.85]
        return Color(hue: hues[i % hues.count], saturation: 0.75, brightness: 0.92)
    }

    // MARK: selected band

    private var selectedBand: some View {
        VStack(spacing: 14) {
            HStack {
                Text("BAND \(selected + 1)")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(.secondary)
                Spacer()
                Button("Reset") {
                    update(selected) {
                        $0.type = .off; $0.gainDB = 0; $0.q = 0.7; $0.frequency = 1000
                    }
                }
                .font(.caption)
            }
            .padding(.horizontal)

            Picker("Type", selection: Binding(
                get: { band.type },
                set: { t in update(selected) { $0.type = t } })
            ) {
                ForEach(EQType.allCases) { Text($0.label).tag($0) }
            }
            .pickerStyle(.segmented)
            .padding(.horizontal)

            HStack(spacing: 28) {
                // Frequency rides a log scale so the knob feels even across the band.
                Knob(label: "Freq",
                     caption: freqLabel(band.frequency) + " Hz",
                     value: Binding(
                        get: { log10(max(20, controller.config.bands[selected].frequency)) },
                        set: { l in update(selected) { $0.frequency = pow(10, l).rounded() } }),
                     range: log10(20)...log10(20_000),
                     accent: bandColor(selected),
                     isEnabled: band.type != .off)

                Knob(label: "Q",
                     caption: String(format: "%.2f", band.q),
                     value: bind(selected, \.q),
                     range: 0.2...8,
                     accent: bandColor(selected),
                     isEnabled: band.type != .off)

                Knob(label: "Gain",
                     caption: String(format: "%+.1f dB", band.gainDB),
                     value: bind(selected, \.gainDB),
                     range: -18...18,
                     accent: bandColor(selected),
                     isEnabled: band.type != .off)
            }

            if band.type == .off {
                Text("Pick a filter type to enable this band.")
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            }
        }
    }
}
