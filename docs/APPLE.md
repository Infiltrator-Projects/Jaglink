<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Apple / iPhone target

The JAGLINK iPhone project is `app/ios/JAGLINK.xcodeproj`.

JAGLINK owns its `JagLinkBLETransport` CoreBluetooth byte-stream provider and `JagLinkDiagnosticsController`. Together they perform adapter/GATT discovery, ELM327 initialisation, standard OBD-II capability discovery, generic stored/pending/permanent DTC reads and generic live-data scheduling. The target contains no external vehicle-diagnostics source dependency.

The SwiftUI app exposes the X400 network profile, generic faults, live OBD-II parameters, favourites and diagnostic CSV export. Jaguar manufacturer-specific module requests remain disabled until they are supported by reproducible evidence.

Build an unsigned simulator target with:

```sh
xcodebuild -project app/ios/JAGLINK.xcodeproj \
  -scheme JAGLINK \
  -configuration Debug \
  -sdk iphonesimulator \
  -destination 'generic/platform=iOS Simulator' \
  CODE_SIGNING_ALLOWED=NO build
```

For an unsigned physical-device IPA on macOS, use the repository script:

```sh
bash ./scripts/build-ios-ipa.sh
```

It builds the `iphoneos` target for generic physical iOS hardware with code signing disabled, confirms the app executable contains arm64, packages `Payload/JAGLINK.app`, validates the ZIP/IPA, and writes both the IPA and a SHA-256 checksum under `dist/` by default.

The manually runnable GitHub Actions workflow **Build unsigned iPhone IPA** performs the same physical-device build and uploads exactly `JAGLINK-unsigned.ipa` plus `JAGLINK-unsigned.ipa.sha256` as a workflow artifact.

A recursive Git checkout is required so the directly pinned `src/infiltratr-common` shared-library submodule is present.
