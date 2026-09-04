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

struct JagSavedVehicleProfileSummary: Identifiable {
    let id: String
    let vin: String
    let displayName: String
    let updatedAt: Date?
    let adapterIdentifier: String?
}

private func jaglinkLocalized(_ key: String) -> String {
    let selected = UserDefaults.standard.string(
        forKey: "link.displayLanguage") ?? "en-AU"
    let language = JagInterfaceLanguage.canonical(selected)
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
    @Published private(set) var vehicleVINText = "Waiting for VIN"
    @Published private(set) var vehiclePlatformText = "Jaguar vehicle identity pending"
    @Published private(set) var vehicleConfigurationText = "Waiting for standard VIN"
    @Published private(set) var vehiclePowertrainText = "Waiting for standard VIN"
    @Published private(set) var vehicleBuildText = "Waiting for standard VIN"
    @Published private(set) var faultScanStatusText = "Not scanned"
    @Published private(set) var storedDTCs = [String]()
    @Published private(set) var pendingDTCs = [String]()
    @Published private(set) var permanentDTCs = [String]()
    @Published private(set) var readinessStatusText = "Not collected"
    @Published private(set) var readinessMonitorStatus = [String]()
    @Published private(set) var freezeFrameContext = [String]()
    @Published private(set) var diagnosticCapabilityText = "Unknown / probing"
    @Published private(set) var diagnosticCapabilityDetailText = ""
    @Published private(set) var standardResponderSummary = "0 physical responders"
    @Published private(set) var supportedPIDSummary = "0 advertised PIDs"
    @Published private(set) var standardVINText = "Unavailable / not yet read"
    @Published private(set) var standardLiveValueRows = [String]()
    @Published private(set) var isActive = false
    @Published private(set) var isReady = false
    @Published private(set) var isSimulationActive = false
    @Published private(set) var diagnosticParameters = [DiagnosticParameter]()
    @Published private(set) var jaguarNetworks = [JaguarNetworkInfo]()
    @Published private(set) var profileDisplayName = "Jaguar vehicle"
    @Published private(set) var savedVehicleProfiles = [JagSavedVehicleProfileSummary]()
    @Published private(set) var selectedVehicleVIN: String?
    @Published private(set) var recordedSampleCount = 0
    @Published private(set) var versionText = "Unknown"
    @Published private(set) var csvExportURL: URL?
    @Published private(set) var isPreparingCSV = false
    @Published private(set) var languageTags = [String]()
    @Published private(set) var languageNames = [String]()
    @Published private(set) var selectedLanguageID = "en-AU"
    @Published private(set) var measurementKeys = [String]()
    @Published private(set) var measurementNames = [String]()
    @Published private(set) var selectedMeasurementID = "metric"

    private let controller = JagLinkDiagnosticsController()
    private let vehicleProfileStore = LinkVehicleProfileStore(
        productNamespace: "jaglink",
        legacyProfileKey: nil,
        legacySelectedVINKey: nil,
        legacyAdapterMappingKey: nil)
    private var lastPersistedLiveVIN: String?

    var selectedVehicleDisplayName: String {
        guard let selectedVehicleVIN else { return "No vehicle loaded" }
        return savedVehicleProfiles.first(where: { $0.vin == selectedVehicleVIN })?.displayName
            ?? "Jaguar vehicle"
    }

    override init() {
        super.init()
        migrateLegacySharedSettings()
        controller.delegate = self
        loadJaguarProfile()
        selectedVehicleVIN = vehicleProfileStore.selectedVehicleVIN
        if let value = jaglink_version() { versionText = String(cString: value) }
        refresh()
    }

    func connect() {
        clearPreparedExport()
        if isActive { return }

        guard let presenter = presentingViewController() else {
            beginConnection(.automatic)
            return
        }

        let currentVehicleText: String
        if let selectedVehicleVIN {
            currentVehicleText = "\(selectedVehicleDisplayName) · \(selectedVehicleVIN)"
        } else {
            currentVehicleText = "No saved vehicle loaded"
        }

        let picker = LinkConnectionPickerViewController(
            vehicleText: currentVehicleText,
            knownAdapterIdentifier: associatedAdapterIdentifier(
                for: selectedVehicleVIN)
        ) { [weak self] source in
            Task { @MainActor [weak self] in
                self?.beginConnection(source)
            }
        }
        let navigation = UINavigationController(rootViewController: picker)
        navigation.modalPresentationStyle = .pageSheet
        presenter.present(navigation, animated: true)
    }

    private func beginConnection(_ source: LinkConnectionSource) {
        guard !isActive else { return }
        lastPersistedLiveVIN = nil
        switch source {
        case .automatic:
            isSimulationActive = false
            controller.start()
        case .simulated:
            isSimulationActive = true
            controller.startSimulated()
        case .peripheral(let identifier):
            isSimulationActive = false
            controller.start(withPeripheralIdentifier: identifier)
        }
    }

    func disconnect() {
        controller.disconnect()
        isSimulationActive = false
    }

    func selectSavedVehicle(vin: String) {
        guard !isActive else { return }
        guard vehicleProfileStore.selectOfflineVehicle(withVIN: vin) else { return }
        selectedVehicleVIN = vin
        refresh()
    }

    var interfaceLocaleIdentifier: String {
        JagInterfaceLanguage.canonical(selectedLanguageID)
    }

    func localizedText(_ key: String) -> String { controller.localizedText(forKey: key) }
    func selectLanguage(_ id: String) { controller.setSelectedLanguageTag(id); refresh() }
    func selectMeasurementSystem(_ id: String) { controller.setSelectedMeasurementSystemKey(id); refresh() }

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
        guard !isPreparingCSV else { return }
        /*
         * Copy the recorder bytes while on the main actor, then perform the
         * filesystem write away from CoreBluetooth/session scheduling. Evidence
         * preparation must not pause or disconnect a live diagnostic session.
         */
        guard let data = controller.csvDataSnapshot() else {
            clearPreparedExport()
            return
        }

        let url = FileManager.default.temporaryDirectory
            .appendingPathComponent("JAGLINK-diagnostic-evidence-\(UUID().uuidString).csv")
        isPreparingCSV = true

        Task { [weak self] in
            do {
                try await Task.detached(priority: .utility) {
                    try data.write(to: url, options: .atomic)
                }.value
                guard let self else {
                    try? FileManager.default.removeItem(at: url)
                    return
                }
                self.clearPreparedExport()
                self.csvExportURL = url
            } catch {
                try? FileManager.default.removeItem(at: url)
            }
            self?.isPreparingCSV = false
        }
    }

    func diagnosticsControllerDidUpdate(_ controller: JagLinkDiagnosticsController) {
        refresh()
    }

    private func associatedAdapterIdentifier(for vin: String?) -> String? {
        guard let vin else { return nil }
        return vehicleProfileStore.associatedAdapterIdentifier(forVIN: vin)
    }

    private func refreshSavedVehicleProfiles() {
        savedVehicleProfiles = vehicleProfileStore.savedProfiles.compactMap { profile in
            guard let vin = profile["vin"] as? String, vin.count == 17 else { return nil }
            let displayName = (profile["displayName"] as? String) ?? "Jaguar vehicle"
            let timestamp = (profile["updatedAt"] as? NSNumber)?.doubleValue
            let adapter = vehicleProfileStore.associatedAdapterIdentifier(forVIN: vin)
            return JagSavedVehicleProfileSummary(
                id: vin,
                vin: vin,
                displayName: displayName,
                updatedAt: timestamp.map { Date(timeIntervalSince1970: $0) },
                adapterIdentifier: adapter)
        }
        selectedVehicleVIN = vehicleProfileStore.selectedVehicleVIN
        if let selectedVehicleVIN,
           let selected = savedVehicleProfiles.first(where: { $0.vin == selectedVehicleVIN }) {
            profileDisplayName = selected.displayName
        }
    }

    private func saveVehicleProfile(vin: String, displayName: String) {
        var profile = vehicleProfileStore.profile(forVIN: vin) ?? [:]
        profile["displayName"] = displayName
        profile["manufacturer"] = "Jaguar"
        profile["platform"] = vehiclePlatformText
        profile["configuration"] = vehicleConfigurationText
        profile["powertrain"] = vehiclePowertrainText
        profile["build"] = vehicleBuildText
        vehicleProfileStore.saveProfile(profile, forVIN: vin)
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

    private func migrateLegacySharedSettings() {
        let defaults = UserDefaults.standard
        if defaults.object(forKey: "link.displayLanguage") == nil,
           let legacy = defaults.string(forKey: "jaglink.language") {
            controller.setSelectedLanguageTag(JagInterfaceLanguage.canonical(legacy))
        }
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
        refreshSavedVehicleProfiles()
        statusText = controller.statusText
        peripheralName = controller.peripheralName ?? "No adapter"
        adapterIdentifier = controller.adapterIdentifier ?? "Unknown"
        vehicleVINText = controller.isActive
            ? (controller.vehicleVINText ?? "Not returned by SAE Mode 09")
            : (selectedVehicleVIN ?? "No vehicle loaded")
        if controller.isActive {
            vehiclePlatformText = controller.vehiclePlatformText
            vehicleConfigurationText = controller.vehicleConfigurationText
            vehiclePowertrainText = controller.vehiclePowertrainText
            vehicleBuildText = controller.vehicleBuildText
        } else if let profile = vehicleProfileStore.profile(forVIN: selectedVehicleVIN ?? "") {
            vehiclePlatformText = (profile["platform"] as? String) ?? "Saved Jaguar profile"
            vehicleConfigurationText = (profile["configuration"] as? String) ?? "Saved vehicle configuration"
            vehiclePowertrainText = (profile["powertrain"] as? String) ?? "Saved powertrain information"
            vehicleBuildText = (profile["build"] as? String) ?? "Saved build information"
        }
        if controller.isActive,
           let liveVIN = controller.vehicleVINText,
           liveVIN.count == 17,
           lastPersistedLiveVIN != liveVIN {
            vehicleProfileStore.recordLiveVIN(liveVIN)
            selectedVehicleVIN = liveVIN
            let existingName =
                (vehicleProfileStore.profile(forVIN: liveVIN)?["displayName"] as? String)
            let name = existingName ??
                (liveVIN == selectedVehicleVIN
                    ? profileDisplayName
                    : "Jaguar vehicle · \(liveVIN)")
            profileDisplayName = name
            saveVehicleProfile(vin: liveVIN, displayName: name)
            lastPersistedLiveVIN = liveVIN
            refreshSavedVehicleProfiles()
        }
        faultScanStatusText = controller.faultScanStatusText
        storedDTCs = controller.storedDTCs
        pendingDTCs = controller.pendingDTCs
        permanentDTCs = controller.permanentDTCs
        readinessStatusText = controller.readinessStatusText
        readinessMonitorStatus = controller.readinessMonitorStatus
        freezeFrameContext = controller.freezeFrameContext
        diagnosticCapabilityText = controller.diagnosticCapabilityText
        diagnosticCapabilityDetailText = controller.diagnosticCapabilityDetailText
        standardResponderSummary = controller.standardResponderSummary
        supportedPIDSummary = controller.supportedPIDSummary
        standardVINText = controller.standardVINText
        standardLiveValueRows = controller.standardLiveValueRows
        languageTags = controller.availableLanguageTags
        languageNames = controller.availableLanguageNames
        selectedLanguageID = controller.selectedLanguageTag
        measurementKeys = controller.availableMeasurementSystemKeys
        measurementNames = controller.availableMeasurementSystemNames
        selectedMeasurementID = controller.selectedMeasurementSystemKey
        isActive = controller.isActive
        isReady = controller.isReady
        diagnosticParameters = loadDiagnosticParameters()
        recordedSampleCount = Int(clamping: controller.recordedSampleCount)
    }
}
