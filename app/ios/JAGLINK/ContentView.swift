// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import SwiftUI

private enum JagPalette {
    static let racingGreen = Color(red: 0.035, green: 0.255, blue: 0.165)
    static let deepGreen = Color(red: 0.020, green: 0.095, blue: 0.068)
    static let cockpit = Color(red: 0.025, green: 0.055, blue: 0.045)
    static let panel = Color(red: 0.035, green: 0.125, blue: 0.090)
    static let panelRaised = Color(red: 0.050, green: 0.165, blue: 0.115)
    static let ivory = Color(red: 0.955, green: 0.932, blue: 0.865)
    static let mutedIvory = Color(red: 0.790, green: 0.775, blue: 0.720)
    static let chrome = Color(red: 0.735, green: 0.755, blue: 0.745)
    static let warmMetal = Color(red: 0.735, green: 0.635, blue: 0.390)
    static let jaguarRed = Color(red: 0.585, green: 0.055, blue: 0.075)
    static let amber = Color(red: 0.790, green: 0.525, blue: 0.165)
}

private let jagLinkTheme = LinkDiagnosticTheme(
    backgroundTop: JagPalette.cockpit,
    backgroundMiddle: JagPalette.deepGreen,
    backgroundBottom: JagPalette.deepGreen,
    panel: JagPalette.panel,
    panelRaised: JagPalette.panelRaised,
    primaryText: JagPalette.ivory,
    secondaryText: JagPalette.chrome,
    mutedText: JagPalette.mutedIvory,
    border: JagPalette.warmMetal.opacity(0.30),
    accent: JagPalette.warmMetal,
    success: JagPalette.racingGreen,
    warning: JagPalette.amber,
    fault: JagPalette.jaguarRed,
    typography: LinkDiagnosticTypography(
        display: .system(size: 29, weight: .semibold, design: .serif),
        body: .body,
        bodyBold: .body.bold(),
        subheadline: .subheadline,
        subheadlineBold: .subheadline.bold(),
        headline: .headline,
        caption: .caption,
        captionBold: .caption.bold(),
        caption2: .caption2,
        caption2Bold: .caption2.bold(),
        title3: .title3,
        title2: .title2.bold()))

private let jagDashboardColumns = LinkDiagnosticLayout.dashboardColumns

private struct JaguarBadge: View {
    var size: CGFloat = 52

    var body: some View {
        Image("JAGLINKEmblem")
            .resizable()
            .scaledToFit()
            .frame(width: size, height: size)
            .shadow(color: .black.opacity(0.32), radius: 7, x: 0, y: 4)
            .accessibilityHidden(true)
    }
}

private struct JagStatusPill: View {
    let text: String
    let active: Bool

    var body: some View {
        LinkStatusPill(text: text, active: active)
    }
}

private struct JagPanel<Content: View>: View {
    let title: String
    let systemImage: String
    let content: Content

    init(title: String, systemImage: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.systemImage = systemImage
        self.content = content()
    }

    var body: some View {
        LinkLabeledPanel(title: title, systemImage: systemImage) {
            content
        }
    }
}

private struct JagHomeTile<Destination: View>: View {
    let title: String
    let subtitle: String
    let symbol: String
    let destination: () -> Destination

    init(
        _ title: String,
        _ subtitle: String,
        _ symbol: String,
        @ViewBuilder destination: @escaping () -> Destination
    ) {
        self.title = title
        self.subtitle = subtitle
        self.symbol = symbol
        self.destination = destination
    }

    var body: some View {
        LinkHomeTile(title, subtitle, symbol, destination: destination)
    }
}

private struct JagActionTile: View {
    let title: String
    let subtitle: String
    let symbol: String
    let action: () -> Void

    var body: some View {
        LinkActionTile(title: title, subtitle: subtitle, symbol: symbol, action: action)
    }
}

private struct JagTileFace: View {
    let title: String
    let subtitle: String
    let symbol: String

