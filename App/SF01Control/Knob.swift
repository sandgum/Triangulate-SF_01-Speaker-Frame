import SwiftUI

/// Rotary knob driven by a vertical drag — dragging up increases.
///
/// Vertical drag rather than true circular tracking: on a phone the finger
/// covers the knob, and chasing an angle under your own thumb is fiddly.
struct Knob: View {
    let label: String
    let caption: String
    @Binding var value: Double
    let range: ClosedRange<Double>
    var accent: Color = .accentColor
    var isSelected = false
    var isEnabled = true
    var onSelect: (() -> Void)?

    /// Points of vertical travel for the full range.
    private let travel: CGFloat = 160
    private let sweep: Double = 0.75          // 270° of a full turn
    private let startAngle: Double = 135

    @State private var dragAnchor: Double?

    private var fraction: Double {
        let span = range.upperBound - range.lowerBound
        guard span > 0 else { return 0 }
        return min(max((value - range.lowerBound) / span, 0), 1)
    }

    var body: some View {
        VStack(spacing: 4) {
            ZStack {
                Circle()
                    .trim(from: 0, to: sweep)
                    .stroke(Color.secondary.opacity(0.22),
                            style: .init(lineWidth: 5, lineCap: .round))
                    .rotationEffect(.degrees(startAngle))

                Circle()
                    .trim(from: 0, to: sweep * fraction)
                    .stroke(isEnabled ? accent : Color.secondary.opacity(0.4),
                            style: .init(lineWidth: 5, lineCap: .round))
                    .rotationEffect(.degrees(startAngle))

                // pointer
                Capsule()
                    .fill(isEnabled ? accent : Color.secondary)
                    .frame(width: 2.5, height: 11)
                    .offset(y: -12)
                    .rotationEffect(.degrees(startAngle + 90 + sweep * 360 * fraction))

                Circle()
                    .strokeBorder(isSelected ? accent : .clear, lineWidth: 2)
                    .padding(-5)
            }
            .frame(width: 52, height: 52)
            .contentShape(Circle())
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { g in
                        guard isEnabled else { return }
                        if dragAnchor == nil {
                            dragAnchor = value
                            onSelect?()
                        }
                        let span = range.upperBound - range.lowerBound
                        let delta = Double(-g.translation.height / travel) * span
                        value = min(max((dragAnchor ?? value) + delta,
                                        range.lowerBound), range.upperBound)
                    }
                    .onEnded { _ in dragAnchor = nil }
            )

            Text(label)
                .font(.caption2.weight(.medium))
                .foregroundStyle(isEnabled ? .primary : .secondary)
            Text(caption)
                .font(.caption2.monospacedDigit())
                .foregroundStyle(.secondary)
        }
        .opacity(isEnabled ? 1 : 0.55)
    }
}
