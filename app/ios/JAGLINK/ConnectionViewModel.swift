// SPDX-License-Identifier: GPL-3.0-or-later
import Combine
import Foundation
import UIKit

struct DiagnosticParameter: Identifiable {
    let id: String
    let protocolName: String
    let moduleIdentifier: UInt32
    let parameterIdentifier: UInt32
    let shortName: String
    let title: String
    let suffix: String
    let formattedValue: String
    let value: Double?
    let favourite: Bool
    let history: [Double]
    var isAvailable: Bool { value != nil }
}

struct JaguarNetworkInfo: Identifiable {
    let id: String
    let name: String
    let kind: String
    let role: String
    let nominalBaud: UInt32
    let status: String
    let provenance: String
}

@MainActor
final class ConnectionViewModel: NSObject, ObservableObject, @preconcurrency JagLinkDiagnosticsControllerDelegate {
    @Published private(set) var statusText = "Idle"
    @Published private(set) var peripheralName = "No adapter"
    @Published private(set) var adapterIdentifier = "Unknown"
    @Published private(set) var faultScanStatusText = "Not scanned"
    @Published private(set) var storedDTCs = [String]()
    @Published private(set) var pendingDTCs = [String]()
    @Published private(set) var permanentDTCs = [String]()
    @Published private(set) var isActive = false
    @Published private(set) var isReady = false
    @Published private(set) var isSimulationActive = false
    @Published private(set) var diagnosticParameters = [DiagnosticParameter]()
    @Published private(set) var jaguarNetworks = [JaguarNetworkInfo]()
    @Published private(set) var profileDisplayName = "Jaguar X-Type (X400)"
    @Published private(set) var recordedSampleCount = 0
    @Published private(set) var versionText = "0.1.0"
    @Published private(set) var csvExportURL: URL?

    private let controller = JagLinkDiagnosticsController()
    private var simulationTimer: Timer?
    private var simulationStep = 0
    private var simulationHistory = [String: [Double]]()
    private var simulationFavourites: Set<String> = [
        "obd2.engine.rpm",
        "obd2.vehicle.speed",
        "obd2.engine.coolant",
        "obd2.engine.map"
    ]

    override init() {
        super.init()
        controller.delegate = self
        loadJaguarProfile()
        if let value = jaglink_version() { versionText = String(cString: value) }
        refresh()
    }

    func connect() {
        clearPreparedExport()
        if isSimulationActive { return }

        let alert = UIAlertController(
            title: "Connection Test",
            message: "Choose a real Bluetooth adapter or run the built-in simulated ECU.",
            preferredStyle: .alert
        )
        alert.addAction(UIAlertAction(title: "Real Adapter", style: .default) { [weak self] _ in
            Task { @MainActor [weak self] in
                self?.controller.start()
            }
        })
        alert.addAction(UIAlertAction(title: "Simulated ECU", style: .default) { [weak self] _ in
            Task { @MainActor [weak self] in
                self?.startSimulation()
            }
        })
        alert.addAction(UIAlertAction(title: "Cancel", style: .cancel))

        guard let presenter = presentingViewController() else {
            controller.start()
            return
        }
        presenter.present(alert, animated: true)
    }

    func disconnect() {
        if isSimulationActive {
            stopSimulation()
        } else {
            controller.disconnect()
        }
    }

    func toggleFavourite(stableKey: String) {
        if isSimulationActive {
            if simulationFavourites.contains(stableKey) {
                simulationFavourites.remove(stableKey)
            } else {
                simulationFavourites.insert(stableKey)
            }
            rebuildSimulationParameters()
            return
        }

        let pid: UInt8? = stableKey.withCString { key in
            guard let definition = jaglink_parameter_obd2_definition_for_stable_key(key) else { return nil }
            return UInt8(exactly: definition.pointee.key.identifier)
        }
        guard let pid else { return }
        controller.setFavourite(!controller.favourite(forPID: pid), forPID: pid)
        refresh()
    }

    func prepareCSVExport() {
        let csv: String?
        if isSimulationActive {
            csv = simulationCSV()
        } else {
            csv = controller.csvSnapshot()
        }
        guard let csv, let data = csv.data(using: .utf8) else {
            clearPreparedExport()
            return
        }

        clearPreparedExport()
        let url = FileManager.default.temporaryDirectory.appendingPathComponent("JAGLINK-diagnostic-evidence-\(UUID().uuidString).csv")
        do {
            try data.write(to: url, options: .atomic)
            csvExportURL = url
        } catch {
            csvExportURL = nil
        }
    }

    func diagnosticsControllerDidUpdate(_ controller: JagLinkDiagnosticsController) {
        if !isSimulationActive { refresh() }
    }

