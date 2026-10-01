import SwiftUI

/// Log-frequency response plot with a draggable handle per active band.
/// Drag a handle sideways for frequency, up and down for gain.
struct EQGraphView: View {
    let bands: [EQBand]
    let sampleRate: Double
    let crossoverHz: Double
    @Binding var selected: Int
    /// (band index, frequency, gain) as the handle moves.
    var onMove: (Int, Double, Double) -> Void

    static let fMin = 20.0
    static let fMax = 20_000.0
    static let dbRange = 18.0

    private static let gridFreqs: [(Double, String)] = [
        (50, "50"), (100, "100"), (500, "500"),
        (1_000, "1k"), (5_000, "5k"), (10_000, "10k"),
    ]

    // MARK: coordinate mapping

    static func x(_ f: Double, _ w: CGFloat) -> CGFloat {
        let lo = log10(fMin), hi = log10(fMax)
        return CGFloat((log10(min(max(f, fMin), fMax)) - lo) / (hi - lo)) * w
    }
    static func freq(_ x: CGFloat, _ w: CGFloat) -> Double {
        let lo = log10(fMin), hi = log10(fMax)
        let t = min(max(Double(x / max(w, 1)), 0), 1)
        return pow(10, lo + t * (hi - lo))
    }
    static func y(_ db: Double, _ h: CGFloat) -> CGFloat {
        CGFloat((dbRange - min(max(db, -dbRange), dbRange)) / (2 * dbRange)) * h
    }
    static func db(_ y: CGFloat, _ h: CGFloat) -> Double {
        let t = min(max(Double(y / max(h, 1)), 0), 1)
        return dbRange - t * 2 * dbRange
    }

    var body: some View {
        GeometryReader { geo in
            let w = geo.size.width, h = geo.size.height

            ZStack {
                Canvas { ctx, size in
                    draw(ctx: &ctx, w: size.width, h: size.height)
                }

                ForEach(bands.filter { $0.type != .off }) { band in
                    handle(band, w: w, h: h)
                }
            }
            .coordinateSpace(name: "graph")
        }
        .frame(height: 230)
        .background(Color.secondary.opacity(0.07))
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }

    // MARK: handle

    private func handle(_ band: EQBand, w: CGFloat, h: CGFloat) -> some View {
        let isSel = band.index == selected
        return Circle()
            .fill(color(band.index).opacity(isSel ? 0.95 : 0.6))
            .overlay(Circle().strokeBorder(.white.opacity(0.9), lineWidth: isSel ? 2 : 1))
            .overlay(
                Text("\(band.index + 1)")
                    .font(.caption2.bold())
                    .foregroundStyle(.white)
            )
            .frame(width: isSel ? 30 : 24, height: isSel ? 30 : 24)
            .position(x: Self.x(band.frequency, w), y: Self.y(band.gainDB, h))
            .gesture(
                DragGesture(minimumDistance: 0, coordinateSpace: .named("graph"))
                    .onChanged { g in
                        selected = band.index
                        onMove(band.index,
                               Self.freq(g.location.x, w),
                               Self.db(g.location.y, h))
                    }
            )
    }

    private func color(_ i: Int) -> Color {
        let hues: [Double] = [0.00, 0.08, 0.14, 0.33, 0.48, 0.58, 0.72, 0.85]
        return Color(hue: hues[i % hues.count], saturation: 0.75, brightness: 0.92)
    }

    // MARK: drawing

    private func draw(ctx: inout GraphicsContext, w: CGFloat, h: CGFloat) {
        // gain grid
        for db in stride(from: -12.0, through: 12.0, by: 6.0) {
            var p = Path()
            let yy = Self.y(db, h)
            p.move(to: .init(x: 0, y: yy)); p.addLine(to: .init(x: w, y: yy))
            ctx.stroke(p, with: .color(.secondary.opacity(db == 0 ? 0.45 : 0.15)),
                       lineWidth: db == 0 ? 1.2 : 0.5)
            if db != 0 {
                ctx.draw(Text("\(Int(db))").font(.system(size: 8))
                            .foregroundStyle(.secondary),
                         at: .init(x: 13, y: yy - 6))
            }
        }

        // frequency grid
        for (f, label) in Self.gridFreqs {
            var p = Path()
            let xx = Self.x(f, w)
            p.move(to: .init(x: xx, y: 0)); p.addLine(to: .init(x: xx, y: h))
            ctx.stroke(p, with: .color(.secondary.opacity(0.15)), lineWidth: 0.5)
            ctx.draw(Text(label).font(.system(size: 8)).foregroundStyle(.secondary),
                     at: .init(x: xx, y: h - 8))
        }

        // crossover branches, so the EQ can be read against where the ways split
        for isHigh in [false, true] {
            var p = Path()
            var first = true
            for px in stride(from: 0.0, through: Double(w), by: 2.0) {
                let f = Self.freq(CGFloat(px), w)
                let db = EQResponse.crossoverDB(crossoverHz, at: f,
                                                sampleRate: sampleRate, highPass: isHigh)
                let pt = CGPoint(x: px, y: Self.y(db, h))
                if first { p.move(to: pt); first = false } else { p.addLine(to: pt) }
            }
            ctx.stroke(p, with: .color(.secondary.opacity(0.35)),
                       style: .init(lineWidth: 1, dash: [3, 3]))
        }

        // the selected band on its own
        if selected < bands.count, bands[selected].type != .off {
            var p = Path()
            var first = true
            for px in stride(from: 0.0, through: Double(w), by: 2.0) {
                let f = Self.freq(CGFloat(px), w)
                let db = EQResponse.bandDB(bands[selected], at: f, sampleRate: sampleRate)
                let pt = CGPoint(x: px, y: Self.y(db, h))
                if first { p.move(to: pt); first = false } else { p.addLine(to: pt) }
            }
            ctx.stroke(p, with: .color(color(selected).opacity(0.55)), lineWidth: 1.5)
        }

        // combined response
        var total = Path()
        var first = true
        for px in stride(from: 0.0, through: Double(w), by: 1.0) {
            let f = Self.freq(CGFloat(px), w)
            let db = EQResponse.totalDB(bands: bands, at: f, sampleRate: sampleRate)
            let pt = CGPoint(x: px, y: Self.y(db, h))
            if first { total.move(to: pt); first = false } else { total.addLine(to: pt) }
        }
        ctx.stroke(total, with: .color(.accentColor), lineWidth: 2.5)
    }
}
