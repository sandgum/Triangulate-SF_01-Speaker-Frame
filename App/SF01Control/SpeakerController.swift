//
//  SpeakerController.swift
//  CoreBluetooth client for the SF_01 speaker's GATT control service.
//

import Foundation
import CoreBluetooth

@MainActor
final class SpeakerController: NSObject, ObservableObject {

    enum State: Equatable {
        case bluetoothOff, unauthorized, scanning, connecting, ready
        case disconnected(String)

        var label: String {
            switch self {
            case .bluetoothOff:      return "Bluetooth is off"
            case .unauthorized:      return "Bluetooth permission denied"
            case .scanning:          return "Looking for SF_01…"
            case .connecting:        return "Connecting…"
            case .ready:             return "Connected"
            case .disconnected(let why): return why
            }
        }
        var isReady: Bool { self == .ready }
    }

    @Published private(set) var state: State = .scanning
    @Published private(set) var deviceName: String?
    @Published var config = SF01Config()

    private var central: CBCentralManager!
    private var peripheral: CBPeripheral?
    private var commandChar: CBCharacteristic?

    /// The speaker echoes every change back as a notification. While the user
    /// is dragging a slider those echoes arrive a beat late and would yank the
    /// knob backwards, so remote updates are ignored briefly after a local
    /// write. Anything changed elsewhere (the UART shell, AVRCP volume) still
    /// lands as soon as the user stops touching the control.
    private var lastWrite = Date.distantPast
    private let echoGuard: TimeInterval = 0.5

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: .main)
    }

    /// A slider drag emits values far faster than BLE should carry them, and
    /// the radio is shared with the A2DP audio link. Commands are coalesced by
    /// control and flushed on a 50 ms tick, so a drag costs at most 20 writes
    /// per second and the final value always lands.
    private var pending: [String: SF01Command] = [:]
    private var flushTimer: Timer?

    func send(_ command: SF01Command) {
        pending[command.coalesceKey] = command
        lastWrite = Date()
        guard flushTimer == nil else { return }
        flushTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] t in
            Task { @MainActor in
                guard let self else { t.invalidate(); return }
                if self.pending.isEmpty { t.invalidate(); self.flushTimer = nil; return }
                let batch = self.pending
                self.pending.removeAll()
                for (_, cmd) in batch { self.write(cmd) }
            }
        }
    }

    private func write(_ command: SF01Command) {
        guard let p = peripheral, let c = commandChar else { return }
        let type: CBCharacteristicWriteType =
            c.properties.contains(.writeWithoutResponse) ? .withoutResponse : .withResponse
        p.writeValue(command.data, for: c, type: type)
    }

    /// Re-read after a save/load/defaults round trip, where the echo guard
    /// would otherwise swallow the update we actually want.
    func refreshSoon() {
        lastWrite = .distantPast
        guard let p = peripheral,
              let ch = p.services?.first(where: { $0.uuid == SF01.service })?
                        .characteristics?.first(where: { $0.uuid == SF01.config })
        else { return }
        p.readValue(for: ch)
    }

    private func beginScan() {
        guard central.state == .poweredOn else { return }
        state = .scanning
        central.scanForPeripherals(withServices: [SF01.service])
    }
}

extension SpeakerController: CBCentralManagerDelegate {
    nonisolated func centralManagerDidUpdateState(_ c: CBCentralManager) {
        Task { @MainActor in
            switch c.state {
            case .poweredOn:     beginScan()
            case .unauthorized:  state = .unauthorized
            default:             state = .bluetoothOff
            }
        }
    }

    nonisolated func centralManager(_ c: CBCentralManager, didDiscover p: CBPeripheral,
                                    advertisementData: [String: Any], rssi: NSNumber) {
        Task { @MainActor in
            c.stopScan()
            peripheral = p
            deviceName = p.name
            state = .connecting
            p.delegate = self
            c.connect(p)
        }
    }

    nonisolated func centralManager(_ c: CBCentralManager, didConnect p: CBPeripheral) {
        Task { @MainActor in p.discoverServices([SF01.service]) }
    }

    nonisolated func centralManager(_ c: CBCentralManager, didFailToConnect p: CBPeripheral,
                                    error: Error?) {
        Task { @MainActor in
            state = .disconnected("Could not connect")
            beginScan()
        }
    }

    nonisolated func centralManager(_ c: CBCentralManager, didDisconnectPeripheral p: CBPeripheral,
                                    error: Error?) {
        Task { @MainActor in
            commandChar = nil
            peripheral = nil
            state = .disconnected("Speaker disconnected")
            beginScan()
        }
    }
}

extension SpeakerController: CBPeripheralDelegate {
    nonisolated func peripheral(_ p: CBPeripheral, didDiscoverServices error: Error?) {
        Task { @MainActor in
            guard let svc = p.services?.first(where: { $0.uuid == SF01.service }) else {
                state = .disconnected("Control service not found")
                return
            }
            p.discoverCharacteristics([SF01.config, SF01.command], for: svc)
        }
    }

    nonisolated func peripheral(_ p: CBPeripheral, didDiscoverCharacteristicsFor svc: CBService,
                                error: Error?) {
        Task { @MainActor in
            for ch in svc.characteristics ?? [] {
                switch ch.uuid {
                case SF01.config:
                    p.setNotifyValue(true, for: ch)
                    p.readValue(for: ch)
                case SF01.command:
                    commandChar = ch
                default:
                    break
                }
            }
            if commandChar != nil { state = .ready }
        }
    }

    nonisolated func peripheral(_ p: CBPeripheral, didUpdateValueFor ch: CBCharacteristic,
                                error: Error?) {
        Task { @MainActor in
            guard ch.uuid == SF01.config, let data = ch.value else { return }
            guard let incoming = SF01Config(data) else {
                state = .disconnected("Firmware protocol mismatch")
                return
            }
            guard Date().timeIntervalSince(lastWrite) > echoGuard else { return }
            config = incoming
        }
    }
}