    private func presentingViewController() -> UIViewController? {
        guard let scene = UIApplication.shared.connectedScenes
            .compactMap({ $0 as? UIWindowScene })
            .first(where: { $0.activationState == .foregroundActive }),
              let root = scene.windows.first(where: \.isKeyWindow)?.rootViewController else {
            return nil
        }
        return topViewController(root)
    }

    private func topViewController(_ controller: UIViewController) -> UIViewController {
        if let presented = controller.presentedViewController {
            return topViewController(presented)
        }
        if let navigation = controller as? UINavigationController,
           let visible = navigation.visibleViewController {
            return topViewController(visible)
        }
        if let tabs = controller as? UITabBarController,
           let selected = tabs.selectedViewController {
            return topViewController(selected)
        }
        return controller
    }

    private func startSimulation() {
        if controller.isActive { controller.disconnect() }
        simulationTimer?.invalidate()
        simulationStep = 0
        simulationHistory.removeAll()
        isSimulationActive = true
        isActive = true
        isReady = true
        statusText = "Simulated ECU"
        peripheralName = "JAGLINK Demo Adapter"
        adapterIdentifier = "ELM327 SIM v1.0"
        faultScanStatusText = "Complete · simulated vehicle response"
        storedDTCs = ["P0171", "P0420"]
        pendingDTCs = ["P0300"]
        permanentDTCs = ["P0133"]
        recordedSampleCount = 0
        rebuildSimulationParameters()

        let timer = Timer(
            timeInterval: 1.0,
            target: self,
            selector: #selector(simulationTimerFired),
            userInfo: nil,
            repeats: true
        )
        simulationTimer = timer
        RunLoop.main.add(timer, forMode: .common)
    }

    private func stopSimulation() {
        simulationTimer?.invalidate()
        simulationTimer = nil
        simulationHistory.removeAll()
        isSimulationActive = false
        refresh()
    }

    @objc private func simulationTimerFired() {
        guard isSimulationActive else { return }
        simulationStep += 1
        rebuildSimulationParameters()
        recordedSampleCount += diagnosticParameters.filter(\.isAvailable).count
    }

    private func simulatedValue(for stableKey: String) -> Double? {
        let wave = sin(Double(simulationStep) * 0.33)
        let slowWave = sin(Double(simulationStep) * 0.11)
        switch stableKey {
        case "obd2.engine.rpm": return 1050.0 + (wave * 320.0)
        case "obd2.vehicle.speed": return 48.0 + (slowWave * 18.0)
        case "obd2.engine.map": return 112.0 + (wave * 18.0)
        case "obd2.engine.throttle": return 24.0 + (wave * 8.0)
        case "obd2.engine.load": return 38.0 + (wave * 12.0)
        case "obd2.engine.maf": return 17.5 + (wave * 4.2)
        case "obd2.engine.coolant": return 89.0 + (slowWave * 1.5)
        case "obd2.engine.intake_air": return 27.0 + (slowWave * 2.0)
        case "obd2.diesel.rail_pressure": return 32000.0 + (wave * 6500.0)
        case "obd2.diesel.egr_command": return 28.0 + (wave * 9.0)
        case "obd2.diesel.egr_error": return wave * 2.5
        case "obd2.engine.barometric_pressure": return 100.0
        case "obd2.aftertreatment.catalyst_temp_b1s1": return 365.0 + (wave * 22.0)
        case "obd2.electrical.control_module_voltage": return 14.18 + (wave * 0.08)
        case "obd2.environment.ambient_air": return 23.0
        case "obd2.engine.oil_temperature": return 94.0 + slowWave
        case "obd2.engine.fuel_rate": return 4.6 + (wave * 0.9)
        case "obd2.aftertreatment.egt_b1s1": return 410.0 + (wave * 35.0)
        case "obd2.dpf.bank1_delta_pressure": return 1.75 + (wave * 0.35)
        case "obd2.dpf.bank1_inlet_temperature": return 348.0 + (wave * 28.0)
        default: return nil
        }
    }

    private func rebuildSimulationParameters() {
        let count = jaglink_parameter_obd2_definition_count()
        var result = [DiagnosticParameter]()
        result.reserveCapacity(count)

        for index in 0..<count {
            guard let definition = jaglink_parameter_obd2_definition_at(index) else { continue }
            let metadata = definition.pointee
            let stableKey = string(from: metadata.stable_key)
            guard !stableKey.isEmpty else { continue }

            let value = simulatedValue(for: stableKey)
            if let value {
                var history = simulationHistory[stableKey, default: []]
                if history.last != value {
                    history.append(value)
                    if history.count > 60 { history.removeFirst(history.count - 60) }
                    simulationHistory[stableKey] = history
                }
            }

            result.append(DiagnosticParameter(
                id: stableKey,
                protocolName: string(from: jaglink_parameter_protocol_name(metadata.key.protocol)),
                moduleIdentifier: metadata.key.module,
                parameterIdentifier: metadata.key.identifier,
                shortName: string(from: metadata.short_name),
                title: string(from: metadata.name),
                suffix: string(from: metadata.suffix),
                formattedValue: formattedValue(definition: definition, value: value),
                value: value,
                favourite: simulationFavourites.contains(stableKey),
                history: simulationHistory[stableKey] ?? []
            ))
        }
        diagnosticParameters = result
    }

