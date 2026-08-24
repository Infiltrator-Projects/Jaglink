from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one occurrence, found {count}")
    return text.replace(old, new, 1)


app_path = Path("app/ios/JAGLINK/JAGLINKApp.swift")
a = app_path.read_text()
a = replace_once(
    a,
    'struct JAGLINKApp: App {\n    @State private var showingAbout = false\n',
    'struct JAGLINKApp: App {\n    @State private var showingAbout = false\n    @AppStorage("jaglink.language") private var language = "en"\n',
    "app language storage",
)
a = replace_once(
    a,
    '            ContentView()\n                .preferredColorScheme(.dark)\n',
    '            ContentView()\n                .environment(\\.locale, Locale(identifier: language))\n                .preferredColorScheme(.dark)\n',
    "root locale",
)
app_path.write_text(a)

view_path = Path("app/ios/JAGLINK/ContentView.swift")
s = view_path.read_text()
settings_header = 'private struct JagSettingsView: View {\n    @ObservedObject var model: ConnectionViewModel\n'
s = replace_once(
    s,
    settings_header,
    settings_header + '    @AppStorage("jaglink.language") private var language = "en"\n',
    "settings language storage",
)
application_marker = '                JagPanel(title: "Application", systemImage: "gearshape.fill") {\n'
if application_marker not in s:
    raise SystemExit("Jag settings Application marker missing")
language_panel = '''                JagPanel(title: "Language", systemImage: "globe") {
                    Picker("Language", selection: $language) {
                        Text("English").tag("en")
                        Text("Deutsch").tag("de")
                        Text("Polski").tag("pl")
                    }
                    .pickerStyle(.segmented)
                }

'''
s = s.replace(application_marker, language_panel + application_marker, 1)
replacements = {
    'Text(text.uppercased())': 'Text(LocalizedStringKey(text)).textCase(.uppercase)',
    'Label(title, systemImage: systemImage)': 'Label(LocalizedStringKey(title), systemImage: systemImage)',
    'Text(title)': 'Text(LocalizedStringKey(title))',
    'Text(subtitle)': 'Text(LocalizedStringKey(subtitle))',
    'Text(label)': 'Text(LocalizedStringKey(label))',
    'Text(value)': 'Text(LocalizedStringKey(value))',
    'Text(parameter.title)': 'Text(LocalizedStringKey(parameter.title))',
    'Text(network.status.uppercased())': 'Text(LocalizedStringKey(network.status)).textCase(.uppercase)',
    'Text(model.faultScanStatusText)': 'Text(LocalizedStringKey(model.faultScanStatusText))',
    '.navigationTitle(title)': '.navigationTitle(LocalizedStringKey(title))',
}
for old, new in replacements.items():
    s = s.replace(old, new)
view_path.write_text(s)