    var body: some View {
        LinkTileFace(title: title, subtitle: subtitle, symbol: symbol)
    }
}

private struct JagMetricTile: View {
    let parameter: DiagnosticParameter
    let toggleFavourite: () -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 9) {
            HStack(alignment: .top, spacing: 6) {
                Text(LocalizedStringKey(parameter.title))
                    .font(.caption.weight(.semibold))
                    .foregroundStyle(JagPalette.ivory)
                    .lineLimit(2)
                Spacer(minLength: 2)
                Button(action: toggleFavourite) {
                    Image(systemName: parameter.favourite ? "star.fill" : "star")
                        .font(.caption.weight(.bold))
                        .foregroundStyle(parameter.favourite ? JagPalette.warmMetal : JagPalette.chrome)
                }
                .buttonStyle(.plain)
            }

            Text(parameter.formattedValue)
                .font(.system(size: 21, weight: .bold, design: .rounded))
                .monospacedDigit()
                .foregroundStyle(JagPalette.warmMetal)
                .lineLimit(1)
                .minimumScaleFactor(0.62)

            Text(parameter.id)
                .font(.caption2.monospaced())
                .foregroundStyle(JagPalette.chrome.opacity(0.62))
                .lineLimit(1)
                .truncationMode(.middle)
        }
        .frame(maxWidth: .infinity, minHeight: 112, alignment: .leading)
        .padding(13)
        .background(
            RoundedRectangle(cornerRadius: 15, style: .continuous)
                .fill(JagPalette.cockpit.opacity(0.56))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 15, style: .continuous)
                .stroke(JagPalette.warmMetal.opacity(0.28), lineWidth: 0.8)
        )
    }
}

private extension View {
    func jagDiagnosticScreen(_ title: String) -> some View {
        linkDiagnosticScreen(title)
    }
}

struct ContentView: View {
    @StateObject private var model = ConnectionViewModel()

    var body: some View {
        LinkCommandCentreShell(
            showProgress: model.isActive && !model.isReady,
            header: { header },
            progress: { connectionProgress },
            connection: { connectionCard },
            primary: { primaryGrid },
            tools: { supportingTools })
            .linkDiagnosticTheme(jagLinkTheme)
    }

    private var header: some View {
        LinkBrandHeader {
            brandIdentity
        } status: {
            JagStatusPill(text: model.statusText, active: model.isReady)
        }
    }

    private var brandIdentity: some View {
        HStack(spacing: 14) {
            JaguarBadge(size: 54)
            VStack(alignment: .leading, spacing: 3) {
                Text("JAGLINK")
                    .font(.system(size: 29, weight: .semibold, design: .serif))
                    .tracking(3.8)
                    .foregroundStyle(JagPalette.ivory)
                Text("JAGUAR · LINK DIAGNOSTICS")
                    .font(.caption2.weight(.bold))
                    .tracking(1.4)
                    .foregroundStyle(JagPalette.warmMetal)
                Text(model.profileDisplayName)
                    .font(.caption)
                    .foregroundStyle(JagPalette.mutedIvory)
                    .lineLimit(1)
            }
        }
    }

