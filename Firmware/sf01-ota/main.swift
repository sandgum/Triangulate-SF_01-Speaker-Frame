//
//  sf01-ota — pushes a firmware image to the SF_01 speaker over Bluetooth LE.
//
//  The wire protocol is specified in ../SF_01 ESP32 Firmware/main/fw_update.h.
//
//  This is packaged as an .app bundle and started through LaunchServices
//  (`open`) rather than run as a bare binary. macOS attributes a Bluetooth
//  request to the "responsible" process; for a bare binary that is whatever
//  spawned it, and if that parent has no Bluetooth usage string the system
//  kills the child outright. As its own app it gets its own, one-time
//  permission prompt instead.
//

import Foundation
import CoreBluetooth

// MARK: - protocol constants

enum SF01 {
    static let service = CBUUID(string: "5F010000-A1B2-4C3D-8E4F-5F0100000001")
    static let fwCtrl  = CBUUID(string: "5F010003-A1B2-4C3D-8E4F-5F0100000001")
    static let fwData  = CBUUID(string: "5F010004-A1B2-4C3D-8E4F-5F0100000001")
    static let fwInfo  = CBUUID(string: "5F010005-A1B2-4C3D-8E4F-5F0100000001")

    static let cmdBegin: UInt8 = 0x01, cmdAbort: UInt8 = 0x03, cmdConfirm: UInt8 = 0x04
    static let evtReady: UInt8 = 0x01, evtAck: UInt8 = 0x02, evtDone: UInt8 = 0x03
    static let evtConfirmed: UInt8 = 0x04, evtError: UInt8 = 0x7F

    static let projectName = "sf01_speaker"

    static func errorText(_ code: UInt8) -> String {
        switch code {
        case 1:  return "speaker rejected the command"
        case 2:  return "image does not fit the update partition"
        case 3:  return "speaker could not open the update partition"
        case 4:  return "speaker's receive buffer overflowed"
        case 5:  return "flash write failed"
        case 6:  return "speaker timed out waiting for data"
        case 7:  return "image failed verification (corrupt, or not signed with this speaker's key)"
        case 8:  return "speaker could not select the new image"
        case 9:  return "an update is already in progress"
        case 10: return "update aborted"
        default: return "speaker error \(code)"
        }
    }
}

// MARK: - logging

final class Log {
    static var handle: FileHandle?
    static let start = Date()

    static func open(_ path: String?) {
        guard let path else { return }
        FileManager.default.createFile(atPath: path, contents: nil)
        handle = FileHandle(forWritingAtPath: path)
    }
    static func line(_ s: String) {
        let t = String(format: "[%6.1fs] ", Date().timeIntervalSince(start))
        let out = t + s + "\n"
        FileHandle.standardOutput.write(out.data(using: .utf8)!)
        handle?.write(out.data(using: .utf8)!)
    }
    static func finish(ok: Bool, _ why: String) -> Never {
        line(ok ? "RESULT: OK — \(why)" : "RESULT: FAIL — \(why)")
        try? handle?.synchronize()
        exit(ok ? 0 : 1)
    }
}

// MARK: - firmware image

struct ImageInfo {
    let version: String, project: String, date: String, time: String
    let elfSHA: Data            // 32 bytes

    /// esp_app_desc_t sits right after the image header and first segment header.
    init?(_ bin: Data) {
        guard bin.count > 256, bin[0] == 0xE9 else { return nil }
        let d = 32
        guard bin[d] == 0x32, bin[d+1] == 0x54, bin[d+2] == 0xCD, bin[d+3] == 0xAB else { return nil }
        func str(_ off: Int, _ len: Int) -> String {
            let raw = bin.subdata(in: (d + off)..<(d + off + len))
            return String(decoding: raw.prefix { $0 != 0 }, as: UTF8.self)
        }
        version = str(16, 32); project = str(48, 32)
        time = str(80, 16);    date = str(96, 16)
        elfSHA = bin.subdata(in: (d + 144)..<(d + 176))
    }
    var buildID: String { elfSHA.prefix(8).map { String(format: "%02x", $0) }.joined() }
}

struct DeviceInfo {
    let otaState: UInt8, pending: Bool, slot: UInt8
    let buildID: String, version: String, date: String, time: String

