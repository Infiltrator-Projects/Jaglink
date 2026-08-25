from pathlib import Path

path = Path("app/ios/JAGLINK/ConnectionViewModel.swift")
text = path.read_text()

class_marker = "@MainActor\nfinal class ConnectionViewModel: NSObject, ObservableObject, @preconcurrency JagLinkDiagnosticsControllerDelegate {"
helper = '''private func jaglinkLocalized(_ key: String) -> String {
    let language = UserDefaults.standard.string(forKey: "jaglink.language") ?? "en"
    guard let path = Bundle.main.path(forResource: language, ofType: "lproj"),
          let bundle = Bundle(path: path) else {
        return key
    }
    return bundle.localizedString(forKey: key, value: key, table: nil)
}

@MainActor
final class ConnectionViewModel: NSObject, ObservableObject, @preconcurrency JagLinkDiagnosticsControllerDelegate {'''
if text.count(class_marker) != 1:
    raise SystemExit("ConnectionViewModel class marker changed")
text = text.replace(class_marker, helper, 1)

replacements = [
    ('title: "Connection Test"', 'title: jaglinkLocalized("Connection Test")'),
    ('message: "Real Adapter uses Bluetooth. Simulated ELM327 runs the same diagnostic stack against an in-process ELM327 byte-stream emulator."', 'message: jaglinkLocalized("Real Adapter uses Bluetooth. Simulated ELM327 runs the same diagnostic stack against an in-process ELM327 byte-stream emulator.")'),
    ('UIAlertAction(title: "Real Adapter", style: .default)', 'UIAlertAction(title: jaglinkLocalized("Real Adapter"), style: .default)'),
    ('UIAlertAction(title: "Simulated ELM327", style: .default)', 'UIAlertAction(title: jaglinkLocalized("Simulated ELM327"), style: .default)'),
    ('UIAlertAction(title: "Cancel", style: .cancel)', 'UIAlertAction(title: jaglinkLocalized("Cancel"), style: .cancel)'),
]
for old, new in replacements:
    if text.count(old) != 1:
        raise SystemExit(f"alert marker changed: {old}")
    text = text.replace(old, new, 1)

path.write_text(text)
