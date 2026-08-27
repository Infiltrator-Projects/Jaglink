<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Apple / iPhone target

The JAGLINK iPhone project is `app/ios/JAGLINK.xcodeproj`.

## Ownership boundary

LINK owns the shared CoreBluetooth byte-stream provider and the product-neutral diagnostic-flow controller. JAGLINK's `JagLinkBLETransport` source is a compatibility compilation shim over LINK's `LinkBLETransport`; it is not a second Bluetooth implementation. `JagLinkDiagnosticsController` owns the Jaguar/X400 presentation edge and VIN-derived Jaguar identity while delegating ELM327 framing, standard OBD-II capability discovery, stored/pending/permanent DTC reads and live-data scheduling to LINK.

The SwiftUI application exposes the X400 network profile, generic faults, live OBD-II parameters, favourites and diagnostic CSV export. Jaguar manufacturer-specific module requests remain evidence-gated and are not enabled merely to match Mercedes feature depth.

## Build

Build an unsigned simulator target with:

```sh
xcodebuild -project app/ios/JAGLINK.xcodeproj \
  -scheme JAGLINK \
  -configuration Debug \
  -sdk iphonesimulator \
  -destination 'generic/platform=iOS Simulator' \
  CODE_SIGNING_ALLOWED=NO build
```

For an unsigned physical-device IPA on macOS, use:

```sh
bash ./scripts/build-ios-ipa.sh
```

The main JAGLINK CI workflow builds both Debug and Release simulator configurations and also builds the unsigned physical-device IPA used by numbered releases. There is no separate release-capable IPA workflow to drift from the main quality gate.

A recursive Git checkout is required so the exact `src/link` gitlink and LINK's nested Infiltratr Common dependency are present.