    private var connectionCard: some View {
        LinkPanel {
            VStack(alignment: .leading, spacing: 12) {
                HStack(alignment: .firstTextBaseline) {
                    VStack(alignment: .leading, spacing: 3) {
                        Text(model.isActive ? "Diagnostic session" : "Vehicle connection")
                            .font(.headline)
                            .foregroundStyle(JagPalette.ivory)
                        Text(model.statusText)
                            .font(.caption)
                            .foregroundStyle(JagPalette.mutedIvory)
                            .fixedSize(horizontal: false, vertical: true)
                    }
                    Spacer(minLength: 12)
                    Image(systemName: model.isReady
                          ? "checkmark.circle.fill"
                          : model.isActive ? "dot.radiowaves.left.and.right" : "cable.connector")
                        .foregroundStyle(model.isReady ? JagPalette.racingGreen : JagPalette.warmMetal)
                }

                Button {
                    model.isActive ? model.disconnect() : model.connect()
                } label: {
                    Label(model.isActive ? "Disconnect" : "Connect to vehicle",
                          systemImage: model.isActive ? "cable.connector.slash" : "cable.connector")
                        .font(.subheadline.weight(.semibold))
                        .foregroundStyle(JagPalette.deepGreen)
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 11)
                        .background(
                            RoundedRectangle(cornerRadius: 12, style: .continuous)
                                .fill(JagPalette.warmMetal))
                }
                .buttonStyle(.plain)

                if model.isReady {
                    HStack(spacing: 9) {
                        Image(systemName: "car.side.fill")
                            .foregroundStyle(JagPalette.warmMetal)
                        VStack(alignment: .leading, spacing: 2) {
                            Text(model.profileDisplayName)
                                .font(.subheadline.weight(.semibold))
                                .foregroundStyle(JagPalette.ivory)
                            Text(model.vehicleVINText)
                                .font(.caption2)
                                .foregroundStyle(JagPalette.mutedIvory)
                                .lineLimit(1)
                        }
                    }
                } else if !model.isActive {
                    Text("Connect once to identify the vehicle, faults, modules and supported live data.")
                        .font(.caption)
                        .foregroundStyle(JagPalette.mutedIvory)
                }
            }
        }
    }

    private var connectionProgress: some View {
        LinkPanel {
            VStack(alignment: .leading, spacing: 7) {
                Label("Connecting to vehicle", systemImage: "dot.radiowaves.left.and.right")
                    .font(.headline)
                    .foregroundStyle(JagPalette.ivory)
                Text(model.statusText)
                    .font(.subheadline)
                    .foregroundStyle(JagPalette.chrome)
                    .fixedSize(horizontal: false, vertical: true)
                if model.peripheralName != "No adapter" {
                    Text(model.peripheralName)
                        .font(.caption)
                        .foregroundStyle(JagPalette.mutedIvory)
                }
            }
        }
    }

    private var primaryGrid: some View {
        LinkDiagnosticGrid {
            LinkTaskTile(.vehicle) { JagVehicleView(model: model) }
            LinkTaskTile(.log) { JagEvidenceView(model: model) }
            LinkTaskTile(.errors) { JagFaultsView(model: model) }
            LinkTaskTile(.dashboard) { JagDashboardView(model: model) }
            LinkTaskTile(.table) { JagTableView(model: model) }
            LinkTaskTile(.graph) { JagGraphView(model: model) }
            LinkTaskTile(.tests) { JagTestsView(model: model) }
            LinkTaskTile(.services) { JagServicesView(model: model) }
        }
    }

    private var supportingTools: some View {
        LinkPanel {
            VStack(alignment: .leading, spacing: 7) {
                LinkSectionHeader(title: "Settings", kicker: "Application")
                LinkCompactLink("Settings", "Adapter and app preferences", "gearshape.fill") {
                    JagSettingsView(model: model)
                }
            }
        }
    }
}

private struct JagVehicleView: View {
    @ObservedObject var model: ConnectionViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Vehicle", systemImage: "car.side.fill") {
                    jagValueRow("Profile", model.profileDisplayName, icon: "shield.lefthalf.filled")
                    jagDivider
                    jagValueRow("VIN", model.vehicleVINText, icon: "number")
                    jagDivider
                    jagValueRow("Platform", model.vehiclePlatformText, icon: "car.side")
                    jagDivider
                    jagValueRow("Configuration", model.vehicleConfigurationText, icon: "slider.horizontal.3")
                    jagDivider
                    jagValueRow("Powertrain", model.vehiclePowertrainText, icon: "engine.combustion")
                    jagDivider
                    jagValueRow("Build", model.vehicleBuildText, icon: "building.2")
                    jagDivider
                    jagValueRow("Adapter", model.peripheralName, icon: "antenna.radiowaves.left.and.right")
                    jagDivider
                    jagValueRow("ELM identity", model.adapterIdentifier, icon: "cpu")
                    jagDivider
                    jagValueRow("Status", model.statusText, icon: "checkmark.seal")
                }
                JagPanel(title: "Control units", systemImage: "square.stack.3d.up.fill") {
                    NavigationLink {
                        JagModulesView(model: model)
                    } label: {
                        HStack(spacing: 12) {
                            VStack(alignment: .leading, spacing: 3) {
                                Text("Networks and module inventory")
                                    .font(.headline)
                                    .foregroundStyle(JagPalette.ivory)
                                Text("Open discovered Jaguar networks and capability details")
                                    .font(.caption)
                                    .foregroundStyle(JagPalette.mutedIvory)
                            }
                            Spacer()
                            Image(systemName: "chevron.right")
                                .foregroundStyle(JagPalette.warmMetal)
                        }
                    }
                    .buttonStyle(.plain)
                }

            }
            .padding(16)
        }
        .jagDiagnosticScreen("Vehicle")
    }
}

