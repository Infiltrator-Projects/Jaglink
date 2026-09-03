// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import SwiftUI

struct JagInterfaceLanguage: Identifiable, Hashable {
    let id: String
    let nativeName: String

    static let all: [JagInterfaceLanguage] = {
        let count = Int(link_i18n_supported_locale_count())
        return (0..<count).compactMap { index in
            guard let locale = link_i18n_supported_locale(index),
                  let name = link_i18n_supported_locale_name(index) else { return nil }
            return JagInterfaceLanguage(id: String(cString: locale), nativeName: String(cString: name))
        }
    }()

    static func canonical(_ stored: String) -> String {
        switch stored {
        case "en": return "en-AU"
        case "de": return "de-DE"
        case "pl": return "pl-PL"
        default: return all.contains(where: { $0.id == stored }) ? stored : "en-AU"
        }
    }

    static func displayName(for stored: String) -> String {
        let code = canonical(stored)
        return all.first(where: { $0.id == code })?.nativeName ?? "English (Australia)"
    }
}

private var jaglinkAboutInfo: LinkDiagnosticAboutInfo {
    LinkDiagnosticAboutInfo(
        productName: "JAGLINK",
        subtitle: "JAGUAR · LINK DIAGNOSTICS",
        version: Bundle.main.object(
            forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "Unknown",
        summary: "A C-first, open-source Jaguar diagnostics platform authored by Xavier Wheaton and Shannon Smith.",
        authors: ["Xavier Wheaton", "Shannon Smith"],
        copyright: "Copyright © 2026 Xavier Wheaton and Shannon Smith",
        website: URL(string: "https://github.com/Infiltrator-Projects/Jaglink"),
        licenseName: "GPL-3.0-or-later",
        licenseText: "JAGLINK is free software licensed under the GNU General Public License version 3 or, at your option, any later version (GPL-3.0-or-later).\n\nSee LICENSE in the source package for the complete licence text.",
        credits: [
            "Xavier Wheaton — Author and project contributor",
            "Shannon Smith — Author and project maintainer"
        ])
}

@main
struct JAGLINKApp: App {
    @State private var showingAbout = false

    var body: some Scene {
        WindowGroup {
            ContentView()
                .linkDiagnosticTheme(jagLinkTheme)
                .preferredColorScheme(.dark)
                .tint(jagLinkTheme.accent)
                .safeAreaInset(edge: .bottom, spacing: 0) {
                    LinkDiagnosticAboutButton(
                        productName: "JAGLINK",
                        copyright: "© 2026 Xavier Wheaton & Shannon Smith") {
                            showingAbout = true
                        }
                        .linkDiagnosticTheme(jagLinkTheme)
                }
                .sheet(isPresented: $showingAbout) {
                    LinkDiagnosticAboutView(
                        info: jaglinkAboutInfo,
                        onClose: { showingAbout = false }) {
                            Image("JAGLINKEmblem")
                                .resizable()
                                .scaledToFit()
                                .frame(width: 82, height: 82)
                                .shadow(
                                    color: .black.opacity(0.32),
                                    radius: 8, x: 0, y: 5)
                                .accessibilityHidden(true)
                        }
                        .linkDiagnosticTheme(jagLinkTheme)
                        .preferredColorScheme(.dark)
                        .tint(jagLinkTheme.accent)
                }
        }
    }
}