    private func simulationCSV() -> String {
        var rows = ["timestamp_ms,source,type,key,value"]
        let timestamp = Int(Date().timeIntervalSince1970 * 1000.0)
        rows.append("\(timestamp),simulated,adapter,identity,ELM327 SIM v1.0")
        rows.append("\(timestamp),simulated,dtc,stored,P0171")
        rows.append("\(timestamp),simulated,dtc,stored,P0420")
        rows.append("\(timestamp),simulated,dtc,pending,P0300")
        for parameter in diagnosticParameters where parameter.value != nil {
            rows.append("\(timestamp),simulated,parameter,\(parameter.id),\(parameter.formattedValue)")
        }
        return rows.joined(separator: "\n") + "\n"
    }

    private func clearPreparedExport() {
        if let url = csvExportURL { try? FileManager.default.removeItem(at: url) }
        csvExportURL = nil
    }

    private func string(from cString: UnsafePointer<CChar>?) -> String {
        guard let cString else { return "" }
        return String(cString: cString)
    }

    private func loadJaguarProfile() {
        guard let profile = jaglink_jaguar_x400_profile() else { return }
        profileDisplayName = string(from: profile.pointee.display_name)
        guard let networks = profile.pointee.networks else { return }
        var result = [JaguarNetworkInfo]()
        for index in 0..<Int(profile.pointee.network_count) {
            let network = networks[index]
            let key = string(from: network.key)
            guard !key.isEmpty else { continue }
            result.append(JaguarNetworkInfo(
                id: key,
                name: string(from: network.name),
                kind: string(from: jaglink_jaguar_network_kind_name(network.kind)),
                role: string(from: jaglink_jaguar_network_role_name(network.role)),
                nominalBaud: network.nominal_baud,
                status: string(from: jaglink_jaguar_definition_status_name(network.status)),
                provenance: string(from: network.provenance)))
        }
        jaguarNetworks = result
    }

    private func formattedValue(definition: UnsafePointer<JaglinkParameterDefinition>, value: Double?) -> String {
        var buffer = [CChar](repeating: 0, count: 96)
        let success = buffer.withUnsafeMutableBufferPointer { storage in
            jaglink_parameter_format_value(definition, value != nil, value ?? 0.0, storage.baseAddress, storage.count)
        }
        guard success else { return "N/A" }
        return buffer.withUnsafeBufferPointer { storage in
            guard let base = storage.baseAddress else { return "N/A" }
            return String(cString: base)
        }
    }

    private func loadDiagnosticParameters() -> [DiagnosticParameter] {
        let count = jaglink_parameter_obd2_definition_count()
        guard count > 0 else { return [] }
        var result = [DiagnosticParameter]()
        result.reserveCapacity(count)
        for index in 0..<count {
            guard let definition = jaglink_parameter_obd2_definition_at(index) else { continue }
            let metadata = definition.pointee
            guard let pid = UInt8(exactly: metadata.key.identifier) else { continue }
            let history = controller.recentValues(forPID: pid, limit: 60).map(\.doubleValue)
            let value = history.last
            let stableKey = string(from: metadata.stable_key)
            guard !stableKey.isEmpty else { continue }
            result.append(DiagnosticParameter(
                id: stableKey,
                protocolName: string(from: jaglink_parameter_protocol_name(metadata.key.protocol)),
                moduleIdentifier: metadata.key.module,
                parameterIdentifier: metadata.key.identifier,
                shortName: string(from: metadata.short_name),
                title: string(from: metadata.name),
                suffix: string(from: metadata.suffix),
                formattedValue: formattedValue(definition: definition, value: value),
                value: value,
                favourite: controller.favourite(forPID: pid),
                history: history))
        }
        return result
    }

    private func refresh() {
        guard !isSimulationActive else { return }
        statusText = controller.statusText
        peripheralName = controller.peripheralName ?? "No adapter"
        adapterIdentifier = controller.adapterIdentifier ?? "Unknown"
        faultScanStatusText = controller.faultScanStatusText
        storedDTCs = controller.storedDTCs
        pendingDTCs = controller.pendingDTCs
        permanentDTCs = controller.permanentDTCs
        isActive = controller.isActive
        isReady = controller.isReady
        diagnosticParameters = loadDiagnosticParameters()
        recordedSampleCount = Int(clamping: controller.recordedSampleCount)
    }
}