private struct JagModulesView: View {
    @ObservedObject var model: ConnectionViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "X400 Networks", systemImage: "point.3.connected.trianglepath.dotted") {
                    ForEach(model.jaguarNetworks) { network in
                        HStack(alignment: .top, spacing: 12) {
                            Image(systemName: networkIcon(network.kind))
                                .font(.title3)
                                .foregroundStyle(JagPalette.warmMetal)
                                .frame(width: 28)

                            VStack(alignment: .leading, spacing: 4) {
                                HStack {
                                    Text(network.name)
                                        .font(.headline)
                                        .foregroundStyle(JagPalette.ivory)
                                    Spacer()
                                    Text(LocalizedStringKey(network.status)).textCase(.uppercase)
                                        .font(.caption2.bold())
                                        .foregroundStyle(JagPalette.racingGreen)
                                }
                                Text("\(network.kind.uppercased()) · \(network.role)")
                                    .font(.subheadline)
                                    .foregroundStyle(JagPalette.mutedIvory)
                                Text(rateText(network.nominalBaud))
                                    .font(.caption.monospaced().weight(.semibold))
                                    .foregroundStyle(JagPalette.warmMetal)
                                Text(network.provenance)
                                    .font(.caption)
                                    .foregroundStyle(JagPalette.chrome.opacity(0.72))
                            }
                        }
                        .padding(.vertical, 4)

                        if network.id != model.jaguarNetworks.last?.id { jagDivider }
                    }
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Modules")
    }
}

private struct JagFaultsView: View {
    @ObservedObject var model: ConnectionViewModel

    private var total: Int {
        model.storedDTCs.count + model.pendingDTCs.count + model.permanentDTCs.count
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Errors", systemImage: "exclamationmark.triangle.fill") {
                    HStack {
                        Text(LocalizedStringKey(model.faultScanStatusText))
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.mutedIvory)
                        Spacer()
                        Text("\(total)")
                            .font(.title2.monospacedDigit().weight(.bold))
                            .foregroundStyle(total == 0 ? JagPalette.racingGreen : JagPalette.jaguarRed)
                    }
                    faultRows(title: "Stored", codes: model.storedDTCs)
                    faultRows(title: "Pending", codes: model.pendingDTCs)
                    faultRows(title: "Permanent", codes: model.permanentDTCs)
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Errors")
    }
}

private struct JagLiveDataView: View {
    @ObservedObject var model: ConnectionViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                if model.diagnosticParameters.isEmpty {
                    JagPanel(title: "Live Data", systemImage: "waveform.path.ecg") {
                        Text("Connect to the vehicle to populate live parameters.")
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.mutedIvory)
                    }
                } else {
                    LazyVGrid(columns: jagDashboardColumns, spacing: 12) {
                        ForEach(model.diagnosticParameters) { parameter in
                            JagMetricTile(parameter: parameter) {
                                model.toggleFavourite(stableKey: parameter.id)
                            }
                        }
                    }
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Live Data")
    }
}

