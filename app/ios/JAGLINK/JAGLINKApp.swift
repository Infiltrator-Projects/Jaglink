// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import SwiftUI

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
