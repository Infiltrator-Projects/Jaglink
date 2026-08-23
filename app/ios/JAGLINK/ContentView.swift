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

private let jagDashboardColumns = [
    GridItem(.flexible(), spacing: 14),
    GridItem(.flexible(), spacing: 14)
]

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
        HStack(spacing: 7) {
            Circle()
                .fill(active ? JagPalette.racingGreen : JagPalette.chrome.opacity(0.72))
                .frame(width: 7, height: 7)
            Text(text.uppercased())
                .font(.caption2.weight(.bold))
                .tracking(0.8)
                .lineLimit(1)
        }
        .foregroundStyle(JagPalette.ivory)
        .padding(.horizontal, 10)
        .padding(.vertical, 7)
        .background(Capsule().fill(JagPalette.panel))
        .overlay(Capsule().stroke(JagPalette.warmMetal.opacity(0.34), lineWidth: 1))
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
        VStack(alignment: .leading, spacing: 14) {
            Label(title, systemImage: systemImage)
                .font(.headline.weight(.semibold))
                .foregroundStyle(JagPalette.ivory)
            content
        }
        .padding(16)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(
            RoundedRectangle(cornerRadius: 18, style: .continuous)
                .fill(JagPalette.panel)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 18, style: .continuous)
                .stroke(JagPalette.warmMetal.opacity(0.30), lineWidth: 1)
        )
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
        NavigationLink {
            destination()
        } label: {
            JagTileFace(title: title, subtitle: subtitle, symbol: symbol)
        }
        .buttonStyle(.plain)
    }
}

private struct JagActionTile: View {
    let title: String
    let subtitle: String
    let symbol: String
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            JagTileFace(title: title, subtitle: subtitle, symbol: symbol)
        }
        .buttonStyle(.plain)
    }
}

private struct JagTileFace: View {
    let title: String
    let subtitle: String
    let symbol: String

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Image(systemName: symbol)
                .font(.system(size: 24, weight: .semibold))
                .foregroundStyle(JagPalette.warmMetal)
                .frame(width: 30, height: 30, alignment: .leading)

            Text(title)
                .font(.headline.weight(.semibold))
                .foregroundStyle(JagPalette.ivory)
                .lineLimit(1)

            Text(subtitle)
                .font(.caption)
                .foregroundStyle(JagPalette.mutedIvory)
                .lineLimit(2)
                .fixedSize(horizontal: false, vertical: true)

            Spacer(minLength: 0)

            HStack {
                Spacer()
                Image(systemName: "chevron.right")
                    .font(.caption.weight(.bold))
                    .foregroundStyle(JagPalette.chrome.opacity(0.76))
            }
        }
        .frame(maxWidth: .infinity, minHeight: 118, alignment: .leading)
        .padding(16)
        .background(
            RoundedRectangle(cornerRadius: 19, style: .continuous)
                .fill(
                    LinearGradient(
                        colors: [JagPalette.panelRaised.opacity(0.72), JagPalette.panel],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
        )
        .overlay(
            RoundedRectangle(cornerRadius: 19, style: .continuous)
                .stroke(JagPalette.warmMetal.opacity(0.30), lineWidth: 1)
        )
    }
}

private struct JagMetricTile: View {
    let parameter: DiagnosticParameter
    let toggleFavourite: () -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 9) {
            HStack(alignment: .top, spacing: 6) {
                Text(parameter.title)
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
        self
            .background(JagPalette.cockpit.ignoresSafeArea())
            .navigationTitle(title)
            .navigationBarTitleDisplayMode(.inline)
            .toolbarBackground(JagPalette.cockpit, for: .navigationBar)
            .toolbarBackground(.visible, for: .navigationBar)
            .toolbarColorScheme(.dark, for: .navigationBar)
    }
}

struct ContentView: View {
    @StateObject private var model = ConnectionViewModel()

    var body: some View {
        NavigationStack {
            ZStack {
                LinearGradient(
                    colors: [JagPalette.cockpit, JagPalette.deepGreen],
                    startPoint: .top,
                    endPoint: .bottomTrailing
                )
                .ignoresSafeArea()

                ScrollView {
                    VStack(alignment: .leading, spacing: 18) {
                        header
                        tileGrid
                    }
                    .padding(.horizontal, 20)
                    .padding(.top, 18)
                    .padding(.bottom, 30)
                }
            }
            .toolbar(.hidden, for: .navigationBar)
            .tint(JagPalette.warmMetal)
        }
    }

    private var header: some View {
        ViewThatFits(in: .horizontal) {
            HStack(alignment: .center, spacing: 14) {
                brandIdentity
                Spacer(minLength: 8)
                JagStatusPill(text: model.statusText, active: model.isActive)
            }

            VStack(alignment: .leading, spacing: 11) {
                brandIdentity
                JagStatusPill(text: model.statusText, active: model.isActive)
            }
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
                Text("X400 · JAGUAR DIAGNOSTICS")
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

    private var tileGrid: some View {
        LazyVGrid(columns: jagDashboardColumns, spacing: 14) {
            JagActionTile(
                title: "Connect",
                subtitle: model.isActive ? "Disconnect vehicle link" : "Adapter and vehicle link",
                symbol: model.isActive ? "cable.connector.slash" : "cable.connector"
            ) {
                model.isActive ? model.disconnect() : model.connect()
            }

            JagHomeTile("Live Data", "Sensors and values", "waveform.path.ecg") {
                JagLiveDataView(model: model)
            }

            JagHomeTile("Faults", "Stored, pending, permanent", "exclamationmark.triangle.fill") {
                JagFaultsView(model: model)
            }

            JagHomeTile("Vehicle", "Identity and profile", "car.side.fill") {
                JagVehicleView(model: model)
            }

            JagHomeTile("Modules", "Networks and capabilities", "square.stack.3d.up.fill") {
                JagModulesView(model: model)
            }

            JagHomeTile("Dashboard", "At-a-glance measurements", "gauge.with.dots.needle.67percent") {
                JagDashboardView(model: model)
            }

            JagHomeTile("Evidence", "Session log and CSV", "doc.text.magnifyingglass") {
                JagEvidenceView(model: model)
            }

            JagHomeTile("Settings", "Adapter and app details", "gearshape.fill") {
                JagSettingsView(model: model)
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
                    jagValueRow("Adapter", model.peripheralName, icon: "antenna.radiowaves.left.and.right")
                    jagDivider
                    jagValueRow("ELM identity", model.adapterIdentifier, icon: "cpu")
                    jagDivider
                    jagValueRow("Status", model.statusText, icon: "checkmark.seal")
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
                                    Text(network.status.uppercased())
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
                JagPanel(title: "Fault Memory", systemImage: "exclamationmark.triangle.fill") {
                    HStack {
                        Text(model.faultScanStatusText)
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
        .jagDiagnosticScreen("Faults")
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
        .jagDiagnosticScreen("Evidence")
    }
}

private struct JagSettingsView: View {
    @ObservedObject var model: ConnectionViewModel

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
        Text(label)
            .font(.subheadline)
            .foregroundStyle(JagPalette.mutedIvory)
        Spacer()
        Text(value)
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
            Text(title)
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