private struct JagDashboardView: View {
    @ObservedObject var model: ConnectionViewModel

    private var totalFaultCount: Int {
        model.storedDTCs.count + model.pendingDTCs.count + model.permanentDTCs.count
    }

    private var displayed: [DiagnosticParameter] {
        let favourites = model.diagnosticParameters.filter { $0.favourite }
        if !favourites.isEmpty { return Array(favourites.prefix(6)) }
        return Array(model.diagnosticParameters.prefix(6))
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Vehicle Summary", systemImage: "gauge.with.dots.needle.67percent") {
                    jagValueRow("Connection", model.statusText, icon: "link")
                    jagDivider
                    jagValueRow("Fault records", "\(totalFaultCount)", icon: "exclamationmark.triangle")
                    jagDivider
                    jagValueRow("Recorded samples", "\(model.recordedSampleCount)", icon: "waveform.path.ecg")
                }

                if displayed.isEmpty {
                    JagPanel(title: "Measurements", systemImage: "waveform.path.ecg") {
                        Text("Connect to the vehicle to populate dashboard measurements.")
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.mutedIvory)
                    }
                } else {
                    LazyVGrid(columns: jagDashboardColumns, spacing: 12) {
                        ForEach(displayed) { parameter in
                            JagMetricTile(parameter: parameter) {
                                model.toggleFavourite(stableKey: parameter.id)
                            }
                        }
                    }
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Dashboard")
    }
}

private struct JagEvidenceView: View {
    @ObservedObject var model: ConnectionViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Diagnostic Evidence", systemImage: "doc.text.magnifyingglass") {
                    jagValueRow("Fault scan", model.faultScanStatusText, icon: "exclamationmark.triangle")
                    jagDivider
                    jagValueRow("Recorded samples", "\(model.recordedSampleCount)", icon: "waveform.path.ecg")
                    jagDivider
                    Button {
                        model.prepareCSVExport()
                    } label: {
                        Label("Prepare diagnostic CSV", systemImage: "square.and.arrow.down")
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .fontWeight(.semibold)
                            .foregroundStyle(JagPalette.warmMetal)
                    }

                    if let url = model.csvExportURL {
                        ShareLink(item: url) {
                            Label("Share diagnostic CSV", systemImage: "square.and.arrow.up")
                                .frame(maxWidth: .infinity, alignment: .leading)
                                .fontWeight(.semibold)
                                .foregroundStyle(JagPalette.ivory)
                        }
                    }
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Log")
    }
}

private struct JagTableView: View {
    @ObservedObject var model: ConnectionViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Table", systemImage: "tablecells") {
                    if model.diagnosticParameters.isEmpty {
                        Text("Connect to populate supported diagnostic parameters.")
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.mutedIvory)
                    } else {
                        ForEach(model.diagnosticParameters) { parameter in
                            HStack(alignment: .firstTextBaseline, spacing: 10) {
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(parameter.title)
                                        .font(.subheadline.weight(.semibold))
                                        .foregroundStyle(JagPalette.ivory)
                                    Text("\(parameter.protocolName) · \(parameter.shortName)")
                                        .font(.caption2)
                                        .foregroundStyle(JagPalette.mutedIvory)
                                }
                                Spacer()
                                Text(parameter.formattedValue)
                                    .font(.subheadline.monospacedDigit().weight(.semibold))
                                    .foregroundStyle(parameter.isAvailable ? JagPalette.warmMetal : JagPalette.mutedIvory)
                            }
                            .padding(.vertical, 6)
                            if parameter.id != model.diagnosticParameters.last?.id { jagDivider }
                        }
                    }
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Table")
    }
}

private struct JagGraphView: View {
    @ObservedObject var model: ConnectionViewModel

