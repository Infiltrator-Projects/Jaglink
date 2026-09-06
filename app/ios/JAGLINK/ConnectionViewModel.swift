// SPDX-License-Identifier: GPL-3.0-or-later
import Combine
import Foundation
import UIKit

typealias DiagnosticParameter = LinkDiagnosticParameter

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

@MainActor
final class ConnectionViewModel: NSObject, ObservableObject, @preconcurrency JagLinkDiagnosticsControllerDelegate {
    @Published private(set) var statusText = "Idle"
    @Published private(set) var peripheralName = "No adapter"
    @Published private(set) var adapterIdentifier = "Unknown"
    @Published private(set) var obdProtocolText = "OBD-II protocol not identified"
    @Published private(set) var vehicleVINText = "Waiting for VIN"
    @Published private(set) var vehiclePlatformText = "Jaguar vehicle identity pending"
    @Published private(set) var vehicleConfigurationText = "Waiting for standard VIN"
    @Published private(set) var vehiclePowertrainText = "Waiting for standard VIN"
    @Published private(set) var vehicleBuildText = "Waiting for standard VIN"
    @Published private(set) var faultScanStatusText = "Not scanned"
    @Published private(set) var storedDTCs = [String]()
    @Published private(set) var pendingDTCs = [String]()
    @Published private(set) var permanentDTCs = [String]()
    @Published private(set) var storedDTCDisplayRows = [String]()
    @Published private(set) var pendingDTCDisplayRows = [String]()
    @Published private(set) var permanentDTCDisplayRows = [String]()
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
    @Published private(set) var diagnosticParameters = [LinkDiagnosticParameter]()
    @Published private(set) var dashboardParameters = [LinkDiagnosticParameter]()
    @Published private(set) var jaguarNetworks = [JaguarNetworkInfo]()
    @Published private(set) var profileDisplayName = "Jaguar vehicle"
    @Published private(set) var savedVehicleProfiles = [JagSavedVehicleProfileSummary]()
    @Published private(set) var selectedVehicleVIN: String?
    @Published private(set) var recordedSampleCount = 0
    @Published private(set) var versionText = "Unknown"
    @Published private(set) var linkVersionText = "Unknown"
    @Published private(set) var csvExportURL: URL?
    @Published private(set) var isPreparingCSV = false
    @Published private(set) var languageTags = [String]()
    @Published private(set) var languageNames = [String]()
    @Published private(set) var selectedLanguageID = "en-AU"
    @Published private(set) var measurementKeys = [String]()
    @Published private(set) var measurementNames = [String]()
    @Published private(set) var selectedMeasurementID = "metric"
    @Published private(set) var instantaneousFuelEconomyText = "Unavailable"
    @Published private(set) var averageFuelEconomyText = "Unavailable"
    @Published private(set) var fuelRateText = "Unavailable"
    @Published private(set) var fuelTripText = "Unavailable"
    @Published private(set) var fuelEconomySourceText = "Unavailable"
    @Published private(set) var factoryFuelSignalStatusText = "Jaguar factory fuel signal not yet enabled"

    private let controller = JagLinkDiagnosticsController()
    private let vehicleProfileStore = LinkVehicleProfileStore(
        productNamespace: "jaglink",
        legacyProfileKey: nil,
        legacySelectedVINKey: nil,
        legacyAdapterMappingKey: nil)
    private let dashboardSelectionStore = LinkPIDSelectionStore(
        productNamespace: "jaglink",
        legacyGlobalKey: nil,
        legacyVehicleKey: nil)
    private let pollingSelectionStore = LinkPIDSelectionStore(
        productNamespace: "jaglink-polling",
        legacyGlobalKey: nil,
        legacyVehicleKey: nil)
    private let standardControllerIdentifier = "standard-obd2"
    private var lastPersistedLiveVIN: String?
    private var lastPersistedReadyVIN: String?
    private var lastCapabilityMergeVIN: String?

    private let legacyLanguageAliases = [
        "en": "en-AU",
        "de": "de-DE",
        "pl": "pl-PL"
    ]

    var selectedVehicleDisplayName: String {
        guard let selectedVehicleVIN else { return "No vehicle loaded" }
        return savedVehicleProfiles.first(where: { $0.vin == selectedVehicleVIN })?.displayName
            ?? "Jaguar vehicle"
    }

