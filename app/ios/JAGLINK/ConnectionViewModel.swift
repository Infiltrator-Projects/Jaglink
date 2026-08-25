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

private func jaglinkLocalized(_ key: String) -> String {
    let language = UserDefaults.standard.string(forKey: "jaglink.language") ?? "en"
    guard let path = Bundle.main.path(forResource: language, ofType: "lproj"),
          let bundle = Bundle(path: path) else {
        return key
    }
    return bundle.localizedString(forKey: key, value: key, table: nil)
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

    override init() {
        super.init()
        controller.delegate = self
        loadJaguarProfile()
        if let value = jaglink_version() { versionText = String(cString: value) }
        refresh()
    }

    func connect() {
        clearPreparedExport()
        if isActive { return }

        let alert = UIAlertController(
            title: jaglinkLocalized("Connection Test"),
            message: jaglinkLocalized("Real Adapter uses Bluetooth. Simulated ELM327 runs the same diagnostic stack against an in-process ELM327 byte-stream emulator."),
            preferredStyle: .alert
        )
        alert.addAction(UIAlertAction(title: jaglinkLocalized("Real Adapter"), style: .default) { [weak self] _ in
            Task { @MainActor [weak self] in
                guard let self else { return }
                self.isSimulationActive = false
                self.controller.start()
            }
        })
        alert.addAction(UIAlertAction(title: jaglinkLocalized("Simulated ELM327"), style: .default) { [weak self] _ in
            Task { @MainActor [weak self] in
                guard let self else { return }
                self.isSimulationActive = true
                self.controller.startSimulated()
            }
        })
        alert.addAction(UIAlertAction(title: jaglinkLocalized("Cancel"), style: .cancel))

        guard let presenter = presentingViewController() else {
            isSimulationActive = false
            controller.start()
            return
        }
        presenter.present(alert, animated: true)
    }

    func disconnect() {
        controller.disconnect()
        isSimulationActive = false
    }

    func toggleFavourite(stableKey: String) {
        let pid: UInt8? = stableKey.withCString { key in
            guard let definition = jaglink_parameter_obd2_definition_for_stable_key(key) else { return nil }
            return UInt8(exactly: definition.pointee.key.identifier)
        }
        guard let pid else { return }
        controller.setFavourite(!controller.favourite(forPID: pid), forPID: pid)
        refresh()
    }

    func prepareCSVExport() {
        guard let csv = controller.csvSnapshot(), let data = csv.data(using: .utf8) else {
            clearPreparedExport()
            return
        }
        clearPreparedExport()
        let url = FileManager.default.temporaryDirectory
            .appendingPathComponent("JAGLINK-diagnostic-evidence-\(UUID().uuidString).csv")
        do {
            try data.write(to: url, options: .atomic)
            csvExportURL = url
        } catch {
            csvExportURL = nil
        }
    }

    func diagnosticsControllerDidUpdate(_ controller: JagLinkDiagnosticsController) {
        refresh()
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
        if let presented = controller.presentedViewController { return topViewController(presented) }
        if let navigation = controller as? UINavigationController,
           let visible = navigation.visibleViewController { return topViewController(visible) }
        if let tabs = controller as? UITabBarController,
           let selected = tabs.selectedViewController { return topViewController(selected) }
        return controller
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