    private var graphed: [DiagnosticParameter] {
        let values = model.diagnosticParameters.filter { !$0.history.isEmpty }
        let favourites = values.filter { $0.favourite }
        return Array((favourites.isEmpty ? values : favourites).prefix(4))
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                if graphed.isEmpty {
                    JagPanel(title: "Graph", systemImage: "chart.xyaxis.line") {
                        Text("Collect live samples to populate parameter history.")
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.mutedIvory)
                    }
                } else {
                    ForEach(graphed) { parameter in
                        JagPanel(title: parameter.title, systemImage: "chart.xyaxis.line") {
                            jagValueRow("Current", parameter.formattedValue, icon: "waveform.path.ecg")
                            jagDivider
                            jagValueRow("History", "\(parameter.history.count) samples", icon: "clock.arrow.circlepath")
                            Text("The shared LINK telemetry history is retained for time-series presentation.")
                                .font(.caption)
                                .foregroundStyle(JagPalette.mutedIvory)
                        }
                    }
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Graph")
    }
}

private struct JagTestsView: View {
    @ObservedObject var model: ConnectionViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Readiness", systemImage: "checkmark.square.fill") {
                    jagValueRow("Status", model.readinessStatusText, icon: "checklist")
                    if model.readinessMonitorStatus.isEmpty {
                        Text("No readiness-monitor detail has been returned yet.")
                            .font(.caption)
                            .foregroundStyle(JagPalette.mutedIvory)
                    } else {
                        ForEach(model.readinessMonitorStatus, id: \.self) { row in
                            Text(row)
                                .font(.subheadline)
                                .foregroundStyle(JagPalette.chrome)
                        }
                    }
                }

                JagPanel(title: "Freeze-frame context", systemImage: "camera.metering.matrix") {
                    if model.freezeFrameContext.isEmpty {
                        Text("No standard freeze-frame context captured.")
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.mutedIvory)
                    } else {
                        ForEach(model.freezeFrameContext, id: \.self) { row in
                            Text(row)
                                .font(.subheadline)
                                .foregroundStyle(JagPalette.chrome)
                        }
                    }
                }

                JagPanel(title: "Additional tests", systemImage: "checkmark.seal") {
                    Text("Verified standard monitor results and Jaguar self-tests belong here as they are implemented. Unsupported tests are never fabricated.")
                        .font(.caption)
                        .foregroundStyle(JagPalette.mutedIvory)
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Tests")
    }
}

private struct JagServicesView: View {
    @ObservedObject var model: ConnectionViewModel

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Services", systemImage: "wrench.and.screwdriver.fill") {
                    Text(model.isActive
                         ? "No verified service procedure is enabled for this session."
                         : "Connect to the vehicle to evaluate supported service procedures.")
                        .font(.headline)
                        .foregroundStyle(JagPalette.ivory)
                    Text("A service action appears only when its target module, prerequisites, request sequence and safety behaviour are explicitly supported. Unknown or destructive operations remain unavailable.")
                        .font(.caption)
                        .foregroundStyle(JagPalette.mutedIvory)
                        .fixedSize(horizontal: false, vertical: true)
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Services")
    }
}

private struct JagSettingsView: View {
    @ObservedObject var model: ConnectionViewModel
    @AppStorage("jaglink.language") private var language = "en-AU"

    private var version: String {
        Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "Unknown"
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 15) {
                JagPanel(title: "Adapter", systemImage: "cable.connector") {
                    jagValueRow("Name", model.peripheralName, icon: "antenna.radiowaves.left.and.right")
                    jagDivider
                    jagValueRow("Identity", model.adapterIdentifier, icon: "cpu")
                    jagDivider
                    jagValueRow("Status", model.statusText, icon: "checkmark.seal")
                }

                JagPanel(title: "Language", systemImage: "globe") {
                    NavigationLink {
                        JagLanguageSelectionView(selection: $language)
                    } label: {
                        HStack(spacing: 12) {
                            Image(systemName: "globe")
                                .foregroundStyle(JagPalette.warmMetal)
                            Text("Language")
                                .font(.headline)
                                .foregroundStyle(JagPalette.ivory)
                            Spacer()
                            Text(JagInterfaceLanguage.displayName(for: language))
                                .font(.subheadline)
                                .foregroundStyle(JagPalette.mutedIvory)
                            Image(systemName: "chevron.right")
                                .font(.caption.weight(.bold))
                                .foregroundStyle(JagPalette.chrome)
                        }
                        .contentShape(Rectangle())
                    }
                    .buttonStyle(.plain)
                }

                JagPanel(title: "Application", systemImage: "gearshape.fill") {
                    jagValueRow("Version", version, icon: "number")
                    jagDivider
                    jagValueRow("Profile", model.profileDisplayName, icon: "car.side.fill")
                    jagDivider
                    jagValueRow("Bundle", Bundle.main.bundleIdentifier ?? "Unknown", icon: "app.badge")
                }
            }
            .padding(16)
        }
        .jagDiagnosticScreen("Settings")
    }
}

