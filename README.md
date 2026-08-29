<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK

[![JAGLINK CI](https://github.com/Infiltrator-Projects/Jaglink/actions/workflows/ci.yml/badge.svg)](https://github.com/Infiltrator-Projects/Jaglink/actions/workflows/ci.yml)

JAGLINK is the Jaguar X-Type X400 product face built on the shared LINK vehicle-diagnostics engine.

**Current source version:** see [`VERSION`](VERSION)  
**Shared engine:** exact LINK gitlink at `src/link`; LINK owns the nested Infiltratr Common pin  
**Platforms:** Linux, iPhone/iOS and Windows Discover  
**Licence:** GPL-3.0-or-later

## Role in the project family

```text
Infiltratr Common
        ↓
       LINK
        ↓
     JAGLINK
 Jaguar/X400 face
```

JAGLINK pins one exact LINK commit at `src/link`. LINK owns the Common dependency beneath it, so JAGLINK carries no second top-level Common submodule. CMake and CI validate the committed recursive gitlinks rather than maintaining duplicate expected-version constants.

Shared workspace, Classical CAN/CAN-FD ISO-TP, transport, ELM327, standard OBD-II, generic DTC knowledge, the complete product-neutral UDS service catalogue/codecs, scheduler/telemetry, portable diagnostic sequencing, Discover safety/evidence and the Windows OpenPort/J2534 shell belong in LINK. Jaguar identity, X400 definitions and genuinely Jaguar-specific behaviour remain in JAGLINK.

## Applications in this repository

JAGLINK is one manufacturer product family with more than one application target. It is intentionally **not** split into separate `JAGLINK` and `JAGLINK-Reader` repositories.

```text
JAGLINK repository
  |- JAGLINK
  |    normal Jaguar diagnostic application
  |
  `- JAGLINK Discover
       specialist Jaguar ECU/module discovery,
       identification, read-only inventory and evidence/dump tool
```

The main application is the normal driver/technician diagnostic experience. Discover is the deeper engineering-oriented reader used to determine what control modules are present, identify them, read documented information and preserve raw evidence.

The current Windows Discover implementation is the first stage of that role: passive CAN capture plus a bounded read-only standard OBD inventory. It should evolve in place into the deeper Jaguar ECU/module reader rather than spawning a separate repository or duplicate scanner implementation.

Generic Discover mechanics belong in LINK. Jaguar/X400 network topology, module identities, verified endpoints, read-only probes and decoders belong here.

## Capabilities

The current Jaguar layer represents the source-corroborated X400 network topology without inventing undocumented module addresses, PIDs or request formats:

- CAN — 500 kbit/s;
- SCP — 41.6 kbit/s;
- ISO 9141 diagnostic serial link — 10.4 kbit/s; and
- D2B optical infotainment network — 5.6 Mbit/s.

The product uses LINK's shared ELM327/OBD-II/UDS engine, Classical CAN and CAN-FD ISO-TP support, portable diagnostic-flow controller and JAGLINK Discover specialist reader target. Product-prefixed compatibility headers expose LINK's 64-byte CAN-FD contract and complete 27-service generic UDS catalogue without duplicating implementations.

Linux adapter discovery is also shared: tty/RFCOMM ELM327, Vgate-style BLE/GATT, Bluetooth Classic/SPP and direct USB Tactrix OpenPort 2.0 all come from LINK rather than JAGLINK-specific transport code. The native OpenPort bridge currently covers the CAN/ISO-15765 modes used by the present LINK-family diagnostics; K-line is not claimed until LINK exposes it through that bridge.

Manufacturer-specific meanings remain evidence-gated until documentation or reproducible vehicle captures establish them.

## Architecture

Portable diagnostic behaviour is C11. C++ is used only where it materially improves a design. Platform-required languages remain narrow presentation/interop edges and must not become alternate protocol implementations.

The Linux shell is C/GTK4 and embeds the canonical Jaguar+OBD emblem as a registered GResource. The iPhone project is `app/ios/JAGLINK.xcodeproj`; SwiftUI remains a presentation edge over the shared portable model.

The native iOS UDS bridge compiles the exact pinned LINK core UDS and service-codec implementations. CI verifies that bridge explicitly so a new shared UDS implementation cannot disappear from iOS while remaining present in CMake builds.

Windows Discover uses LINK's shared native shell with Common Controls v6 styling, DPI-aware layout, evidence annotations and UTF-8-safe status rendering. It is the current platform implementation of the specialist Jaguar ECU/module reader. The executable uses the canonical iPhone app icon as its Windows product image, embeds version/copyright metadata and links the static MSVC runtime.

As Discover gains manufacturer-aware depth, the boundary stays the same: reusable interrogation, transport, safety and evidence behaviour goes into LINK; Jaguar-specific topology, module definitions and evidence-backed read-only requests remain in JAGLINK.

CI launches the built Windows executable, waits for the `JAGLINK Discover` main window, verifies that it remains alive and closes it cleanly. Compilation alone is not sufficient for release.

## Build and test

```bash
git clone --recurse-submodules https://github.com/Infiltrator-Projects/Jaglink.git
cd Jaglink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On Linux, a connected Tactrix appears as `OP2:Tactrix OpenPort 2.0` in the same LINK-owned adapter chooser as Vgate/ELM interfaces. It is driven directly through LINK/libusb and does not require the Windows J2534 DLL.

GitHub CI verifies the exact recursive dependency tree, portable core, product-to-LINK facade, sanitizer coverage, Linux application/package path, Windows Discover executable and launch smoke test, Apple/iOS build and unsigned physical-device IPA before any release job can run.

The native Linux `.run` contains the complete source/dependency tree, builds JAGLINK with tests enabled, runs CTest, and installs only after the native test suite passes.

## Release assets

A successful numbered release is atomic across supported targets and publishes:

| File | Purpose |
| --- | --- |
| `JAGLINK-<version>-unsigned.ipa` | Unsigned physical-device iPhone package. |
| `JAGLINK-<version>-linux-amd64.deb` | Generic Linux amd64 Debian package. |
| `JAGLINK-<version>-linux-native.run` | Native local Linux build/test/install program. |
| `JAGLINK-<version>-windows-discover.exe` | JAGLINK Discover specialist read-only ECU/module scanner and evidence application. |
| `JAGLINK-<version>-source.zip` | Exact tested source archive including the pinned dependency tree. |
| `SHA256SUMS.txt` | SHA-256 checksums for all project-owned release artifacts. |

Release notes derive the LINK and Common versions from that exact dependency tree instead of maintaining separate hard-coded version strings.

## Repository and release policy

This repository uses `main` as its working branch. Development changes are made directly on `main`; the normal project workflow does not depend on PR, feature or release branches.

Every push to `main` runs JAGLINK CI. Ordinary commits do not publish. A commit is release-eligible only when its subject begins with the exact source version as `Release <version>` and every required CI job succeeds.

The release job re-checks that the tested SHA is still the exact current `main`, verifies the complete artifact set, creates the version tag and publishes the atomic release. Existing version tags and published releases are immutable and are never moved, replaced or edited in place.

Manually runnable build/smoke workflows are diagnostic helpers only and are not release-approval mechanisms.

## Engineering rules

- Broadly reusable non-automotive primitives belong in Infiltratr Common.
- Shared automotive behaviour belongs in LINK.
- Jaguar-only definitions and behaviour stay in JAGLINK.
- A second JAGLINK application target does not require a second repository; Discover remains part of this manufacturer product family.
- Public APIs document ownership, lifetime, failure behaviour and invariants.
- Comments explain rationale and non-obvious state-machine constraints rather than obvious syntax.
- Unknown or unsafe diagnostic services are denied before transport transmission.

## Documentation

- [`docs/ORIGIN.md`](docs/ORIGIN.md) owns repository and dependency boundaries.
- [`docs/ROADMAP.md`](docs/ROADMAP.md) owns current completion status and priorities.
- [`docs/JAGUAR.md`](docs/JAGUAR.md), [`docs/DISCOVER.md`](docs/DISCOVER.md) and [`docs/APPLE.md`](docs/APPLE.md) are scoped vehicle, specialist-reader and platform notes; they do not override LINK's shared contracts.

The committed gitlinks and `VERSION` files are the sole authorities for dependency and source versions.

## Licence

Copyright © 2026 Xavier Wheaton and Shannon Smith.

JAGLINK is free software licensed under the GNU General Public License version 3 or, at your option, any later version (`GPL-3.0-or-later`).
