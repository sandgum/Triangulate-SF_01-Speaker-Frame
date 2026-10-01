import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var controller: SpeakerController

    private var cfg: SF01Config { controller.config }
    private var ready: Bool { controller.state.isReady }

    // Each control writes through to the speaker as it changes.
    private func bind(_ path: WritableKeyPath<SF01Config, Double>,
                      _ command: @escaping (Double) -> SF01Command) -> Binding<Double> {
        Binding(get: { controller.config[keyPath: path] },
                set: { v in
                    controller.config[keyPath: path] = v
                    controller.send(command(v))
                })
    }

    var body: some View {
        NavigationStack {
            List {
                statusSection
                outputSection
                crossoverSection
                levelsSection
                equaliserSection
                presetsSection
            }
            .navigationTitle("SF_01")
            .disabled(!ready)
            .animation(.default, value: ready)
        }
    }

    // MARK: sections

    private var statusSection: some View {
        Section {
            HStack(spacing: 10) {
                Circle()
                    .fill(ready ? .green : .orange)
                    .frame(width: 10, height: 10)
                VStack(alignment: .leading, spacing: 1) {
                    Text(controller.deviceName ?? "SF_01 Speaker")
                        .font(.headline)
                    Text(controller.state.label)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                Spacer()
                if ready {
                    Text("\(cfg.sampleRate / 1000) kHz")
                        .font(.caption.monospacedDigit())
                        .foregroundStyle(.secondary)
                } else {
                    ProgressView()
                }
            }
            .padding(.vertical, 2)
        }
    }

    private var outputSection: some View {
        Section("Output") {
            Toggle("Amplifiers live", isOn: Binding(
                get: { cfg.outputLive },
                set: { on in
                    controller.config.outputLive = on
                    controller.send(.mute(!on))
                }))

            ValueSlider(
                title: "Volume",
                value: Binding(
                    get: { Double(cfg.volume) },
                    set: { v in
                        controller.config.volume = Int(v)
                        controller.send(.volume(Int(v)))
                    }),
                range: 0...127, step: 1,
                format: { String(format: "%.0f", $0 / 127 * 100) + "%" })
        }
    }

    private var crossoverSection: some View {
        Section {
            ValueSlider(
                title: "Frequency",
                value: Binding(
                    get: { log10(max(200, cfg.crossoverHz)) },
                    set: { l in
                        let hz = pow(10, l).rounded()
                        controller.config.crossoverHz = hz
                        controller.send(.crossover(hz))
                    }),
                range: log10(200)...log10(8000), step: 0.005,
                format: { l in
                    let f = pow(10, l)
                    return f >= 1000 ? String(format: "%.2f kHz", f / 1000)
                                     : String(format: "%.0f Hz", f)
                })
        } header: {
            Text("Crossover")
        } footer: {
            Text("Linkwitz-Riley 4th order, 24 dB/octave. Low branch drives the mid-bass woofers, high branch the tweeters.")
        }
    }

    private var levelsSection: some View {
        Section {
            ValueSlider(title: "Woofers",
                        value: bind(\.wooferGainDB) { .gain(target: .woofer, db: $0) },
                        range: -12...12, step: 0.5,
                        format: { String(format: "%+.1f dB", $0) })

            ValueSlider(title: "Tweeters",
                        value: bind(\.tweeterGainDB) { .gain(target: .tweeter, db: $0) },
                        range: -12...12, step: 0.5,
                        format: { String(format: "%+.1f dB", $0) })

            ValueSlider(title: "Master",
                        value: bind(\.masterGainDB) { .gain(target: .master, db: $0) },
                        range: -20...6, step: 0.5,
                        format: { String(format: "%+.1f dB", $0) })

            Toggle("Invert tweeter polarity", isOn: Binding(
                get: { cfg.tweeterInverted },
                set: { on in
                    controller.config.tweeterInverted = on
                    controller.send(.invert(on))
                }))

            Stepper(value: Binding(
                get: { cfg.tweeterDelay },
                set: { n in
                    controller.config.tweeterDelay = n
                    controller.send(.delay(n))
                }), in: 0...63) {
                    HStack {
                        Text("Tweeter delay")
                        Spacer()
                        Text("\(cfg.tweeterDelay) smp")
                            .foregroundStyle(.secondary)
                            .monospacedDigit()
                    }
                    .font(.subheadline)
                }
        } header: {
            Text("Levels & alignment")
        } footer: {
            Text("One sample of delay is about 23 µs at 44.1 kHz, roughly 8 mm of path length.")
        }
    }

    private var equaliserSection: some View {
        Section("Equaliser") {
            NavigationLink {
                EQScreen()
            } label: {
                HStack {
                    Label("Response & knobs", systemImage: "slider.horizontal.3")
                    Spacer()
                    Text(activeBandSummary)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            ForEach(cfg.bands.filter { $0.type != .off }) { band in
                NavigationLink {
                    BandEditor(index: band.index)
                } label: {
                    HStack {
                        Text("Band \(band.index + 1)")
                        Spacer()
                        Text(summary(band))
                            .foregroundStyle(.secondary)
                            .monospacedDigit()
                    }
                    .font(.subheadline)
                }
            }
        }
    }

    private var activeBandSummary: String {
        let n = cfg.bands.filter { $0.type != .off }.count
        return n == 0 ? "all flat" : "\(n) active"
    }

    private func summary(_ b: EQBand) -> String {
        let f = b.frequency >= 1000
            ? String(format: "%.1fk", b.frequency / 1000)
            : String(format: "%.0f", b.frequency)
        return "\(b.type.label) · \(f)Hz · \(String(format: "%+.1f", b.gainDB))dB"
    }

    private var presetsSection: some View {
        Section {
            Button("Save to speaker") {
                controller.send(.save)
            }
            Button("Reload saved") {
                controller.send(.load)
                controller.refreshSoon()
            }
            Button("Reset to defaults", role: .destructive) {
                controller.send(.defaults)
                controller.refreshSoon()
            }
        } footer: {
            Text("Saved settings are kept in the speaker's flash and restored on power-up.")
        }
    }
}
