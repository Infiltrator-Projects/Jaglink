// SPDX-License-Identifier: GPL-3.0-or-later
import Foundation
import SwiftUI

private enum JAGLINKAboutStyle {
    static let cockpit = Color(red: 0.025, green: 0.055, blue: 0.045)
    static let deepGreen = Color(red: 0.020, green: 0.095, blue: 0.068)
    static let panel = Color(red: 0.035, green: 0.125, blue: 0.090)
    static let panelRaised = Color(red: 0.050, green: 0.165, blue: 0.115)
    static let ivory = Color(red: 0.955, green: 0.932, blue: 0.865)
    static let mutedIvory = Color(red: 0.790, green: 0.775, blue: 0.720)
    static let chrome = Color(red: 0.735, green: 0.755, blue: 0.745)
    static let warmMetal = Color(red: 0.735, green: 0.635, blue: 0.390)
}

private struct JAGLINKAboutBackground: View {
    var body: some View {
        LinearGradient(
            colors: [JAGLINKAboutStyle.cockpit, JAGLINKAboutStyle.deepGreen],
            startPoint: .top,
            endPoint: .bottomTrailing
        )
        .ignoresSafeArea()
    }
}

private struct JAGLINKAboutLogo: View {
    var size: CGFloat = 82

    var body: some View {
        Image("JAGLINKEmblem")
            .resizable()
            .scaledToFit()
            .frame(width: size, height: size)
            .shadow(color: .black.opacity(0.32), radius: 8, x: 0, y: 5)
            .accessibilityHidden(true)
    }
}

private struct JAGLINKAboutPanel<Content: View>: View {
    let content: Content

    init(@ViewBuilder content: () -> Content) {
        self.content = content()
    }

    var body: some View {
        content
            .padding(16)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(
                RoundedRectangle(cornerRadius: 18, style: .continuous)
                    .fill(
                        LinearGradient(
                            colors: [JAGLINKAboutStyle.panelRaised, JAGLINKAboutStyle.panel],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .overlay(
                        RoundedRectangle(cornerRadius: 18, style: .continuous)
                            .stroke(JAGLINKAboutStyle.warmMetal.opacity(0.42), lineWidth: 1)
                    )
            )
    }
}

private enum JAGLINKAboutDetail: String, Identifiable {
    case credits
    case license

    var id: String { rawValue }
}

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

@main
struct JAGLINKApp: App {
    @State private var showingAbout = false
    @AppStorage("jaglink.language") private var language = "en-AU"

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environment(\.locale, Locale(identifier: JagInterfaceLanguage.canonical(language)))
                .environment(\.layoutDirection, JagInterfaceLanguage.canonical(language).hasPrefix("ar") ? .rightToLeft : .leftToRight)
                .onAppear {
                    let canonical = JagInterfaceLanguage.canonical(language)
                    if language != canonical { language = canonical }
                }
                .preferredColorScheme(.dark)
                .tint(JAGLINKAboutStyle.warmMetal)
                .safeAreaInset(edge: .bottom, spacing: 0) {
                    Button {
                        showingAbout = true
                    } label: {
                        HStack(spacing: 8) {
                            Text("JAGLINK")
                                .fontWeight(.bold)
                                .tracking(1.0)
                            Text("© 2026 Xavier Wheaton & Shannon Smith")
                                .foregroundStyle(JAGLINKAboutStyle.chrome)
                                .lineLimit(1)
                                .minimumScaleFactor(0.70)
                            Spacer(minLength: 8)
                            Label("About", systemImage: "info.circle")
                        }
                        .font(.caption)
                        .foregroundStyle(JAGLINKAboutStyle.ivory)
                        .padding(.horizontal, 16)
                        .padding(.vertical, 9)
                        .frame(maxWidth: .infinity)
                        .background(JAGLINKAboutStyle.cockpit)
                        .overlay(alignment: .top) {
                            Rectangle()
                                .fill(JAGLINKAboutStyle.warmMetal.opacity(0.42))
                                .frame(height: 1)
                        }
                    }
                    .buttonStyle(.plain)
                }
                .sheet(isPresented: $showingAbout) {
                    JAGLINKAboutView {
                        showingAbout = false
                    }
                    .preferredColorScheme(.dark)
                    .tint(JAGLINKAboutStyle.warmMetal)
                }
        }
    }
}

private struct JAGLINKAboutView: View {
    let onClose: () -> Void
    @State private var detail: JAGLINKAboutDetail?

    private var version: String {
        Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "Unknown"
    }

