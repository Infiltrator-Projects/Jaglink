<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK

JAGLINK is the Jaguar X-Type X400 product face built on the shared LINK vehicle-diagnostics engine.

**Current release: 0.2.5.**

## Dependency hierarchy

```text
Infiltratr Common
        ↓
       LINK
        ↓
     JAGLINK
 Jaguar/X400 face
```

JAGLINK pins LINK 0.6.0 at `src/link`. LINK in turn pins Infiltratr Common 1.10.0, so JAGLINK no longer carries a second top-level Common submodule. Native iPhone project references to Common resolve through LINK's nested dependency checkout.

LINK owns the shared workspace, ISO-TP, parameter/store/scheduler/telemetry runtime, Discover safety/evidence and the Windows OpenPort 2.0/J2534 scanner. JAGLINK retains only compatibility adaptors for those APIs plus Jaguar identity, Jaguar/X400 definitions and genuinely Jaguar-specific behaviour.

The next shared-code migrations are ELM327, standard OBD-II and UDS.

## X400 foundation

The current Jaguar layer represents the source-corroborated X400 network topology without inventing undocumented module addresses, PIDs or request formats:

- CAN — 500 kbit/s;
- SCP — 41.6 kbit/s;
- ISO 9141 diagnostic serial link — 10.4 kbit/s;
- D2B optical infotainment network — 5.6 Mbit/s.

Manufacturer-specific meanings remain evidence-gated until documentation or reproducible vehicle captures establish them.

## Build

```sh
git clone --recurse-submodules https://github.com/The-First-Infiltrator/Jaglink.git
cd Jaglink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### Linux

```sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DJAGLINK_BUILD_LINUX_APP=ON
cmake --build build-linux --target jaglink-linux
./build-linux/jaglink
```

The Linux shell is C/GTK4, uses the canonical Jaguar+OBD emblem in-app and in the desktop launcher, and includes the standard About dialog with version, authors, website and GPL information.

### Windows OpenPort/J2534 Discover

`jaglink-discover` is the Jaguar face of LINK's shared read-only scanner. It supports passive 500 kbit/s CAN capture, a bounded standard OBD inventory, JSON Lines evidence/annotations and deny-by-default blocking of unsafe or unknown diagnostic services.

### iPhone

The native project is `app/ios/JAGLINK.xcodeproj`. SwiftUI is confined to the native presentation edge; the portable diagnostic implementation remains C-first. The iPhone About experience follows the same structure as MBLINK while retaining Jaguar styling and the canonical Jaguar+OBD emblem.

On macOS, `bash ./scripts/build-ios-ipa.sh` builds the unsigned physical-device IPA and checksum.

## Release assets

A successful `main` release publishes:

- unsigned physical-device IPA and SHA-256 checksum;
- Linux `.deb` package;
- native self-compiling Linux `.run` package;
- branded Win32 `jaglink-discover.exe`;
- package checksums.

The release workflow fails, rather than reporting a false green, if the requested release tag already belongs to another commit.

## Engineering policy

- C is the default implementation language; C++ is used where it materially improves the design.
- Platform-required languages stay at narrow UI/interop boundaries.
- Shared automotive behaviour belongs in LINK; broadly reusable primitives belong in Infiltratr Common.
- Jaguar-only definitions and behaviour stay in JAGLINK.
- Public APIs and non-obvious implementation decisions require contract/rationale documentation without narrating obvious syntax.
- Unknown or unsafe diagnostic services are denied before transport transmission.

See `docs/JAGUAR.md`, `docs/APPLE.md`, `docs/DISCOVER.md`, `docs/ORIGIN.md` and `docs/ROADMAP.md`.

## Licence

Copyright © 2026 Xavier Wheaton and Shannon Smith.

JAGLINK is licensed under `GPL-3.0-or-later`.
