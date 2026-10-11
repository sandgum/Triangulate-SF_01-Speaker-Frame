import SwiftUI

/// Equaliser tab: the response plot, one gain slider per band, and the full
/// editor for whichever band is selected.
struct EQScreen: View {
    @EnvironmentObject private var controller: SpeakerController
    @State private var selected = 0

    private var cfg: SF01Config { controller.config }
    private var band: EQBand { cfg.bands[min(selected, cfg.bands.count - 1)] }
    private var activeCount: Int { cfg.bands.filter { $0.type != .off }.count }

    /// Where a band lands the first time it is switched on. The speaker's
    /// unused bands sit at 0 Hz with no Q, which is not a usable filter.
    private static let startingFrequencies: [Double] =
        [60, 150, 400, 1_000, 2_500, 6_000, 10_000, 14_000]

    private func startingFrequency(_ i: Int) -> Double {
        Self.startingFrequencies[i % Self.startingFrequencies.count]
    }

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

    private func bandColor(_ i: Int) -> Color {
        let hues: [Double] = [0.00, 0.08, 0.14, 0.33, 0.48, 0.58, 0.72, 0.85]
        return Color(hue: hues[i % hues.count], saturation: 0.75, brightness: 0.92)
    }

    var body: some View {
        NavigationStack {
            Group {
                if controller.state.isReady {
                    editor
                } else {
                    ContentUnavailableView {
                        Label("Speaker not connected", systemImage: "hifispeaker")
                    } description: {
                        Text(controller.state.label)
                    }
                }
            }
            .background(Color(.systemGroupedBackground).ignoresSafeArea())
            .navigationTitle("Equaliser")
            .animation(.default, value: controller.state.isReady)
            .sensoryFeedback(.selection, trigger: selected)
        }
    }

    private var editor: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 24) {
                VStack(alignment: .leading, spacing: 8) {
                    EQGraphView(bands: cfg.bands,
                                sampleRate: Double(cfg.sampleRate),
                                crossoverHz: cfg.crossoverHz,
                                selected: $selected) { i, f, db in
                        update(i) {
                            $0.frequency = min(max(f, 20), 20_000).rounded()
                            $0.gainDB = (db * 2).rounded() / 2
                        }
                    }

                    Text("Drag a handle sideways for frequency, up and down for gain. Dashed lines show the crossover.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                        .padding(.horizontal, 6)
                }

                card("Band gain") {
                    Text(activeCount == 0 ? "All flat"
                                          : "\(activeCount) of \(cfg.bands.count) active")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                } content: {
                    VStack(spacing: 2) {
                        ForEach(cfg.bands) { b in
                            gainRow(b)
                        }
                    }
                    .padding(6)
                }

                card("Band \(selected + 1)") {
                    Button("Reset") {
                        update(selected) {
                            $0.type = .off
                            $0.gainDB = 0
                            $0.q = 0.7
                            $0.frequency = startingFrequency($0.index)
                        }
                    }
                    .font(.footnote.weight(.semibold))
                } content: {
                    selectedBand
                        .padding(14)
                }
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 12)
            .animation(.snappy(duration: 0.2), value: selected)
        }
    }

    /// A titled group on the grouped background, matching the look of the
    /// inset list on the Speaker tab.
    private func card<Accessory: View, Content: View>(
        _ title: String,
        @ViewBuilder accessory: () -> Accessory,
        @ViewBuilder content: () -> Content
    ) -> some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(alignment: .firstTextBaseline) {
                Text(title)
                    .font(.subheadline.weight(.semibold))
                    .foregroundStyle(.secondary)
                Spacer()
                accessory()
            }
            .padding(.horizontal, 6)

            content()
                .frame(maxWidth: .infinity)
                .background(Color(.secondarySystemGroupedBackground),
                            in: RoundedRectangle(cornerRadius: 18, style: .continuous))
        }
    }

    // MARK: all bands

    /// One band: number, frequency, the system slider for gain, and the value.
    private func gainRow(_ b: EQBand) -> some View {
        let isSelected = b.index == selected
        let isOn = b.type != .off
        let color = bandColor(b.index)

        return HStack(spacing: 10) {
            Text("\(b.index + 1)")
                .font(.caption2.bold())
                .foregroundStyle(.white)
                .frame(width: 22, height: 22)
                .background(Circle().fill(color.opacity(isOn ? 0.95 : 0.45)))

            Text(b.frequency >= 20 ? freqLabel(b.frequency) : "–")
                .font(.caption.monospacedDigit())
                .foregroundStyle(.secondary)
                .frame(width: 42, alignment: .leading)

            Slider(
                value: Binding(
                    get: { controller.config.bands[b.index].gainDB },
                    set: { v in
                        // Moving a band's slider also makes it the selected band.
                        selected = b.index
                        update(b.index) { $0.gainDB = v }
                    }),
                in: -18...18,
                step: 0.5
            ) {
                Text("Band \(b.index + 1) gain")
            }
            .tint(color)
            .disabled(!isOn)
            .accessibilityValue(String(format: "%+.1f dB", b.gainDB))

            Text(isOn ? String(format: "%+.1f", b.gainDB) : "Off")
                .font(.caption.monospacedDigit())
                .foregroundStyle(isOn ? .primary : .secondary)
                .frame(width: 44, alignment: .trailing)
        }
        .padding(.vertical, 4)
        .padding(.horizontal, 8)
        .background(
            RoundedRectangle(cornerRadius: 12, style: .continuous)
                .fill(isSelected ? color.opacity(0.16) : Color.clear))
        .contentShape(Rectangle())
        .onTapGesture { selected = b.index }
    }

    // MARK: selected band

    private var selectedBand: some View {
        VStack(spacing: 14) {
            Picker("Type", selection: Binding(
                get: { band.type },
                set: { t in
                    update(selected) {
                        $0.type = t
                        // First time on: give it somewhere sensible to start.
                        if t != .off {
                            if $0.frequency < 20 { $0.frequency = startingFrequency($0.index) }
                            if $0.q < 0.2 { $0.q = 0.7 }
                        }
                    }
                })
            ) {
                ForEach(EQType.allCases) { Text($0.label).tag($0) }
            }
            .pickerStyle(.segmented)

            if band.type == .off {
                Text("Pick a filter type to switch this band on.")
                    .font(.footnote)
                    .foregroundStyle(.secondary)
                    .frame(maxWidth: .infinity, alignment: .leading)
            }

            VStack(spacing: 10) {
                // Frequency rides a log scale so the slider feels even across the band.
                ValueSlider(
                    title: "Frequency",
                    value: Binding(
                        get: { log10(max(20, controller.config.bands[selected].frequency)) },
                        set: { l in update(selected) { $0.frequency = pow(10, l).rounded() } }),
                    range: log10(20)...log10(20_000),
                    step: 0.01,
                    format: { freqLabel(pow(10, $0)) + " Hz" })

                ValueSlider(
                    title: "Q",
                    value: bind(selected, \.q),
                    range: 0.2...8,
                    step: 0.05,
                    format: { String(format: "%.2f", $0) })

                ValueSlider(
                    title: "Gain",
                    value: bind(selected, \.gainDB),
                    range: -18...18,
                    step: 0.5,
                    format: { String(format: "%+.1f dB", $0) })
            }
            .tint(bandColor(selected))
            .disabled(band.type == .off)
        }
    }
}
