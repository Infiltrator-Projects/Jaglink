// SPDX-License-Identifier: GPL-3.0-or-later
import Combine
import Foundation

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

/**
 * Jaguar-only state layered over LINK's complete standard product model.
 *
 * LINK owns connection selection, saved vehicles, standard SAE snapshots,
 * polling, dashboard selection, units, languages and evidence export. This
 * class owns only X400 identity, Jaguar DTC presentation and fuel summaries.
 */
@MainActor
final class ConnectionViewModel: LinkStandardProductViewModel,
    @preconcurrency JagLinkDiagnosticsControllerDelegate {

    @Published private(set) var vehiclePlatformText = "Jaguar vehicle identity pending"
    @Published private(set) var vehicleConfigurationText = "Waiting for standard VIN"
    @Published private(set) var vehiclePowertrainText = "Waiting for standard VIN"
    @Published private(set) var vehicleBuildText = "Waiting for standard VIN"
    @Published private(set) var storedDTCDisplayRows = [String]()
    @Published private(set) var pendingDTCDisplayRows = [String]()
    @Published private(set) var permanentDTCDisplayRows = [String]()
    @Published private(set) var jaguarNetworks = [JaguarNetworkInfo]()
    @Published private(set) var profileDisplayName = "Jaguar vehicle"
    @Published private(set) var instantaneousFuelEconomyText = "Unavailable"
    @Published private(set) var averageFuelEconomyText = "Unavailable"
    @Published private(set) var fuelRateText = "Unavailable"
    @Published private(set) var fuelTripText = "Unavailable"
    @Published private(set) var fuelEconomySourceText = "Unavailable"
    @Published private(set) var factoryFuelSignalStatusText =
        "Jaguar factory fuel signal not yet enabled"

    private let jaguarController: JagLinkDiagnosticsController

    init() {
        let controller = JagLinkDiagnosticsController()
        jaguarController = controller
        let version: String
        if let rawVersion = jaglink_version() {
            version = String(cString: rawVersion)
        } else {
            version = "Unknown"
        }
        super.init(
            controller: controller,
            configuration: LinkStandardProductConfiguration(
                productName: "JAGLINK",
                productNamespace: "jaglink",
                manufacturerName: "Jaguar",
                vehicleName: "Jaguar vehicle",
                versionText: version))
        migrateLegacyLanguagePreference()
        loadJaguarProfile()
        controller.delegate = self
        refreshStandardState()
    }

    func diagnosticsControllerDidUpdate(
        _ controller: JagLinkDiagnosticsController
    ) {
        refreshStandardState()
    }

    override func productWillSaveVehicleProfile(
        _ profile: inout [AnyHashable: Any],
        vin: String
    ) {
        profile["displayName"] = profileDisplayName
        profile["platform"] = vehiclePlatformText
        profile["configuration"] = vehicleConfigurationText
        profile["powertrain"] = vehiclePowertrainText
        profile["build"] = vehicleBuildText
        profile["storedDTCDisplayRows"] = jaguarController.storedDTCDisplayRows
        profile["pendingDTCDisplayRows"] = jaguarController.pendingDTCDisplayRows
        profile["permanentDTCDisplayRows"] = jaguarController.permanentDTCDisplayRows
    }

    override func productDidRestoreVehicleProfile(
        _ profile: [AnyHashable: Any],
        vin: String
    ) {
        profileDisplayName = (profile["displayName"] as? String) ?? "Jaguar vehicle"
        vehiclePlatformText = (profile["platform"] as? String)
            ?? "Saved Jaguar profile"
        vehicleConfigurationText = (profile["configuration"] as? String)
            ?? "Saved vehicle configuration"
        vehiclePowertrainText = (profile["powertrain"] as? String)
            ?? "Saved powertrain information"
        vehicleBuildText = (profile["build"] as? String)
            ?? "Saved build information"
        storedDTCDisplayRows =
            (profile["storedDTCDisplayRows"] as? [String]) ?? storedDTCs
        pendingDTCDisplayRows =
            (profile["pendingDTCDisplayRows"] as? [String]) ?? pendingDTCs
        permanentDTCDisplayRows =
            (profile["permanentDTCDisplayRows"] as? [String]) ?? permanentDTCs
    }

    override func productDidRefreshStandardState() {
        if isActive {
            vehiclePlatformText = jaguarController.vehiclePlatformText
            vehicleConfigurationText = jaguarController.vehicleConfigurationText
            vehiclePowertrainText = jaguarController.vehiclePowertrainText
            vehicleBuildText = jaguarController.vehicleBuildText
            storedDTCDisplayRows = jaguarController.storedDTCDisplayRows
            pendingDTCDisplayRows = jaguarController.pendingDTCDisplayRows
            permanentDTCDisplayRows = jaguarController.permanentDTCDisplayRows
            if let vin = selectedVehicleVIN,
               let name = vehicleProfileStore.profile(forVIN: vin)?["displayName"]
                    as? String {
                profileDisplayName = name
            }
        } else if selectedVehicleVIN == nil {
            vehiclePlatformText = "Jaguar vehicle identity pending"
            vehicleConfigurationText = "Waiting for standard VIN"
            vehiclePowertrainText = "Waiting for standard VIN"
            vehicleBuildText = "Waiting for standard VIN"
            storedDTCDisplayRows = []
            pendingDTCDisplayRows = []
            permanentDTCDisplayRows = []
        }
        instantaneousFuelEconomyText = jaguarController.instantaneousFuelEconomyText
        averageFuelEconomyText = jaguarController.averageFuelEconomyText
        fuelRateText = jaguarController.fuelRateText
        fuelTripText = jaguarController.fuelTripText
        fuelEconomySourceText = jaguarController.fuelEconomySourceText
        factoryFuelSignalStatusText = jaguarController.factoryFuelSignalStatusText
    }

    private func migrateLegacyLanguagePreference() {
        let defaults = UserDefaults.standard
        guard defaults.object(forKey: "link.displayLanguage") == nil,
              let legacy = defaults.string(forKey: "jaglink.language") else {
            return
        }
        jaguarController.setSelectedLanguageTag(
            LinkInterfaceLanguage.canonical(
                legacy,
                aliases: ["en": "en-AU", "de": "de-DE", "pl": "pl-PL"],
                fallback: "en-AU"))
    }

    private func cString(_ value: UnsafePointer<CChar>?) -> String {
        guard let value else { return "" }
        return String(cString: value)
    }

    private func loadJaguarProfile() {
        guard let profile = jaglink_jaguar_x400_profile() else { return }
        profileDisplayName = cString(profile.pointee.display_name)
        guard let networks = profile.pointee.networks else { return }
        jaguarNetworks = (0..<Int(profile.pointee.network_count)).compactMap { index in
            let network = networks[index]
            let key = cString(network.key)
            guard !key.isEmpty else { return nil }
            return JaguarNetworkInfo(
                id: key,
                name: cString(network.name),
                kind: cString(jaglink_jaguar_network_kind_name(network.kind)),
                role: cString(jaglink_jaguar_network_role_name(network.role)),
                nominalBaud: network.nominal_baud,
                status: cString(jaglink_jaguar_definition_status_name(network.status)),
                provenance: cString(network.provenance))
        }
    }
}
