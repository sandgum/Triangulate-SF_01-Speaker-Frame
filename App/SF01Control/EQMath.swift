//
//  EQMath.swift
//  Frequency-response maths for the EQ graph.
//
//  These are the same RBJ cookbook filters the firmware runs in dsp.c, so the
//  drawn curve matches what you hear rather than approximating it.
//

import Foundation

struct Biquad {
    var b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0

    /// |H(e^jw)| in dB at `freq`.
    func magnitudeDB(at freq: Double, sampleRate: Double) -> Double {
        let w = 2 * .pi * freq / sampleRate
        let cw = cos(w), c2w = cos(2 * w)
        let num = b0*b0 + b1*b1 + b2*b2 + 2*(b0*b1 + b1*b2)*cw + 2*b0*b2*c2w
        let den = 1 + a1*a1 + a2*a2 + 2*(a1 + a1*a2)*cw + 2*a2*c2w
        guard den > 0, num >= 0 else { return 0 }
        return 10 * log10(max(num / den, 1e-12))
    }

    static func peaking(_ f0: Double, _ q: Double, _ gainDB: Double, _ fs: Double) -> Biquad {
        let A = pow(10, gainDB / 40)
        let w = 2 * .pi * f0 / fs
        let cw = cos(w), alpha = sin(w) / (2 * q)
        let a0 = 1 + alpha / A
        return Biquad(b0: (1 + alpha * A) / a0,
                      b1: (-2 * cw) / a0,
                      b2: (1 - alpha * A) / a0,
                      a1: (-2 * cw) / a0,
                      a2: (1 - alpha / A) / a0)
    }

    static func lowShelf(_ f0: Double, _ q: Double, _ gainDB: Double, _ fs: Double) -> Biquad {
        let A = pow(10, gainDB / 40)
        let w = 2 * .pi * f0 / fs
        let cw = cos(w), alpha = sin(w) / (2 * q)
        let sq = 2 * sqrt(A) * alpha
        let a0 = (A + 1) + (A - 1) * cw + sq
        return Biquad(b0: (A * ((A + 1) - (A - 1) * cw + sq)) / a0,
                      b1: (2 * A * ((A - 1) - (A + 1) * cw)) / a0,
                      b2: (A * ((A + 1) - (A - 1) * cw - sq)) / a0,
                      a1: (-2 * ((A - 1) + (A + 1) * cw)) / a0,
                      a2: ((A + 1) + (A - 1) * cw - sq) / a0)
    }

    static func highShelf(_ f0: Double, _ q: Double, _ gainDB: Double, _ fs: Double) -> Biquad {
        let A = pow(10, gainDB / 40)
        let w = 2 * .pi * f0 / fs
        let cw = cos(w), alpha = sin(w) / (2 * q)
        let sq = 2 * sqrt(A) * alpha
        let a0 = (A + 1) - (A - 1) * cw + sq
        return Biquad(b0: (A * ((A + 1) + (A - 1) * cw + sq)) / a0,
                      b1: (-2 * A * ((A - 1) + (A + 1) * cw)) / a0,
                      b2: (A * ((A + 1) + (A - 1) * cw - sq)) / a0,
                      a1: (2 * ((A - 1) - (A + 1) * cw)) / a0,
                      a2: ((A + 1) - (A - 1) * cw - sq) / a0)
    }

    static func lowPass(_ f0: Double, _ q: Double, _ fs: Double) -> Biquad {
        let w = 2 * .pi * f0 / fs
        let cw = cos(w), alpha = sin(w) / (2 * q)
        let a0 = 1 + alpha
        return Biquad(b0: ((1 - cw) / 2) / a0, b1: (1 - cw) / a0,
                      b2: ((1 - cw) / 2) / a0, a1: (-2 * cw) / a0,
                      a2: (1 - alpha) / a0)
    }

    static func highPass(_ f0: Double, _ q: Double, _ fs: Double) -> Biquad {
        let w = 2 * .pi * f0 / fs
        let cw = cos(w), alpha = sin(w) / (2 * q)
        let a0 = 1 + alpha
        return Biquad(b0: ((1 + cw) / 2) / a0, b1: (-(1 + cw)) / a0,
                      b2: ((1 + cw) / 2) / a0, a1: (-2 * cw) / a0,
                      a2: (1 - alpha) / a0)
    }
}

enum EQResponse {
    static let butterworthQ = 0.70710678

    static func biquad(for band: EQBand, sampleRate: Double) -> Biquad? {
        guard band.type != .off else { return nil }
        let f = min(max(band.frequency, 20), sampleRate * 0.45)
        let q = min(max(band.q, 0.1), 10)
        switch band.type {
        case .peaking:   return .peaking(f, q, band.gainDB, sampleRate)
        case .lowShelf:  return .lowShelf(f, q, band.gainDB, sampleRate)
        case .highShelf: return .highShelf(f, q, band.gainDB, sampleRate)
        case .off:       return nil
        }
    }

    /// Combined EQ response in dB — the sum of every active band.
    static func totalDB(bands: [EQBand], at freq: Double, sampleRate: Double) -> Double {
        bands.reduce(0) { sum, band in
            guard let bq = biquad(for: band, sampleRate: sampleRate) else { return sum }
            return sum + bq.magnitudeDB(at: freq, sampleRate: sampleRate)
        }
    }

    /// One band on its own, for the highlight behind the selected handle.
    static func bandDB(_ band: EQBand, at freq: Double, sampleRate: Double) -> Double {
        guard let bq = biquad(for: band, sampleRate: sampleRate) else { return 0 }
        return bq.magnitudeDB(at: freq, sampleRate: sampleRate)
    }

    /// Linkwitz-Riley 4th order = two cascaded Butterworth sections.
    static func crossoverDB(_ fx: Double, at freq: Double, sampleRate: Double,
                            highPass: Bool) -> Double {
        let bq = highPass ? Biquad.highPass(fx, butterworthQ, sampleRate)
                          : Biquad.lowPass(fx, butterworthQ, sampleRate)
        return 2 * bq.magnitudeDB(at: freq, sampleRate: sampleRate)
    }
}
