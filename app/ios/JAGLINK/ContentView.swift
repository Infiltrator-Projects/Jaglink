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

private struct JaguarBadge: View {
    var size: CGFloat = 62

    var body: some View {
        ZStack {
            RoundedRectangle(cornerRadius: size * 0.24, style: .continuous)
                .fill(
                    LinearGradient(
                        colors: [JagPalette.panelRaised, JagPalette.deepGreen],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .overlay(
                    RoundedRectangle(cornerRadius: size * 0.24, style: .continuous)
                        .stroke(JagPalette.warmMetal.opacity(0.95), lineWidth: 1.6)
                )

            VStack(spacing: -1) {
                Image(systemName: "cat.fill")
                    .font(.system(size: size * 0.42, weight: .semibold))
                    .foregroundStyle(JagPalette.ivory)
                HStack(spacing: size * 0.07) {
                    Circle().fill(JagPalette.warmMetal)
                    Circle().fill(JagPalette.chrome)
                    Circle().fill(JagPalette.warmMetal)
                }
                .frame(width: size * 0.40, height: size * 0.07)
            }
        }
        .frame(width: size, height: size)
        .shadow(color: .black.opacity(0.32), radius: 8, x: 0, y: 5)
        .accessibilityHidden(true)
    }
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
            HStack(spacing: 9) {
                Image(systemName: systemImage)
                    .font(.system(size: 12, weight: .bold))
                    .frame(width: 25, height: 25)
                    .foregroundStyle(JagPalette.deepGreen)
                    .background(JagPalette.warmMetal)
                    .clipShape(RoundedRectangle(cornerRadius: 7, style: .continuous))

                Text(title.uppercased())
                    .font(.system(size: 12, weight: .bold, design: .rounded))
                    .tracking(1.8)
                    .foregroundStyle(JagPalette.ivory)
            }

            content
        }
        .padding(17)
        .background(
            RoundedRectangle(cornerRadius: 19, style: .continuous)
                .fill(
                    LinearGradient(
                        colors: [JagPalette.panelRaised.opacity(0.92), JagPalette.panel.opacity(0.96)],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .shadow(color: .black.opacity(0.28), radius: 12, x: 0, y: 7)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 19, style: .continuous)
                .stroke(JagPalette.warmMetal.opacity(0.36), lineWidth: 0.8)
        )
    }
}

private struct JagWordmark: View {
    var body: some View {
        HStack(spacing: 15) {
            JaguarBadge()

            VStack(alignment: .leading, spacing: 3) {
                Text("JAGLINK")
                    .font(.system(size: 29, weight: .semibold, design: .serif))
                    .tracking(4.6)
                    .foregroundStyle(JagPalette.ivory)
                Text("X400  ·  JAGUAR DIAGNOSTICS")
                    .font(.system(size: 10, weight: .semibold, design: .rounded))
                    .tracking(1.8)
                    .foregroundStyle(JagPalette.warmMetal)
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
                    colors: [JagPalette.cockpit, JagPalette.deepGreen],
                    startPoint: .top,
                    endPoint: .bottomTrailing
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
                    .padding(.bottom, 30)
                }
            }
            .navigationBarTitleDisplayMode(.inline)
            .toolbarBackground(JagPalette.cockpit, for: .navigationBar)
            .toolbarBackground(.visible, for: .navigationBar)
            .toolbarColorScheme(.dark, for: .navigationBar)
            .toolbar {
                ToolbarItem(placement: .principal) {
                    HStack(spacing: 7) {
                        Image(systemName: "cat.fill")
                            .foregroundStyle(JagPalette.warmMetal)
                        Text("JAGLINK")
                            .font(.system(size: 15, weight: .semibold, design: .serif))
                            .tracking(3)
                    }
                }
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        model.isActive ? model.disconnect() : model.connect()
                    } label: {
                        Image(systemName: model.isActive ? "cable.connector.slash" : "cable.connector")
                            .font(.system(size: 15, weight: .bold))
                            .foregroundStyle(model.isActive ? JagPalette.jaguarRed : JagPalette.warmMetal)
                    }
                    .accessibilityLabel(model.isActive ? "Disconnect" : "Connect")
                }
            }
            .tint(JagPalette.warmMetal)
        }
    }

    private var hero: some View {
        VStack(alignment: .leading, spacing: 17) {
            JagWordmark()

            Text("A Jaguar-focused diagnostic cockpit for the X-Type X400 platform.")
                .font(.subheadline)
                .foregroundStyle(JagPalette.mutedIvory)

            HStack(spacing: 10) {
                statusPill(
                    title: model.isActive ? "CONNECTED" : "STANDBY",
                    icon: model.isActive ? "link" : "power",
                    colour: model.isActive ? JagPalette.racingGreen : JagPalette.chrome
                )
                statusPill(
                    title: "X400",
                    icon: "car.side.fill",
                    colour: JagPalette.warmMetal
                )
            }

            Button {
                model.isActive ? model.disconnect() : model.connect()
            } label: {
                HStack {
                    Image(systemName: model.isActive ? "bolt.slash.fill" : "bolt.fill")
                    Text(model.isActive ? "Disconnect vehicle" : "Connect to X400")
                        .fontWeight(.semibold)
                    Spacer()
                    Image(systemName: "chevron.right")
                        .font(.caption.bold())
                }
                .padding(.horizontal, 16)
                .frame(height: 50)
                .foregroundStyle(JagPalette.deepGreen)
                .background(
                    LinearGradient(
                        colors: [JagPalette.ivory, JagPalette.warmMetal.opacity(0.92)],
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                )
                .clipShape(RoundedRectangle(cornerRadius: 14, style: .continuous))
            }
            .buttonStyle(.plain)
        }
        .padding(19)
        .background(
            LinearGradient(
                colors: [JagPalette.panelRaised, JagPalette.deepGreen],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
        )
        .clipShape(RoundedRectangle(cornerRadius: 25, style: .continuous))
        .overlay(
            RoundedRectangle(cornerRadius: 25, style: .continuous)
                .stroke(JagPalette.warmMetal.opacity(0.62), lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.35), radius: 14, x: 0, y: 8)
        .padding(.top, 12)
    }

    private func statusPill(title: String, icon: String, colour: Color) -> some View {
        Label(title, systemImage: icon)
            .font(.system(size: 10, weight: .bold, design: .rounded))
            .lineLimit(1)
            .padding(.horizontal, 10)
            .padding(.vertical, 7)
            .foregroundStyle(JagPalette.ivory)
            .background(colour.opacity(0.16))
            .overlay(Capsule().stroke(colour.opacity(0.70), lineWidth: 0.8))
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
        JagPanel(title: "X400 Networks", systemImage: "point.3.connected.trianglepath.dotted") {
            ForEach(model.jaguarNetworks) { network in
                HStack(alignment: .top, spacing: 12) {
                    ZStack {
                        RoundedRectangle(cornerRadius: 10, style: .continuous)
                            .fill(JagPalette.cockpit.opacity(0.85))
                            .overlay(
                                RoundedRectangle(cornerRadius: 10, style: .continuous)
                                    .stroke(JagPalette.warmMetal.opacity(0.45), lineWidth: 0.8)
                            )
                        Image(systemName: networkIcon(network.kind))
                            .foregroundStyle(JagPalette.warmMetal)
                    }
                    .frame(width: 40, height: 40)

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
                        Text("\(network.kind.uppercased())  ·  \(network.role)")
                            .font(.subheadline)
                            .foregroundStyle(JagPalette.mutedIvory)
                        Text(rateText(network.nominalBaud))
                            .font(.system(.caption, design: .monospaced).weight(.semibold))
                            .foregroundStyle(JagPalette.warmMetal)
                        Text(network.provenance)
                            .font(.caption)
                            .foregroundStyle(JagPalette.chrome.opacity(0.72))
                    }
                }
                .padding(.vertical, 3)

                if network.id != model.jaguarNetworks.last?.id { jagDivider }
            }
        }
    }

    private var faultPanel: some View {
        JagPanel(title: "Fault Memory", systemImage: "exclamationmark.triangle.fill") {
            Text(model.faultScanStatusText)
                .font(.subheadline)
                .foregroundStyle(JagPalette.mutedIvory)
            faultRows(title: "Stored", codes: model.storedDTCs)
            faultRows(title: "Pending", codes: model.pendingDTCs)
            faultRows(title: "Permanent", codes: model.permanentDTCs)
        }
    }

    private var liveDataPanel: some View {
        JagPanel(title: "Live Data", systemImage: "gauge.with.dots.needle.67percent") {
            if model.diagnosticParameters.isEmpty {
                Text("Connect to the vehicle to populate live parameters.")
                    .font(.subheadline)
                    .foregroundStyle(JagPalette.mutedIvory)
            }

            ForEach(model.diagnosticParameters) { parameter in
                HStack(spacing: 12) {
                    VStack(alignment: .leading, spacing: 2) {
                        Text(parameter.title)
                            .font(.subheadline.weight(.semibold))
                            .foregroundStyle(JagPalette.ivory)
                        Text(parameter.id)
                            .font(.caption2.monospaced())
                            .foregroundStyle(JagPalette.chrome.opacity(0.64))
                    }
                    Spacer()
                    Text(parameter.formattedValue)
                        .font(.system(.body, design: .rounded).weight(.bold))
                        .monospacedDigit()
                        .foregroundStyle(JagPalette.warmMetal)
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
        JagPanel(title: "Diagnostic Log", systemImage: "doc.text.fill") {
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
                .foregroundStyle(JagPalette.chrome.opacity(0.72))
                .padding(.top, 4)
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