    init?(_ d: Data) {
        guard d.count >= 80, d[d.startIndex] == 1 else { return nil }
        let b = [UInt8](d)
        otaState = b[1]; pending = b[2] & 0x01 != 0; slot = b[3]
        buildID = b[8..<16].map { String(format: "%02x", $0) }.joined()
        func str(_ r: Range<Int>) -> String {
            String(decoding: b[r].prefix { $0 != 0 }, as: UTF8.self)
        }
        version = str(16..<48); date = str(48..<64); time = str(64..<80)
    }
    var summary: String {
        "build \(buildID) (\(version), \(date) \(time)), slot ota_\(slot)" +
        (pending ? ", AWAITING CONFIRMATION" : "")
    }
}

// MARK: - updater

final class Updater: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    enum Mode { case scan, push, confirm }
    enum Phase: String {
        case bluetooth, scanning, connecting, discovering, readingInfo, starting
        case sending, finishing, rebooting, verifying, confirming
    }

    let mode: Mode
    let image: Data
    let imageInfo: ImageInfo?
    /// Leave the new image on probation instead of confirming it.
    let leaveUnconfirmed: Bool

    var central: CBCentralManager!
    var device: CBPeripheral?
    var ctrl: CBCharacteristic?, data: CBCharacteristic?, info: CBCharacteristic?

    var phase = Phase.bluetooth
    var afterReboot = false
    var sent = 0, acked = 0, window = 0
    var sendStart = Date()
    var lastLoggedDecile = -1
    var watchdog: DispatchWorkItem?

    init(mode: Mode, image: Data = Data(), info: ImageInfo? = nil, leaveUnconfirmed: Bool = false) {
        self.mode = mode; self.image = image; self.imageInfo = info
        self.leaveUnconfirmed = leaveUnconfirmed
        super.init()
        central = CBCentralManager(delegate: self, queue: nil)
        arm(90, "Bluetooth did not become available. If macOS asked whether “SF01 OTA” may use Bluetooth, click Allow and run this again")
        DispatchQueue.main.asyncAfter(deadline: .now() + 3) { [weak self] in
            guard let self, self.phase == .bluetooth else { return }
            if CBCentralManager.authorization == .notDetermined {
                Log.line("waiting for Bluetooth permission — click Allow on the macOS prompt for “SF01 OTA”")
            }
        }
    }

    /// One watchdog at a time: every phase re-arms it with its own deadline.
    func arm(_ seconds: Double, _ message: String) {
        watchdog?.cancel()
        let w = DispatchWorkItem { Log.finish(ok: false, message) }
        watchdog = w
        DispatchQueue.main.asyncAfter(deadline: .now() + seconds, execute: w)
    }

    func enter(_ p: Phase) { phase = p }

    // MARK: central

    func centralManagerDidUpdateState(_ c: CBCentralManager) {
        switch c.state {
        case .poweredOn:
            Log.line("Bluetooth ready")
            startScan(timeout: 25)
        case .unauthorized:
            Log.finish(ok: false, "Bluetooth access is denied for “SF01 OTA”. Enable it in System Settings → Privacy & Security → Bluetooth")
        case .poweredOff:
            Log.finish(ok: false, "Bluetooth is switched off on this Mac")
        case .unsupported:
            Log.finish(ok: false, "this Mac has no usable Bluetooth LE")
        default:
            break
        }
    }

    func startScan(timeout: Double) {
        enter(.scanning)
        Log.line(afterReboot ? "waiting for the speaker to come back…" : "scanning for the speaker…")
        central.scanForPeripherals(withServices: [SF01.service])
        arm(timeout, afterReboot
            ? "the speaker did not come back after the update. If it is still powered it will roll back to the previous firmware by itself"
            : "speaker not found. Check it is powered and in range, and that the SF01Control app on the phone is closed (the speaker accepts one Bluetooth LE connection at a time)")
    }

    func centralManager(_ c: CBCentralManager, didDiscover p: CBPeripheral,
                        advertisementData: [String: Any], rssi: NSNumber) {
        guard phase == .scanning else { return }
        c.stopScan()
        Log.line("found “\(p.name ?? "?")”, RSSI \(rssi) dBm")
        device = p; p.delegate = self
        enter(.connecting)
        arm(15, "could not connect to the speaker")
        c.connect(p)
    }

    func centralManager(_ c: CBCentralManager, didConnect p: CBPeripheral) {
        enter(.discovering)
        arm(15, "service discovery timed out")
        p.discoverServices([SF01.service])
    }

    func centralManager(_ c: CBCentralManager, didFailToConnect p: CBPeripheral, error: Error?) {
        Log.finish(ok: false, "connection failed: \(error?.localizedDescription ?? "unknown")")
    }

    func centralManager(_ c: CBCentralManager, didDisconnectPeripheral p: CBPeripheral, error: Error?) {
        if phase == .rebooting {
            Log.line("speaker disconnected to reboot")
            afterReboot = true
            ctrl = nil; data = nil; info = nil
            DispatchQueue.main.asyncAfter(deadline: .now() + 2) { self.startScan(timeout: 60) }
        } else {
            Log.finish(ok: false, "connection lost while \(phase.rawValue)")
        }
    }

    // MARK: discovery

    func peripheral(_ p: CBPeripheral, didDiscoverServices error: Error?) {
        guard let svc = p.services?.first(where: { $0.uuid == SF01.service }) else {
            Log.finish(ok: false, "the speaker's control service was not found")
        }
        p.discoverCharacteristics(nil, for: svc)
    }

    func peripheral(_ p: CBPeripheral, didDiscoverCharacteristicsFor svc: CBService, error: Error?) {
        for ch in svc.characteristics ?? [] {
            switch ch.uuid {
            case SF01.fwCtrl: ctrl = ch
            case SF01.fwData: data = ch
            case SF01.fwInfo: info = ch
            default: break
            }
        }
        guard let info, ctrl != nil, data != nil else {
            if mode == .scan {
                Log.finish(ok: true, "Bluetooth works and the speaker is reachable, but its firmware has no update service yet — it needs one USB flash first")
            }
            Log.finish(ok: false, "this speaker's firmware has no update service yet. Flash it over USB once; after that updates can go over the air")
        }
        enter(afterReboot ? .verifying : .readingInfo)
        arm(15, "reading the firmware info timed out")
        p.readValue(for: info)
    }

    // MARK: values

    func peripheral(_ p: CBPeripheral, didUpdateValueFor ch: CBCharacteristic, error: Error?) {
        guard let value = ch.value else { return }
        if ch.uuid == SF01.fwInfo { handleInfo(p, value) }
        else if ch.uuid == SF01.fwCtrl { handleEvent(p, [UInt8](value)) }
    }

    func handleInfo(_ p: CBPeripheral, _ value: Data) {
        guard let dev = DeviceInfo(value) else {
            Log.finish(ok: false, "unreadable firmware info (\(value.count) bytes)")
        }
        switch phase {
        case .readingInfo:
            Log.line("speaker is running \(dev.summary)")
            switch mode {
            case .scan:
                Log.finish(ok: true, "speaker reachable and ready for over-the-air updates")
            case .confirm:
                sendConfirm(p)
            case .push:
                guard let ctrl else { return }
                enter(.starting)
                arm(15, "the speaker did not accept the update request")
                p.setNotifyValue(true, for: ctrl)
            }

        case .verifying:
            Log.line("after reboot: \(dev.summary)")
            guard let want = imageInfo?.buildID, dev.buildID == want else {
                Log.finish(ok: false, "the speaker came back running build \(dev.buildID), not the one just sent — it has rolled back to its previous firmware")
            }
            if leaveUnconfirmed {
                Log.finish(ok: true, "speaker is running build \(dev.buildID) on probation; it rolls back unless confirmed (push.sh --confirm)")
            }
            sendConfirm(p)

        case .confirming:
            if dev.pending {
                Log.finish(ok: false, "the speaker still reports the new image as unconfirmed")
            }
            Log.finish(ok: true, "speaker is running build \(dev.buildID) and has confirmed it")

        default:
            break
        }
    }

    func sendConfirm(_ p: CBPeripheral) {
        guard let ctrl else { return }
        enter(.confirming)
        arm(15, "the speaker did not acknowledge the confirmation")
        p.writeValue(Data([SF01.cmdConfirm]), for: ctrl, type: .withResponse)
    }

    func peripheral(_ p: CBPeripheral, didUpdateNotificationStateFor ch: CBCharacteristic, error: Error?) {
        guard ch.uuid == SF01.fwCtrl, phase == .starting, let ctrl else { return }
        if let error { Log.finish(ok: false, "could not subscribe to update events: \(error.localizedDescription)") }
        var begin = Data([SF01.cmdBegin])
        withUnsafeBytes(of: UInt32(image.count).littleEndian) { begin.append(contentsOf: $0) }
        Log.line("requesting update: \(image.count) bytes")
        p.writeValue(begin, for: ctrl, type: .withResponse)
    }

    func peripheral(_ p: CBPeripheral, didWriteValueFor ch: CBCharacteristic, error: Error?) {
        if let error { Log.finish(ok: false, "write failed while \(phase.rawValue): \(error.localizedDescription)") }
        if phase == .confirming, let info {
            p.readValue(for: info)          // prove the flag actually cleared
        }
    }

    func handleEvent(_ p: CBPeripheral, _ b: [UInt8]) {
        guard let type = b.first else { return }
        func u32(_ o: Int) -> Int {
            guard b.count >= o + 4 else { return 0 }
            return Int(b[o]) | Int(b[o+1]) << 8 | Int(b[o+2]) << 16 | Int(b[o+3]) << 24
        }
        switch type {
        case SF01.evtReady:
            window = u32(1)
            let chunk = p.maximumWriteValueLength(for: .withoutResponse)
            Log.line("speaker ready: window \(window) bytes, \(chunk)-byte packets")
            enter(.sending); sendStart = Date()
            arm(20, "the transfer stalled before any data was acknowledged")
            pump(p)

        case SF01.evtAck:
            acked = u32(1)
            let decile = acked * 10 / max(image.count, 1)
            if decile != lastLoggedDecile {
                lastLoggedDecile = decile
                let secs = max(Date().timeIntervalSince(sendStart), 0.001)
                Log.line(String(format: "  %3d%%  %7d / %d bytes  %.1f kB/s",
                                acked * 100 / max(image.count, 1), acked, image.count,
                                Double(acked) / 1024 / secs))
            }
            if acked >= image.count {
                enter(.finishing)
                arm(45, "the speaker did not finish verifying the image")
                Log.line("all data written, speaker is verifying the image…")
            } else {
                arm(20, "the transfer stalled at \(acked) of \(image.count) bytes")
                pump(p)
            }

        case SF01.evtDone:
            Log.line(String(format: "image verified and accepted in %.1f s — speaker is rebooting into it",
                            Date().timeIntervalSince(sendStart)))
            enter(.rebooting)
            arm(30, "the speaker accepted the image but never rebooted")

        case SF01.evtConfirmed:
            break                              // the info read-back is the real check

        case SF01.evtError:
            Log.finish(ok: false, SF01.errorText(b.count > 1 ? b[1] : 0))

        default:
            break
        }
    }

    // MARK: transfer

    /// Send as much as both flow-control layers allow: the speaker's window
    /// (bytes it has not yet written to flash) and CoreBluetooth's own queue.
    func pump(_ p: CBPeripheral) {
        guard phase == .sending, let data else { return }
        let chunk = p.maximumWriteValueLength(for: .withoutResponse)
        while sent < image.count {
            let n = min(chunk, image.count - sent)
            guard (sent - acked) + n <= window, p.canSendWriteWithoutResponse else { return }
            p.writeValue(image.subdata(in: sent..<(sent + n)), for: data, type: .withoutResponse)
            sent += n
        }
    }

    func peripheralIsReady(toSendWriteWithoutResponse p: CBPeripheral) { pump(p) }
}