private struct JagLanguageSelectionView: View {
    @Binding var selection: String

    var body: some View {
        List {
            ForEach(JagInterfaceLanguage.all) { item in
                Button {
                    selection = item.id
                } label: {
                    HStack(spacing: 12) {
                        Text(item.nativeName)
                            .font(.body.weight(.medium))
                            .foregroundStyle(JagPalette.ivory)
                        Spacer()
                        if JagInterfaceLanguage.canonical(selection) == item.id {
                            Image(systemName: "checkmark")
                                .font(.body.weight(.bold))
                                .foregroundStyle(JagPalette.warmMetal)
                        }
                    }
                    .contentShape(Rectangle())
                }
                .buttonStyle(.plain)
                .listRowBackground(JagPalette.panel)
            }
        }
        .scrollContentBackground(.hidden)
        .background(JagPalette.cockpit)
        .jagDiagnosticScreen("Language")
    }
}

private var jagDivider: some View {
    Rectangle()
        .fill(JagPalette.warmMetal.opacity(0.24))
        .frame(height: 0.7)
}

private func jagValueRow(_ label: String, _ value: String, icon: String) -> some View {
    HStack(spacing: 10) {
        Image(systemName: icon)
            .frame(width: 21)
            .foregroundStyle(JagPalette.warmMetal)
        Text(LocalizedStringKey(label))
            .font(.subheadline)
            .foregroundStyle(JagPalette.mutedIvory)
        Spacer()
        Text(LocalizedStringKey(value))
            .font(.subheadline.weight(.semibold))
            .multilineTextAlignment(.trailing)
            .foregroundStyle(JagPalette.ivory)
    }
}

@ViewBuilder
private func faultRows(title: String, codes: [String]) -> some View {
    HStack(alignment: .top, spacing: 10) {
        Circle()
            .fill(codes.isEmpty ? JagPalette.racingGreen : JagPalette.jaguarRed)
            .frame(width: 8, height: 8)
            .padding(.top, 6)
        VStack(alignment: .leading, spacing: 4) {
            Text(LocalizedStringKey(title))
                .font(.subheadline.weight(.semibold))
                .foregroundStyle(JagPalette.ivory)
            if codes.isEmpty {
                Text("None")
                    .font(.caption)
                    .foregroundStyle(JagPalette.chrome.opacity(0.66))
            } else {
                ForEach(codes, id: \.self) { code in
                    Text(code)
                        .font(.body.monospaced().weight(.semibold))
                        .foregroundStyle(JagPalette.jaguarRed)
                }
            }
        }
        Spacer()
    }
}

private func networkIcon(_ kind: String) -> String {
    switch kind.lowercased() {
    case "can": return "network"
    case "scp": return "wave.3.right"
    case "iso9141", "iso-9141": return "cable.connector"
    case "d2b": return "music.note"
    default: return "circle.hexagongrid"
    }
}

private func rateText(_ baud: UInt32) -> String {
    if baud >= 1_000_000 { return String(format: "%.1f Mbit/s", Double(baud) / 1_000_000.0) }
    if baud >= 1_000 { return String(format: "%.1f kbit/s", Double(baud) / 1_000.0) }
    return "\(baud) bit/s"
}
