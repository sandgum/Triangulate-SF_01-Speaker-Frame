import SwiftUI
import UIKit

/// Main page: the speaker itself, its connection state, and the controls that
/// are not the equaliser — output, crossover, levels and saved tuning.
struct SpeakerScreen: View {
    @EnvironmentObject private var controller: SpeakerController

    @State private var confirmingReset = false
    @State private var justSaved = false
    @State private var saveCount = 0

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
                heroSection

                Group {
                    outputSection
                    crossoverSection
                    levelsSection
                    tuningSection
                }
                .disabled(!ready)
            }
            .navigationTitle("SF_01")
            .animation(.default, value: controller.state)
            .sensoryFeedback(.success, trigger: saveCount)
            .confirmationDialog("Reset every setting to its default?",
                                isPresented: $confirmingReset,
                                titleVisibility: .visible) {
                Button("Reset to defaults", role: .destructive) {
                    controller.send(.defaults)
                    controller.refreshSoon()
                }
                Button("Cancel", role: .cancel) {}
            } message: {
                Text("This also erases the tuning saved in the speaker.")
            }
        }
    }

    // MARK: hero

    private var heroSection: some View {
        Section {
            Image("SF01Render")
                .resizable()
                .aspectRatio(3.0 / 2.0, contentMode: .fit)
                .accessibilityLabel("SF_01 speaker frame")
                .overlay(alignment: .topLeading) { statusBadge.padding(12) }
                .overlay(alignment: .topTrailing) {
                    if ready {
                        Text(String(format: "%.1f kHz", Double(cfg.sampleRate) / 1000))
                            .monospacedDigit()
                            .modifier(BadgeStyle())
                            .padding(12)
                    }
                }
                .clipShape(RoundedRectangle(cornerRadius: 22, style: .continuous))
                .listRowInsets(EdgeInsets())
                .listRowBackground(Color.clear)
        } footer: {
            connectionHint
        }
    }

    private var statusBadge: some View {
        HStack(spacing: 6) {
            switch controller.state {
            case .ready:
                Circle().fill(.green).frame(width: 8, height: 8)
            case .scanning, .connecting:
                ProgressView().controlSize(.mini)
            case .bluetoothOff, .unauthorized, .disconnected:
                Image(systemName: "exclamationmark.triangle.fill")
                    .foregroundStyle(.orange)
            }
            Text(controller.state.label)
        }
        .modifier(BadgeStyle())
    }

    @ViewBuilder private var connectionHint: some View {
        switch controller.state {
        case .scanning, .connecting:
            Text("Make sure the speaker is powered on and in range.")
        case .bluetoothOff:
            Text("Turn on Bluetooth to control the speaker.")
        case .unauthorized:
            VStack(alignment: .leading, spacing: 6) {
                Text("This app needs Bluetooth access to reach the speaker.")
                Button("Open Settings") {
                    if let url = URL(string: UIApplication.openSettingsURLString) {
                        UIApplication.shared.open(url)
                    }
                }
                .font(.footnote.weight(.semibold))
            }
        case .ready, .disconnected:
            EmptyView()
        }
    }

    // MARK: sections

    private var outputSection: some View {
        Section("Output") {
            Toggle(isOn: Binding(
                get: { cfg.outputLive },
                set: { on in
                    controller.config.outputLive = on
                    controller.send(.mute(!on))
                })) {
                RowLabel("Amplifiers live", systemImage: "power")
            }

            ValueSlider(
                title: "Volume",
                systemImage: "speaker.wave.2.fill",
                value: Binding(
                    get: { Double(cfg.volume) },
                    set: { v in
                        controller.config.volume = Int(v)
                        controller.send(.volume(Int(v)))
                    }),
                range: 0...127, step: 1,
                minimumIcon: "speaker.fill",
                maximumIcon: "speaker.wave.3.fill",
                format: { String(format: "%.0f", $0 / 127 * 100) + "%" })
        }
    }

    private var crossoverSection: some View {
        Section {
            ValueSlider(
                title: "Frequency",
                systemImage: "arrow.triangle.branch",
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
                        systemImage: "speaker.wave.1.fill",
                        value: bind(\.wooferGainDB) { .gain(target: .woofer, db: $0) },
                        range: -12...12, step: 0.5,
                        format: { String(format: "%+.1f dB", $0) })

            ValueSlider(title: "Tweeters",
                        systemImage: "speaker.wave.3.fill",
                        value: bind(\.tweeterGainDB) { .gain(target: .tweeter, db: $0) },
                        range: -12...12, step: 0.5,
                        format: { String(format: "%+.1f dB", $0) })

            ValueSlider(title: "Master",
                        systemImage: "plusminus.circle",
                        value: bind(\.masterGainDB) { .gain(target: .master, db: $0) },
                        range: -20...6, step: 0.5,
                        format: { String(format: "%+.1f dB", $0) })

            Toggle(isOn: Binding(
                get: { cfg.tweeterInverted },
                set: { on in
                    controller.config.tweeterInverted = on
                    controller.send(.invert(on))
                })) {
                RowLabel("Invert tweeter polarity", systemImage: "arrow.up.arrow.down")
            }

            Stepper(value: Binding(
                get: { cfg.tweeterDelay },
                set: { n in
                    controller.config.tweeterDelay = n
                    controller.send(.delay(n))
                }), in: 0...63) {
                    HStack(spacing: 12) {
                        Image(systemName: "timer")
                            .foregroundStyle(.tint)
                            .frame(width: 26)
                        VStack(alignment: .leading, spacing: 1) {
                            Text("Tweeter delay")
                            Text(delaySummary)
                                .font(.caption)
                                .foregroundStyle(.secondary)
                                .monospacedDigit()
                        }
                    }
                }
        } header: {
            Text("Levels & alignment")
        } footer: {
            Text("One sample of delay is roughly 8 mm of path length at 44.1 kHz.")
        }
    }

    private var delaySummary: String {
        let n = cfg.tweeterDelay
        guard n > 0, cfg.sampleRate > 0 else { return "None" }
        let micros = Double(n) / Double(cfg.sampleRate) * 1_000_000
        return "\(n) sample\(n == 1 ? "" : "s") · \(String(format: "%.0f", micros)) µs"
    }

    private var tuningSection: some View {
        Section {
            Button {
                controller.send(.save)
                saveCount += 1
                withAnimation { justSaved = true }
                Task {
                    try? await Task.sleep(for: .seconds(2))
                    withAnimation { justSaved = false }
                }
            } label: {
                RowLabel(justSaved ? "Saved" : "Save to speaker",
                         systemImage: justSaved ? "checkmark.circle.fill"
                                                : "square.and.arrow.down",
                         tintIcon: false)
            }

            Button {
                controller.send(.load)
                controller.refreshSoon()
            } label: {
                RowLabel("Reload saved", systemImage: "arrow.counterclockwise",
                         tintIcon: false)
            }

            Button(role: .destructive) {
                confirmingReset = true
            } label: {
                RowLabel("Reset to defaults", systemImage: "trash", tintIcon: false)
            }
        } header: {
            Text("Saved tuning")
        } footer: {
            Text("Saved settings are kept in the speaker's flash and restored on power-up.")
        }
    }
}