    override init() {
        super.init()
        migrateLegacySharedSettings()
        loadJaguarProfile()
        selectedVehicleVIN = vehicleProfileStore.selectedVehicleVIN
        seedDefaultPollingSelection()
        applyStoredPollingPolicy()
        controller.delegate = self
        if let value = jaglink_version() { versionText = String(cString: value) }
        refresh()
    }

    func connect() {
        clearPreparedExport()
        guard !isActive else { return }

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
            knownAdapterIdentifier: associatedAdapterIdentifier(for: selectedVehicleVIN)
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
        lastPersistedReadyVIN = nil
        lastCapabilityMergeVIN = nil
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
        LinkInterfaceLanguage.canonical(
            selectedLanguageID,
            aliases: legacyLanguageAliases,
            fallback: "en-AU")
    }

    func localizedText(_ key: String) -> String { controller.localizedText(forKey: key) }
    func selectLanguage(_ id: String) { controller.setSelectedLanguageTag(id); refresh() }
    func selectMeasurementSystem(_ id: String) { controller.setSelectedMeasurementSystemKey(id); refresh() }

    func toggleFavourite(stableKey: String) {
        guard let parameter = diagnosticParameters.first(where: { $0.id == stableKey }),
              let pid = UInt8(exactly: parameter.parameterIdentifier) else { return }
        controller.setFavourite(!controller.favourite(forPID: pid), forPID: pid)
        refresh()
    }

    func togglePolling(_ parameter: LinkDiagnosticParameter) {
        guard let pid = UInt8(exactly: parameter.parameterIdentifier) else { return }
        let enabled = !controller.pollingEnabled(forPID: pid)
        var enabledKeys = Set(pollingSelectionStore.globalStableKeys)
        if enabled {
            enabledKeys.insert(parameter.id)
        } else {
            enabledKeys.remove(parameter.id)
        }
        pollingSelectionStore.setGlobalStableKeys(Array(enabledKeys).sorted())
        controller.setPollingEnabled(enabled, forPID: pid)
        refresh()
    }