    var body: some View {
        ZStack {
            JAGLINKAboutBackground()

            VStack(spacing: 0) {
                ScrollView {
                    VStack(spacing: 17) {
                        JAGLINKAboutLogo()
                            .padding(.top, 30)

                        VStack(spacing: 4) {
                            Text("JAGLINK")
                                .font(.system(size: 34, weight: .black, design: .rounded))
                                .tracking(2.0)
                                .foregroundStyle(JAGLINKAboutStyle.ivory)
                            Text("X400 · JAGUAR DIAGNOSTICS")
                                .font(.caption2.weight(.bold))
                                .tracking(1.7)
                                .foregroundStyle(JAGLINKAboutStyle.warmMetal)
                        }

                        Text("Version \(version)")
                            .font(.subheadline.monospaced())
                            .foregroundStyle(JAGLINKAboutStyle.chrome)

                        Text("A C-first, open-source Jaguar X-Type X400 diagnostics platform authored by Xavier Wheaton and Shannon Smith.")
                            .font(.body)
                            .multilineTextAlignment(.center)
                            .foregroundStyle(JAGLINKAboutStyle.ivory)
                            .padding(.horizontal, 28)

                        Text("Copyright © 2026 Xavier Wheaton and Shannon Smith")
                            .font(.subheadline)
                            .multilineTextAlignment(.center)
                            .foregroundStyle(JAGLINKAboutStyle.chrome)

                        Link(
                            "Project Website",
                            destination: URL(string: "https://github.com/The-First-Infiltrator/Jaglink")!
                        )
                        .font(.body.weight(.semibold))
                        .foregroundStyle(JAGLINKAboutStyle.warmMetal)
                    }
                    .frame(maxWidth: .infinity)
                }

                HStack(spacing: 10) {
                    Button("Credits") {
                        detail = .credits
                    }
                    .buttonStyle(.bordered)

                    Button("License") {
                        detail = .license
                    }
                    .buttonStyle(.bordered)

                    Button("Close") {
                        onClose()
                    }
                    .buttonStyle(.borderedProminent)
                    .tint(JAGLINKAboutStyle.warmMetal)
                    .foregroundStyle(JAGLINKAboutStyle.deepGreen)
                }
                .frame(maxWidth: .infinity)
                .padding(.horizontal, 16)
                .padding(.vertical, 12)
                .background(JAGLINKAboutStyle.cockpit)
                .overlay(alignment: .top) {
                    Rectangle()
                        .fill(JAGLINKAboutStyle.warmMetal.opacity(0.42))
                        .frame(height: 1)
                }
            }
        }
        .presentationDetents([.medium, .large])
        .presentationDragIndicator(.visible)
        .sheet(item: $detail) { item in
            switch item {
            case .credits:
                NavigationStack {
                    ZStack {
                        JAGLINKAboutBackground()
                        JAGLINKAboutPanel {
                            VStack(alignment: .leading, spacing: 10) {
                                Text("Authors")
                                    .font(.headline)
                                    .foregroundStyle(JAGLINKAboutStyle.ivory)
                                Text("Xavier Wheaton")
                                    .foregroundStyle(JAGLINKAboutStyle.ivory)
                                Text("Shannon Smith")
                                    .foregroundStyle(JAGLINKAboutStyle.ivory)
                            }
                        }
                        .padding(16)
                    }
                    .navigationTitle("Credits")
                    .navigationBarTitleDisplayMode(.inline)
                    .toolbarBackground(JAGLINKAboutStyle.cockpit, for: .navigationBar)
                    .toolbarBackground(.visible, for: .navigationBar)
                    .toolbar {
                        ToolbarItem(placement: .confirmationAction) {
                            Button("Close") { detail = nil }
                        }
                    }
                }
            case .license:
                NavigationStack {
                    ZStack {
                        JAGLINKAboutBackground()
                        ScrollView {
                            JAGLINKAboutPanel {
                                Text(
                                    "JAGLINK is free software licensed under the GNU General Public License version 3 or, at your option, any later version (GPL-3.0-or-later).\n\nSee LICENSE in the source package for the complete licence text."
                                )
                                .foregroundStyle(JAGLINKAboutStyle.ivory)
                            }
                            .padding(16)
                        }
                    }
                    .navigationTitle("License")
                    .navigationBarTitleDisplayMode(.inline)
                    .toolbarBackground(JAGLINKAboutStyle.cockpit, for: .navigationBar)
                    .toolbarBackground(.visible, for: .navigationBar)
                    .toolbar {
                        ToolbarItem(placement: .confirmationAction) {
                            Button("Close") { detail = nil }
                        }
                    }
                }
            }
        }
    }
}