// MARK: - entry point

func usage() -> Never {
    print("""
    usage: sf01-ota scan                           [--log FILE]
           sf01-ota push FIRMWARE.bin [--no-confirm] [--log FILE]
           sf01-ota confirm                        [--log FILE]
    """)
    exit(2)
}

var args = Array(CommandLine.arguments.dropFirst())
var logPath: String?
if let i = args.firstIndex(of: "--log"), i + 1 < args.count {
    logPath = args[i + 1]
    args.removeSubrange(i...(i + 1))
}
var noConfirm = false
if let i = args.firstIndex(of: "--no-confirm") {
    noConfirm = true
    args.remove(at: i)
}
Log.open(logPath)

let updater: Updater
switch args.first {
case "scan":
    updater = Updater(mode: .scan)
case "confirm":
    updater = Updater(mode: .confirm)
case "push":
    guard args.count >= 2 else { usage() }
    guard let bin = FileManager.default.contents(atPath: args[1]) else {
        Log.finish(ok: false, "cannot read \(args[1])")
    }
    guard let meta = ImageInfo(bin) else {
        Log.finish(ok: false, "\(args[1]) is not an ESP32 application image")
    }
    guard meta.project == SF01.projectName else {
        Log.finish(ok: false, "refusing to send “\(meta.project)”: this tool only pushes \(SF01.projectName) builds")
    }
    Log.line("image: build \(meta.buildID) (\(meta.version), \(meta.date) \(meta.time)), \(bin.count) bytes")
    updater = Updater(mode: .push, image: bin, info: meta, leaveUnconfirmed: noConfirm)
default:
    usage()
}

// Nothing here should ever take this long; never leave a process behind.
DispatchQueue.main.asyncAfter(deadline: .now() + 420) {
    Log.finish(ok: false, "gave up after 7 minutes")
}
RunLoop.main.run()