    func prepareCSVExport() {
        guard !isPreparingCSV, let snapshot = controller.csvDataSnapshot() else { return }
        clearPreparedExport()
        isPreparingCSV = true
        let data = snapshot as Data
        Task { [weak self] in
            do {
                let url = try await LinkEvidenceExport.prepareTemporaryCSV(
                    data, productName: "JAGLINK")
                guard let self else {
                    LinkEvidenceExport.removeTemporaryFile(url)
                    return
                }
                self.csvExportURL = url
            } catch {
                self?.csvExportURL = nil
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

    private func saveVehicleProfile(
        vin: String,
        displayName: String,
        includeDiagnosticSnapshot: Bool = false
    ) {
        var profile = vehicleProfileStore.profile(forVIN: vin) ?? [:]
        profile["displayName"] = displayName
        profile["manufacturer"] = "Jaguar"
        profile["platform"] = vehiclePlatformText
        profile["configuration"] = vehicleConfigurationText
        profile["powertrain"] = vehiclePowertrainText
        profile["build"] = vehicleBuildText

        if includeDiagnosticSnapshot {
            profile["obdProtocolText"] = controller.obdProtocolText
            profile["standardResponderSummary"] = controller.standardResponderSummary
            profile["supportedPIDSummary"] = controller.supportedPIDSummary
            profile["standardVINText"] = controller.standardVINText
            profile["standardLiveValueRows"] = controller.standardLiveValueRows
            profile["diagnosticCapabilityText"] = controller.diagnosticCapabilityText
            profile["diagnosticCapabilityDetailText"] = controller.diagnosticCapabilityDetailText
            profile["faultScanStatusText"] = controller.faultScanStatusText
            profile["storedDTCs"] = controller.storedDTCs
            profile["pendingDTCs"] = controller.pendingDTCs
            profile["permanentDTCs"] = controller.permanentDTCs
            profile["storedDTCDisplayRows"] = controller.storedDTCDisplayRows
            profile["pendingDTCDisplayRows"] = controller.pendingDTCDisplayRows
            profile["permanentDTCDisplayRows"] = controller.permanentDTCDisplayRows
            profile["readinessStatusText"] = controller.readinessStatusText
            profile["readinessMonitorStatus"] = controller.readinessMonitorStatus
            profile["freezeFrameContext"] = controller.freezeFrameContext
        }

        vehicleProfileStore.saveProfile(profile, forVIN: vin)
    }

    private func mergeStandardCapabilitiesIfReady(vin: String) {
        guard controller.isReady, lastCapabilityMergeVIN != vin else { return }
        if let flow = controller.diagnosticFlow() {
            _ = vehicleProfileStore.mergeStandardCapabilities(
                fromDiagnosticFlow: flow,
                forVIN: vin)
            lastCapabilityMergeVIN = vin
        }
    }

    private func restoreSavedDiagnosticSnapshot(_ profile: [AnyHashable: Any]) {
        obdProtocolText =
            (profile["obdProtocolText"] as? String)
            ?? "Saved OBD-II protocol unavailable"
        standardResponderSummary =
            (profile["standardResponderSummary"] as? String)
            ?? "Saved standard responder information"
        supportedPIDSummary =
            (profile["supportedPIDSummary"] as? String)
            ?? "Saved standard PID information"
        standardVINText =
            (profile["standardVINText"] as? String)
            ?? "Saved standard VIN information"
        standardLiveValueRows =
            (profile["standardLiveValueRows"] as? [String]) ?? []
        diagnosticCapabilityText =
            (profile["diagnosticCapabilityText"] as? String)
            ?? "Saved diagnostic capability"
        diagnosticCapabilityDetailText =
            (profile["diagnosticCapabilityDetailText"] as? String) ?? ""
        faultScanStatusText =
            (profile["faultScanStatusText"] as? String) ?? "Saved diagnostic state"
        storedDTCs = (profile["storedDTCs"] as? [String]) ?? []
        pendingDTCs = (profile["pendingDTCs"] as? [String]) ?? []
        permanentDTCs = (profile["permanentDTCs"] as? [String]) ?? []
        storedDTCDisplayRows =
            (profile["storedDTCDisplayRows"] as? [String]) ?? storedDTCs
        pendingDTCDisplayRows =
            (profile["pendingDTCDisplayRows"] as? [String]) ?? pendingDTCs
        permanentDTCDisplayRows =
            (profile["permanentDTCDisplayRows"] as? [String]) ?? permanentDTCs
        readinessStatusText =
            (profile["readinessStatusText"] as? String) ?? "Saved readiness state"
        readinessMonitorStatus =
            (profile["readinessMonitorStatus"] as? [String]) ?? []
        freezeFrameContext = (profile["freezeFrameContext"] as? [String]) ?? []
    }

    private func resetOfflineDiagnosticSnapshot() {
        obdProtocolText = "OBD-II protocol not identified"
        faultScanStatusText = "Not scanned"
        storedDTCs = []
        pendingDTCs = []
        permanentDTCs = []
        storedDTCDisplayRows = []
        pendingDTCDisplayRows = []
        permanentDTCDisplayRows = []
        readinessStatusText = "Not collected"
        readinessMonitorStatus = []
        freezeFrameContext = []
        diagnosticCapabilityText = "Unknown / probing"
        diagnosticCapabilityDetailText = ""
        standardResponderSummary = "0 physical responders"
        supportedPIDSummary = "0 advertised PIDs"
        standardVINText = "Unavailable / not yet read"
        standardLiveValueRows = []
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
        LinkEvidenceExport.removeTemporaryFile(csvExportURL)
        csvExportURL = nil
    }

    private func migrateLegacySharedSettings() {
        let defaults = UserDefaults.standard
        if defaults.object(forKey: "link.displayLanguage") == nil,
           let legacy = defaults.string(forKey: "jaglink.language") {
            controller.setSelectedLanguageTag(
                LinkInterfaceLanguage.canonical(
                    legacy,
                    aliases: legacyLanguageAliases,
                    fallback: "en-AU"))
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

    private func formattedDisplayValue(
        definition: UnsafePointer<JaglinkParameterDefinition>,
        value: Double?,
        displayUnit: String
    ) -> String {
        guard let value else { return "N/A" }
        let decimalPlaces = Int(definition.pointee.decimal_places)
        let suffix = displayUnit.isEmpty ? "" : " \(displayUnit)"
        return String(format: "%.*f%@", decimalPlaces, value, suffix)
    }

    private func loadDiagnosticParameters() -> [LinkDiagnosticParameter] {
        let count = jaglink_parameter_obd2_definition_count()
        guard count > 0 else { return [] }
        var result = [LinkDiagnosticParameter]()
        result.reserveCapacity(count)
        for index in 0..<count {
            guard let definition = jaglink_parameter_obd2_definition_at(index) else { continue }
            let metadata = definition.pointee
            guard let pid = UInt8(exactly: metadata.key.identifier) else { continue }
            let history = controller.displayRecentValues(forPID: pid, limit: 60).map(\.doubleValue)
            let value = history.last
            let displayUnit = controller.displayUnit(forPID: pid)
            let range = controller.displayRange(forPID: pid)
            let minimum = range.count >= 2 ? range[0].doubleValue : nil
            let maximum = range.count >= 2 ? range[1].doubleValue : nil
            let stableKey = string(from: metadata.stable_key)
            guard !stableKey.isEmpty else { continue }
            let supported = controller.supportsPID(pid)
            let pollingEnabled = controller.pollingEnabled(forPID: pid)
            result.append(LinkDiagnosticParameter(
                id: stableKey,
                protocolName: string(from: jaglink_parameter_protocol_name(metadata.key.protocol)),
                moduleIdentifier: metadata.key.module,
                parameterIdentifier: metadata.key.identifier,
                shortName: string(from: metadata.short_name),
                title: string(from: metadata.name),
                suffix: displayUnit,
                formattedValue: formattedDisplayValue(
                    definition: definition,
                    value: value,
                    displayUnit: displayUnit),
                value: value,
                structuredValue: nil,
                rawHex: nil,
                vehicleSupported: supported,
                favourite: controller.favourite(forPID: pid),
                pollingEnabled: pollingEnabled,
                history: history,
                sourceLabel: "SAE OBD-II",
                qualityNote: supported && !pollingEnabled ? "Polling disabled" : nil,
                dashboardMinimum: minimum,
                dashboardMaximum: maximum))
        }
        return result
    }

    private func preferredDashboardKeys(from supported: [LinkDiagnosticParameter]) -> [String] {
        let preferredPIDs: [UInt32] = [0x0C, 0x0D, 0x05, 0x11, 0x04, 0x0F]
        let preferred = preferredPIDs.compactMap { pid in
            supported.first(where: { $0.parameterIdentifier == pid })?.id
        }
        return preferred.isEmpty ? Array(supported.prefix(6).map(\.id)) : preferred
    }

    private func refreshDashboardSelection() {
        let supported = diagnosticParameters.filter(\.vehicleSupported)
        let defaults = preferredDashboardKeys(from: supported)

        if !dashboardSelectionStore.hasGlobalSelection, !defaults.isEmpty {
            dashboardSelectionStore.setGlobalStableKeys(defaults)
        }

        let selectedKeys: [String]
        if let vin = selectedVehicleVIN, vin.count == 17 {
            if !dashboardSelectionStore.hasSelection(
                forVIN: vin,
                controllerIdentifier: standardControllerIdentifier),
               !defaults.isEmpty {
                let seed = dashboardSelectionStore.hasGlobalSelection
                    ? dashboardSelectionStore.globalStableKeys
                    : defaults
                dashboardSelectionStore.setStableKeys(
                    seed,
                    forVIN: vin,
                    controllerIdentifier: standardControllerIdentifier)
            }
            selectedKeys = dashboardSelectionStore.hasSelection(
                forVIN: vin,
                controllerIdentifier: standardControllerIdentifier)
                ? dashboardSelectionStore.stableKeys(
                    forVIN: vin,
                    controllerIdentifier: standardControllerIdentifier)
                : dashboardSelectionStore.globalStableKeys
        } else {
            selectedKeys = dashboardSelectionStore.globalStableKeys
        }

        let selected = Set(selectedKeys)
        let chosen = diagnosticParameters.filter {
            selected.contains($0.id) && $0.vehicleSupported
        }
        dashboardParameters = chosen.isEmpty ? Array(supported.prefix(6)) : chosen
    }

    private func allStandardPollingKeys() -> [String] {
        let count = jaglink_parameter_obd2_definition_count()
        guard count > 0 else { return [] }
        return (0..<count).compactMap { index in
            guard let definition = jaglink_parameter_obd2_definition_at(index) else {
                return nil
            }
            let metadata = definition.pointee
            guard let pid = UInt8(exactly: metadata.key.identifier),
                  (pid & 0x1F) != 0 else {
                return nil
            }
            let stableKey = string(from: metadata.stable_key)
            return stableKey.isEmpty ? nil : stableKey
        }
    }

    private func seedDefaultPollingSelection() {
        guard !pollingSelectionStore.hasGlobalSelection else { return }
        pollingSelectionStore.setGlobalStableKeys(allStandardPollingKeys())
    }

    private func applyStoredPollingPolicy() {
        let enabledKeys = Set(pollingSelectionStore.globalStableKeys)
        let count = jaglink_parameter_obd2_definition_count()
        guard count > 0 else { return }
        for index in 0..<count {
            guard let definition = jaglink_parameter_obd2_definition_at(index) else {
                continue
            }
            let metadata = definition.pointee
            guard let pid = UInt8(exactly: metadata.key.identifier),
                  (pid & 0x1F) != 0 else {
                continue
            }
            let stableKey = string(from: metadata.stable_key)
            controller.setPollingEnabled(
                enabledKeys.contains(stableKey),
                forPID: pid)
        }
    }

    private func refreshFuelEconomy() {
        instantaneousFuelEconomyText = controller.instantaneousFuelEconomyText
        averageFuelEconomyText = controller.averageFuelEconomyText
        fuelRateText = controller.fuelRateText
        fuelTripText = controller.fuelTripText
        fuelEconomySourceText = controller.fuelEconomySourceText
        factoryFuelSignalStatusText = controller.factoryFuelSignalStatusText
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
            let previousSelectedVIN = selectedVehicleVIN
            vehicleProfileStore.recordLiveVIN(liveVIN)
            selectedVehicleVIN = liveVIN
            let existingName =
                (vehicleProfileStore.profile(forVIN: liveVIN)?["displayName"] as? String)
            let name = existingName ??
                (liveVIN == previousSelectedVIN
                    ? profileDisplayName
                    : "Jaguar vehicle · \(liveVIN)")
            profileDisplayName = name
            saveVehicleProfile(vin: liveVIN, displayName: name)
            lastPersistedLiveVIN = liveVIN
            lastPersistedReadyVIN = nil
            lastCapabilityMergeVIN = nil
            refreshSavedVehicleProfiles()
        }

        if controller.isActive,
           let liveVIN = controller.vehicleVINText,
           liveVIN.count == 17,
           controller.isReady {
            mergeStandardCapabilitiesIfReady(vin: liveVIN)
            if lastPersistedReadyVIN != liveVIN {
                saveVehicleProfile(
                    vin: liveVIN,
                    displayName: profileDisplayName,
                    includeDiagnosticSnapshot: true)
                lastPersistedReadyVIN = liveVIN
                refreshSavedVehicleProfiles()
            }
        }
        if controller.isActive {
            obdProtocolText = controller.obdProtocolText
            faultScanStatusText = controller.faultScanStatusText
            storedDTCs = controller.storedDTCs
            pendingDTCs = controller.pendingDTCs
            permanentDTCs = controller.permanentDTCs
            storedDTCDisplayRows = controller.storedDTCDisplayRows
            pendingDTCDisplayRows = controller.pendingDTCDisplayRows
            permanentDTCDisplayRows = controller.permanentDTCDisplayRows
            readinessStatusText = controller.readinessStatusText
            readinessMonitorStatus = controller.readinessMonitorStatus
            freezeFrameContext = controller.freezeFrameContext
            diagnosticCapabilityText = controller.diagnosticCapabilityText
            diagnosticCapabilityDetailText = controller.diagnosticCapabilityDetailText
            standardResponderSummary = controller.standardResponderSummary
            supportedPIDSummary = controller.supportedPIDSummary
            standardVINText = controller.standardVINText
            standardLiveValueRows = controller.standardLiveValueRows
        } else if let profile = vehicleProfileStore.profile(forVIN: selectedVehicleVIN ?? "") {
            restoreSavedDiagnosticSnapshot(profile)
        } else {
            resetOfflineDiagnosticSnapshot()
        }
        languageTags = controller.availableLanguageTags
        languageNames = controller.availableLanguageNames
        selectedLanguageID = controller.selectedLanguageTag
        measurementKeys = controller.availableMeasurementSystemKeys
        measurementNames = controller.availableMeasurementSystemNames
        selectedMeasurementID = controller.selectedMeasurementSystemKey
        linkVersionText = controller.linkVersionText
        isActive = controller.isActive
        isReady = controller.isReady
        diagnosticParameters = loadDiagnosticParameters()
        refreshDashboardSelection()
        recordedSampleCount = Int(clamping: controller.recordedSampleCount)
        refreshFuelEconomy()
    }
}
