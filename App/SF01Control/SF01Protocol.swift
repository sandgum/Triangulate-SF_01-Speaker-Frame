//
//  SF01Protocol.swift
//  Wire format for the SF_01 BLE control service.
//
//  This mirrors main/ble_ctl.h in the firmware byte for byte. If you change
//  one side, change the other — SF01Config.protocolVersion is checked on
//  every read so a mismatch surfaces immediately instead of as odd values.
//

import Foundation
import CoreBluetooth

enum SF01 {
    static let service = CBUUID(string: "5F010000-A1B2-4C3D-8E4F-5F0100000001")
    static let config  = CBUUID(string: "5F010001-A1B2-4C3D-8E4F-5F0100000001")
    static let command = CBUUID(string: "5F010002-A1B2-4C3D-8E4F-5F0100000001")

    static let protocolVersion: UInt8 = 1
    static let configLength = 78
    static let bandCount = 8
}

enum EQType: UInt8, CaseIterable, Identifiable {
    case off = 0, peaking = 1, lowShelf = 2, highShelf = 3
    var id: UInt8 { rawValue }
    var label: String {
        switch self {
        case .off:       return "Off"
        case .peaking:   return "Peak"
        case .lowShelf:  return "Low shelf"
        case .highShelf: return "High shelf"
        }
    }
}

struct EQBand: Equatable, Identifiable {
    var index: Int
    var type: EQType = .off
    var frequency: Double = 1000
    var q: Double = 0.7
    var gainDB: Double = 0
    var id: Int { index }
}

struct SF01Config: Equatable {
    var crossoverHz: Double = 2800
    var volume: Int = 96
    var tweeterDelay: Int = 0
    var wooferGainDB: Double = 0
    var tweeterGainDB: Double = -3
    var masterGainDB: Double = -3
    var sampleRate: Int = 44100
    var tweeterInverted: Bool = false
    var outputLive: Bool = false
    var bands: [EQBand] = (0..<SF01.bandCount).map { EQBand(index: $0) }

    // MARK: decode

    init() {}

    init?(_ d: Data) {
        guard d.count >= SF01.configLength, d[0] == SF01.protocolVersion else { return nil }
        func u16(_ o: Int) -> Int { Int(d[o]) | Int(d[o + 1]) << 8 }
        func i16(_ o: Int) -> Int { let v = u16(o); return v > 32_767 ? v - 65_536 : v }

        let flags = d[1]
        tweeterInverted = flags & 0x01 != 0
        outputLive      = flags & 0x02 != 0
        crossoverHz     = Double(u16(2))
        volume          = Int(d[4])
        tweeterDelay    = Int(d[5])
        wooferGainDB    = Double(i16(6))  / 10
        tweeterGainDB   = Double(i16(8))  / 10
        masterGainDB    = Double(i16(10)) / 10
        sampleRate      = u16(12) * 100

        bands = (0..<SF01.bandCount).map { i in
            let b = 14 + i * 8
            return EQBand(index: i,
                          type: EQType(rawValue: d[b]) ?? .off,
                          frequency: Double(u16(b + 2)),
                          q: Double(u16(b + 4)) / 100,
                          gainDB: Double(i16(b + 6)) / 10)
        }
    }
}

// MARK: - command frames

enum SF01Command {
    case crossover(Double)
    case volume(Int)
    case gain(target: GainTarget, db: Double)
    case band(EQBand)
    case invert(Bool)
    case delay(Int)
    case save, load, defaults
    case mute(Bool)

    enum GainTarget: UInt8 { case woofer = 0, tweeter = 1, master = 2 }

    /// Writes for the same control coalesce while a slider is in motion, so
    /// a drag sends the latest value rather than every intermediate one.
    var coalesceKey: String {
        switch self {
        case .crossover:           return "xo"
        case .volume:              return "vol"
        case .gain(let t, _):      return "gain\(t.rawValue)"
        case .band(let b):         return "band\(b.index)"
        case .invert:              return "inv"
        case .delay:               return "delay"
        case .save:                return "save"
        case .load:                return "load"
        case .defaults:            return "defaults"
        case .mute:                return "mute"
        }
    }

    var data: Data {
        func u16(_ v: Int) -> [UInt8] {
            let c = UInt16(clamping: v); return [UInt8(c & 0xFF), UInt8(c >> 8)]
        }
        func i16(_ v: Double) -> [UInt8] {
            let c = UInt16(bitPattern: Int16(clamping: Int(v.rounded())))
            return [UInt8(c & 0xFF), UInt8(c >> 8)]
        }
        switch self {
        case .crossover(let hz):    return Data([0x01] + u16(Int(hz.rounded())))
        case .volume(let v):        return Data([0x02, UInt8(clamping: v)])
        case .gain(let t, let db):  return Data([0x03, t.rawValue] + i16(db * 10))
        case .band(let b):
            return Data([0x04, UInt8(b.index), b.type.rawValue]
                        + u16(Int(b.frequency.rounded()))
                        + u16(Int((b.q * 100).rounded()))
                        + i16(b.gainDB * 10))
        case .invert(let on):       return Data([0x05, on ? 1 : 0])
        case .delay(let n):         return Data([0x06, UInt8(clamping: n)])
        case .save:                 return Data([0x07])
        case .load:                 return Data([0x08])
        case .defaults:             return Data([0x09])
        case .mute(let on):         return Data([0x0A, on ? 1 : 0])
        }
    }
}
