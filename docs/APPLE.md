<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Apple / iPhone target

The JAGLINK iPhone project is `app/ios/JAGLINK.xcodeproj`.

It reuses MBLINK 0.7.12's CoreBluetooth byte-stream provider because that provider contains adapter/GATT behaviour rather than Mercedes diagnostics. JAGLINK supplies its own `JagLinkDiagnosticsController`, which performs ELM327 initialisation, standard OBD-II capability discovery, generic stored/pending/permanent DTC reads and generic live-data scheduling. It does not compile or call the MBLINK Mercedes profile/probe sources.

The SwiftUI app exposes the X400 network profile, generic faults, live OBD-II parameters, favourites and diagnostic CSV export. Jaguar manufacturer-specific module requests remain disabled in 0.1.0.

Build an unsigned simulator target with:

```sh
xcodebuild -project app/ios/JAGLINK.xcodeproj \
  -scheme JAGLINK \
  -configuration Debug \
  -sdk iphonesimulator \
  -destination 'generic/platform=iOS Simulator' \
  CODE_SIGNING_ALLOWED=NO build
```

A recursive Git checkout is required so `upstream/mblink` and its pinned Infiltratr Common submodule are present.
