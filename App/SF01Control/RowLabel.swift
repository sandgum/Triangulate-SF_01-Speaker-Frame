import SwiftUI

/// Icon-and-title used at the leading edge of every settings row, so toggles,
/// sliders, steppers and buttons all line up on the same icon column.
struct RowLabel: View {
    let title: String
    let systemImage: String
    /// Buttons leave this off so the icon follows the button's own colour
    /// (accent, or red for destructive actions).
    var tintIcon = true

    init(_ title: String, systemImage: String, tintIcon: Bool = true) {
        self.title = title
        self.systemImage = systemImage
        self.tintIcon = tintIcon
    }

    var body: some View {
        HStack(spacing: 12) {
            icon.frame(width: 26)
            Text(title)
        }
    }

    @ViewBuilder private var icon: some View {
        if tintIcon {
            Image(systemName: systemImage).foregroundStyle(.tint)
        } else {
            Image(systemName: systemImage)
        }
    }
}

/// Small floating badge. Liquid Glass where the system has it, a material
/// capsule on older releases.
struct BadgeStyle: ViewModifier {
    func body(content: Content) -> some View {
        let padded = content
            .font(.caption.weight(.semibold))
            .padding(.horizontal, 10)
            .padding(.vertical, 6)

        if #available(iOS 26.0, *) {
            padded.glassEffect()
        } else {
            padded.background(.regularMaterial, in: Capsule())
        }
    }
}
