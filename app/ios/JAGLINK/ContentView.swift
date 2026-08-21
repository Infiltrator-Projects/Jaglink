// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import SwiftUI

private enum JagPalette {
    static let racingGreen = Color(red: 0.015, green: 0.225, blue: 0.145)
    static let deepGreen = Color(red: 0.010, green: 0.105, blue: 0.075)
    static let forest = Color(red: 0.018, green: 0.155, blue: 0.105)
    static let ivory = Color(red: 0.955, green: 0.932, blue: 0.865)
    static let parchment = Color(red: 0.905, green: 0.875, blue: 0.795)
    static let chrome = Color(red: 0.735, green: 0.755, blue: 0.745)
    static let charcoal = Color(red: 0.075, green: 0.090, blue: 0.082)
    static let jaguarRed = Color(red: 0.585, green: 0.055, blue: 0.075)
    static let warmMetal = Color(red: 0.735, green: 0.635, blue: 0.390)
}

private struct JagPanel<Content: View>: View {
    let title: String
    let systemImage: String
    @ViewBuilder let content: Content

    init(title: String, systemImage: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.systemImage = systemImage
        self.content = content()
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            Label(title.uppercased(), systemImage: systemImage)
                .font(.system(size: 12, weight: .bold, design: .rounded))
                .tracking(1.6)
                .foregroundStyle(JagPalette.racingGreen)

            content
        }
        .padding(17)
        .background(
            RoundedRectangle(cornerRadius: 18, style: .continuous)
                .fill(JagPalette.ivory)
                .shadow(color: JagPalette.deepGreen.opacity(0.13), radius: 10, x: 0, y: 5)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 18, style: .continuous)
                .stroke(JagPalette.chrome.opacity(0.55), lineWidth: 0.8)
        )
    }
}

private struct JagWordmark: View {
    var body: some View {
        HStack(spacing: 15) {
            ZStack {
                Circle()
                    .fill(JagPalette.deepGreen)
                    .overlay(Circle().stroke(JagPalette.chrome, lineWidth: 2))
                Image(systemName: "cat.fill")
                    .font(.system(size: 29, weight: .semibold))
                    .foregroundStyle(JagPalette.ivory)
            }
            .frame(width: 58, height: 58)

            VStack(alignment: .leading, spacing: 2) {
                Text("JAGLINK")
                    .font(.system(size: 28, weight: .semibold, design: .serif))
                    .tracking(4.2)
                    .foregroundStyle(JagPalette.ivory)
                Text("X400  ·  JAGUAR DIAGNOSTICS")
                    .font(.system(size: 10, weight: .semibold, design: .rounded))
                    .tracking(1.7)
                    .foregroundStyle(JagPalette.chrome)
            }
        }
    }
}

struct ContentView: View {
    @StateObject private var model = ConnectionViewModel()

