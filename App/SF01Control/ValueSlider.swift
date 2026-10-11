import SwiftUI

/// A labelled system slider that shows its current value on the trailing edge.
struct ValueSlider: View {
    let title: String
    var systemImage: String? = nil
    @Binding var value: Double
    let range: ClosedRange<Double>
    var step: Double = 0.1
    /// Optional SF Symbols drawn at the two ends of the track.
    var minimumIcon: String? = nil
    var maximumIcon: String? = nil
    var format: (Double) -> String

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                if let systemImage {
                    RowLabel(title, systemImage: systemImage)
                } else {
                    Text(title)
                }
                Spacer()
                Text(format(value))
                    .foregroundStyle(.secondary)
                    .monospacedDigit()
            }
            slider
                .accessibilityValue(format(value))
        }
        .padding(.vertical, 2)
    }

    @ViewBuilder private var slider: some View {
        if let minimumIcon, let maximumIcon {
            Slider(value: $value, in: range, step: step) {
                Text(title)
            } minimumValueLabel: {
                Image(systemName: minimumIcon).foregroundStyle(.secondary)
            } maximumValueLabel: {
                Image(systemName: maximumIcon).foregroundStyle(.secondary)
            }
        } else {
            Slider(value: $value, in: range, step: step) {
                Text(title)
            }
        }
    }
}