pbx_path = Path("app/ios/JAGLINK.xcodeproj/project.pbxproj")
p = pbx_path.read_text()
if "D2A900000000000000000001 /* Localizable.strings in Resources */" not in p:
    p = replace_once(
        p,
        '\t\tA00000000000000000000032 /* JAGLINK/Assets.xcassets in Resources */ = {isa = PBXBuildFile; fileRef = B00000000000000000000032 /* JAGLINK/Assets.xcassets */; };',
        '\t\tA00000000000000000000032 /* JAGLINK/Assets.xcassets in Resources */ = {isa = PBXBuildFile; fileRef = B00000000000000000000032 /* JAGLINK/Assets.xcassets */; };\n\t\tD2A900000000000000000001 /* Localizable.strings in Resources */ = {isa = PBXBuildFile; fileRef = D2A900000000000000000005 /* Localizable.strings */; };\n\t\tD2A900000000000000000006 /* link_i18n.c in Sources */ = {isa = PBXBuildFile; fileRef = D2A900000000000000000008 /* link_i18n.c */; };\n\t\tD2A900000000000000000007 /* link_i18n_platform.c in Sources */ = {isa = PBXBuildFile; fileRef = D2A900000000000000000009 /* link_i18n_platform.c */; };',
        "PBX build files",
    )
    p = replace_once(
        p,
        '\t\tB00000000000000000000032 /* JAGLINK/Assets.xcassets */ = {isa = PBXFileReference; lastKnownFileType = folder.assetcatalog; path = JAGLINK/Assets.xcassets; sourceTree = SOURCE_ROOT; };',
        '\t\tB00000000000000000000032 /* JAGLINK/Assets.xcassets */ = {isa = PBXFileReference; lastKnownFileType = folder.assetcatalog; path = JAGLINK/Assets.xcassets; sourceTree = SOURCE_ROOT; };\n\t\tD2A900000000000000000002 /* en */ = {isa = PBXFileReference; lastKnownFileType = text.plist.strings; name = en; path = "JAGLINK/en.lproj/Localizable.strings"; sourceTree = SOURCE_ROOT; };\n\t\tD2A900000000000000000003 /* de */ = {isa = PBXFileReference; lastKnownFileType = text.plist.strings; name = de; path = "JAGLINK/de.lproj/Localizable.strings"; sourceTree = SOURCE_ROOT; };\n\t\tD2A900000000000000000004 /* pl */ = {isa = PBXFileReference; lastKnownFileType = text.plist.strings; name = pl; path = "JAGLINK/pl.lproj/Localizable.strings"; sourceTree = SOURCE_ROOT; };\n\t\tD2A900000000000000000008 /* link_i18n.c */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.c; name = link_i18n.c; path = "../../src/link/src/core/i18n.c"; sourceTree = SOURCE_ROOT; };\n\t\tD2A900000000000000000009 /* link_i18n_platform.c */ = {isa = PBXFileReference; lastKnownFileType = sourcecode.c.c; name = link_i18n_platform.c; path = "../../src/link/src/core/i18n_platform.c"; sourceTree = SOURCE_ROOT; };',
        "PBX file refs",
    )
    p = replace_once(
        p,
        '\t\t\t\tB00000000000000000000032 /* JAGLINK/Assets.xcassets */,\n',
        '\t\t\t\tB00000000000000000000032 /* JAGLINK/Assets.xcassets */,\n\t\t\t\tD2A900000000000000000005 /* Localizable.strings */,\n',
        "PBX app group",
    )
    p = replace_once(
        p,
        '/* End PBXGroup section */',
        '''/* End PBXGroup section */

/* Begin PBXVariantGroup section */
\t\tD2A900000000000000000005 /* Localizable.strings */ = {
\t\t\tisa = PBXVariantGroup;
\t\t\tchildren = (
\t\t\t\tD2A900000000000000000002 /* en */,
\t\t\t\tD2A900000000000000000003 /* de */,
\t\t\t\tD2A900000000000000000004 /* pl */,
\t\t\t);
\t\t\tname = Localizable.strings;
\t\t\tsourceTree = "<group>";
\t\t};
/* End PBXVariantGroup section */''',
        "PBX variant section",
    )
    p = replace_once(
        p,
        '\t\t\tknownRegions = (\n\t\t\t\ten,\n\t\t\t\tBase,\n\t\t\t);',
        '\t\t\tknownRegions = (\n\t\t\t\ten,\n\t\t\t\tde,\n\t\t\t\tpl,\n\t\t\t\tBase,\n\t\t\t);',
        "PBX known regions",
    )
    p = replace_once(
        p,
        '\t\t\t\tA00000000000000000000032 /* JAGLINK/Assets.xcassets in Resources */,\n',
        '\t\t\t\tA00000000000000000000032 /* JAGLINK/Assets.xcassets in Resources */,\n\t\t\t\tD2A900000000000000000001 /* Localizable.strings in Resources */,\n',
        "PBX resources",
    )
    p = replace_once(
        p,
        '\t\t\t\tA00000000000000000000022 /* ../../src/obd2/obd2.c in Sources */,\n',
        '\t\t\t\tA00000000000000000000022 /* ../../src/obd2/obd2.c in Sources */,\n\t\t\t\tD2A900000000000000000006 /* link_i18n.c in Sources */,\n\t\t\t\tD2A900000000000000000007 /* link_i18n_platform.c in Sources */,\n',
        "PBX sources",
    )
pbx_path.write_text(p)