    var body: some View {
        NavigationStack {
            ZStack {
                LinearGradient(
                    colors: [JagPalette.parchment, JagPalette.ivory],
                    startPoint: .top,
                    endPoint: .bottom
                )
                .ignoresSafeArea()

                ScrollView {
                    LazyVStack(spacing: 16) {
                        hero
                        vehiclePanel
                        networkPanel
                        faultPanel
                        liveDataPanel
                        logPanel
                        aboutPanel
                    }
                    .padding(.horizontal, 14)
                    .padding(.bottom, 28)
                }
            }
            .navigationBarTitleDisplayMode(.inline)
            .toolbarBackground(JagPalette.deepGreen, for: .navigationBar)
            .toolbarBackground(.visible, for: .navigationBar)
            .toolbarColorScheme(.dark, for: .navigationBar)
            .toolbar {
                ToolbarItem(placement: .principal) {
                    Text("JAGLINK")
                        .font(.system(size: 15, weight: .semibold, design: .serif))
                        .tracking(3)
                }
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        model.isActive ? model.disconnect() : model.connect()
                    } label: {
                        Image(systemName: model.isActive ? "bolt.slash.fill" : "bolt.fill")
                            .font(.system(size: 15, weight: .bold))
                            .foregroundStyle(model.isActive ? JagPalette.jaguarRed : JagPalette.ivory)
                    }
                    .accessibilityLabel(model.isActive ? "Disconnect" : "Connect")
                }
            }
            .tint(JagPalette.racingGreen)
        }
    }

    private var hero: some View {
        VStack(alignment: .leading, spacing: 16) {
            JagWordmark()

            HStack(spacing: 10) {
                statusPill(
                    title: model.isActive ? "CONNECTED" : "READY",
                    icon: model.isActive ? "link" : "power",
                    colour: model.isActive ? JagPalette.warmMetal : JagPalette.chrome
                )
                statusPill(
                    title: model.profileDisplayName,
                    icon: "car.side.fill",
                    colour: JagPalette.chrome
                )
            }

            Button {
                model.isActive ? model.disconnect() : model.connect()
            } label: {
                HStack {
                    Image(systemName: model.isActive ? "cable.connector.slash" : "cable.connector")
                    Text(model.isActive ? "Disconnect vehicle" : "Connect to X400")
                        .fontWeight(.semibold)
                    Spacer()
                    Image(systemName: "chevron.right")
                        .font(.caption.bold())
                }
                .padding(.horizontal, 16)
                .frame(height: 48)
                .foregroundStyle(JagPalette.deepGreen)
                .background(JagPalette.ivory)
                .clipShape(RoundedRectangle(cornerRadius: 13, style: .continuous))
            }
            .buttonStyle(.plain)
        }
        .padding(18)
        .background(
            LinearGradient(
                colors: [JagPalette.deepGreen, JagPalette.racingGreen],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        )
        .clipShape(RoundedRectangle(cornerRadius: 24, style: .continuous))
        .overlay(
            RoundedRectangle(cornerRadius: 24, style: .continuous)
                .stroke(JagPalette.chrome.opacity(0.65), lineWidth: 1)
        )
        .padding(.top, 12)
    }

    private func statusPill(title: String, icon: String, colour: Color) -> some View {
        Label(title, systemImage: icon)
            .font(.system(size: 10, weight: .bold, design: .rounded))
            .lineLimit(1)
            .padding(.horizontal, 10)
            .padding(.vertical, 7)
            .foregroundStyle(JagPalette.ivory)
            .background(colour.opacity(0.20))
            .overlay(Capsule().stroke(colour.opacity(0.65), lineWidth: 0.8))
            .clipShape(Capsule())
    }

    private var vehiclePanel: some View {
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

    private var networkPanel: some View {
        JagPanel(title: "X400 networks", systemImage: "point.3.connected.trianglepath.dotted") {
            ForEach(model.jaguarNetworks) { network in
                HStack(alignment: .top, spacing: 12) {
                    ZStack {
                        RoundedRectangle(cornerRadius: 10, style: .continuous)
                            .fill(JagPalette.racingGreen.opacity(0.10))
                        Image(systemName: networkIcon(network.kind))
                            .foregroundStyle(JagPalette.racingGreen)
                    }
                    .frame(width: 38, height: 38)

                    VStack(alignment: .leading, spacing: 4) {
                        HStack {
                            Text(network.name)
                                .font(.headline)
                                .foregroundStyle(JagPalette.charcoal)
                            Spacer()
                            Text(network.status.uppercased())
                                .font(.caption2.bold())
                                .foregroundStyle(JagPalette.racingGreen)
                        }
                        Text("\(network.kind.uppercased())  ·  \(network.role)")
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.charcoal.opacity(0.78))
                        Text(rateText(network.nominalBaud))
                            .font(.system(.caption, design: .monospaced).weight(.semibold))
                            .foregroundStyle(JagPalette.warmMetal)
                        Text(network.provenance)
                            .font(.caption)
                            .foregroundStyle(JagPalette.charcoal.opacity(0.58))
                    }
                }
                .padding(.vertical, 3)

                if network.id != model.jaguarNetworks.last?.id { jagDivider }
            }
        }
    }

    private var faultPanel: some View {
        JagPanel(title: "Fault memory", systemImage: "exclamationmark.triangle.fill") {
            Text(model.faultScanStatusText)
                .font(.subheadline)
                .foregroundStyle(JagPalette.charcoal.opacity(0.70))
            faultRows(title: "Stored", codes: model.storedDTCs)
            faultRows(title: "Pending", codes: model.pendingDTCs)
            faultRows(title: "Permanent", codes: model.permanentDTCs)
        }
    }

    private var liveDataPanel: some View {
        JagPanel(title: "Live data", systemImage: "gauge.with.dots.needle.67percent") {
            if model.diagnosticParameters.isEmpty {
                Text("Connect to the vehicle to populate live parameters.")
                    .font(.subheadline)
                    .foregroundStyle(JagPalette.charcoal.opacity(0.60))
            }

            ForEach(model.diagnosticParameters) { parameter in
                HStack(spacing: 12) {
                    VStack(alignment: .leading, spacing: 2) {
                        Text(parameter.title)
                            .font(.subheadline.weight(.semibold))
                            .foregroundStyle(JagPalette.charcoal)
                        Text(parameter.id)
                            .font(.caption2.monospaced())
                            .foregroundStyle(JagPalette.charcoal.opacity(0.50))
                    }
                    Spacer()
                    Text(parameter.formattedValue)
                        .font(.system(.body, design: .rounded).weight(.bold))
                        .monospacedDigit()
                        .foregroundStyle(JagPalette.racingGreen)
                    Button {
                        model.toggleFavourite(stableKey: parameter.id)
                    } label: {
                        Image(systemName: parameter.favourite ? "star.fill" : "star")
                            .foregroundStyle(parameter.favourite ? JagPalette.warmMetal : JagPalette.chrome)
                    }
                    .buttonStyle(.plain)
                }
                .padding(.vertical, 3)
            }
        }
    }

    private var logPanel: some View {
        JagPanel(title: "Diagnostic log", systemImage: "doc.text.fill") {
            jagValueRow("Recorded samples", "\(model.recordedSampleCount)", icon: "waveform.path.ecg")
            jagDivider
            Button {
                model.prepareCSVExport()
            } label: {
                Label("Prepare diagnostic CSV", systemImage: "square.and.arrow.down")
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .fontWeight(.semibold)
            }
            if let url = model.csvExportURL {
                ShareLink(item: url) {
                    Label("Share diagnostic CSV", systemImage: "square.and.arrow.up")
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .fontWeight(.semibold)
                }
            }
        }
    }

    private var aboutPanel: some View {
        JagPanel(title: "About JAGLINK", systemImage: "info.circle.fill") {
            jagValueRow("Application", "JAGLINK", icon: "cat.fill")
            jagDivider
            jagValueRow("Version", model.versionText, icon: "tag")
            jagDivider
            jagValueRow("Licence", "GPL-3.0-or-later", icon: "doc.badge.gearshape")
            jagDivider
            jagValueRow("Author", "Shannon Smith", icon: "person.crop.circle")
            Text("Jaguar-specific module requests remain disabled until their protocol behaviour is source-corroborated or vehicle-verified.")
                .font(.caption)
                .foregroundStyle(JagPalette.charcoal.opacity(0.58))
                .padding(.top, 4)
        }
    }

    private var jagDivider: some View {
        Rectangle()
            .fill(JagPalette.chrome.opacity(0.45))
            .frame(height: 0.7)
    }

    private func jagValueRow(_ label: String, _ value: String, icon: String) -> some View {
        HStack(spacing: 10) {
            Image(systemName: icon)
                .frame(width: 20)
                .foregroundStyle(JagPalette.racingGreen)
            Text(label)
                .font(.subheadline)
                .foregroundStyle(JagPalette.charcoal.opacity(0.68))
            Spacer()
            Text(value)
                .font(.subheadline.weight(.semibold))
                .multilineTextAlignment(.trailing)
                .foregroundStyle(JagPalette.charcoal)
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
                if codes.isEmpty {
                    Text("None")
                        .font(.caption)
                        .foregroundStyle(JagPalette.charcoal.opacity(0.55))
                } else {
                    ForEach(codes, id: \.self) { code in
                        Text(code)
                            .font(.system(.body, design: .monospaced).weight(.semibold))
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
}
