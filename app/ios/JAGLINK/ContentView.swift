// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import SwiftUI

struct ContentView: View {
    @StateObject private var model = ConnectionViewModel()

    var body: some View {
        NavigationStack {
            List {
                Section("Vehicle") {
                    LabeledContent("Profile", value: model.profileDisplayName)
                    LabeledContent("Adapter", value: model.peripheralName)
                    LabeledContent("ELM identity", value: model.adapterIdentifier)
                    LabeledContent("Status", value: model.statusText)
                }

                Section("X400 networks") {
                    ForEach(model.jaguarNetworks) { network in
                        VStack(alignment: .leading, spacing: 4) {
                            HStack {
                                Text(network.name).font(.headline)
                                Spacer()
                                Text(network.status).font(.caption).foregroundStyle(.secondary)
                            }
                            Text("\(network.kind.uppercased()) · \(network.role) · \(rateText(network.nominalBaud))")
                                .font(.subheadline)
                            Text(network.provenance)
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        .padding(.vertical, 2)
                    }
                }

                Section("Faults") {
                    LabeledContent("Scan", value: model.faultScanStatusText)
                    faultRows(title: "Stored", codes: model.storedDTCs)
                    faultRows(title: "Pending", codes: model.pendingDTCs)
                    faultRows(title: "Permanent", codes: model.permanentDTCs)
                }

                Section("Live data") {
                    ForEach(model.diagnosticParameters) { parameter in
                        HStack {
                            VStack(alignment: .leading) {
                                Text(parameter.title)
                                Text(parameter.id).font(.caption).foregroundStyle(.secondary)
                            }
                            Spacer()
                            Text(parameter.formattedValue).monospacedDigit()
                            Button {
                                model.toggleFavourite(stableKey: parameter.id)
                            } label: {
                                Image(systemName: parameter.favourite ? "star.fill" : "star")
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }

                Section("Log") {
                    LabeledContent("Samples", value: "\(model.recordedSampleCount)")
                    Button("Prepare diagnostic CSV") { model.prepareCSVExport() }
                    if let url = model.csvExportURL {
                        ShareLink(item: url) { Label("Share diagnostic CSV", systemImage: "square.and.arrow.up") }
                    }
                }

                Section("About") {
                    LabeledContent("Application", value: "JAGLINK")
                    LabeledContent("Version", value: model.versionText)
                    LabeledContent("Licence", value: "GPL-3.0-or-later")
                    LabeledContent("Author", value: "Shannon Smith")
                    Text("Jaguar-specific module requests remain disabled until their protocol behaviour is source-corroborated or vehicle-verified.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }
            .navigationTitle("JAGLINK")
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button(model.isActive ? "Disconnect" : "Connect") {
                        model.isActive ? model.disconnect() : model.connect()
                    }
                }
            }
        }
    }

    @ViewBuilder
    private func faultRows(title: String, codes: [String]) -> some View {
        if codes.isEmpty {
            LabeledContent(title, value: "None")
        } else {
            VStack(alignment: .leading, spacing: 4) {
                Text(title).font(.headline)
                ForEach(codes, id: \.self) { code in
                    Text(code).font(.system(.body, design: .monospaced))
                }
            }
        }
    }

    private func rateText(_ baud: UInt32) -> String {
        if baud >= 1_000_000 { return String(format: "%.1f Mbit/s", Double(baud) / 1_000_000.0) }
        if baud >= 1_000 { return String(format: "%.1f kbit/s", Double(baud) / 1_000.0) }
        return "\(baud) bit/s"
    }
}
