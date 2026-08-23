<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK

[![JAGLINK CI](https://github.com/The-First-Infiltrator/Jaglink/actions/workflows/ci.yml/badge.svg)](https://github.com/The-First-Infiltrator/Jaglink/actions/workflows/ci.yml)

JAGLINK is the Jaguar X-Type X400 product face built on the shared LINK vehicle-diagnostics engine.

**Current source version:** 0.2.13  
**Shared engine:** LINK 0.9.1 → Infiltratr Common 1.11.0  
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

JAGLINK pins LINK at `src/link`. LINK owns the Common dependency beneath it, so JAGLINK carries no second top-level Common submodule.

Shared workspace, ISO-TP, transport, ELM327, standard OBD-II, UDS, scheduler/telemetry, portable diagnostic sequencing, Discover safety/evidence and the Windows OpenPort/J2534 shell belong in LINK. Jaguar identity, X400 definitions and genuinely Jaguar-specific behaviour remain in JAGLINK.

## Capabilities

The current Jaguar layer represents the source-corroborated X400 network topology without inventing undocumented module addresses, PIDs or request formats:

- CAN — 500 kbit/s;
- SCP — 41.6 kbit/s;
- ISO 9141 diagnostic serial link — 10.4 kbit/s; and
- D2B optical infotainment network — 5.6 Mbit/s.

The product uses LINK's shared ELM327/OBD-II/UDS engine, portable diagnostic-flow controller and read-only Windows OpenPort/J2534 Discover scanner.

Manufacturer-specific meanings remain evidence-gated until documentation or reproducible vehicle captures establish them.

## Architecture

Portable diagnostic behaviour is C11. C++ is used only where it materially improves a design. Platform-required languages remain narrow presentation/interop edges and must not become alternate protocol implementations.

The Linux shell is C/GTK4 and embeds the canonical Jaguar+OBD emblem as a registered GResource. The iPhone project is `app/ios/JAGLINK.xcodeproj`; SwiftUI remains a presentation edge over the shared portable model.

Windows Discover uses LINK's shared native shell with Common Controls v6 styling, DPI-aware layout, evidence annotations and UTF-8-safe status rendering. The executable uses the canonical iPhone app icon as its Windows product image, embeds version/copyright metadata and links the static MSVC runtime.

CI launches the built Windows executable, waits for the `JAGLINK Discover` main window, verifies that it remains alive and closes it cleanly. Compilation alone is not sufficient for release.

## Build and test

```bash
git clone --recurse-submodules https://github.com/The-First-Infiltrator/Jaglink.git
cd Jaglink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

GitHub CI verifies the portable core, sanitizer coverage, Linux application/package path, Windows Discover executable and launch smoke test, Apple/iOS build and unsigned physical-device IPA before any release job can run.

## Release assets

A successful numbered release is atomic across supported targets and publishes:

| File | Purpose |
| --- | --- |
| `JAGLINK-<version>-unsigned.ipa` | Unsigned physical-device iPhone package. |
| `JAGLINK-<version>-linux-amd64.deb` | Generic Linux amd64 Debian package. |
| `JAGLINK-<version>-linux-native.run` | Native local Linux build/install program. |
| `JAGLINK-<version>-windows-discover.exe` | Read-only Windows OpenPort/J2534 Discover application. |
| `JAGLINK-<version>-source.zip` | Exact tested source archive including the pinned dependency tree. |
| `SHA256SUMS.txt` | SHA-256 checksums for all project-owned release artifacts. |

## Repository and release policy

This repository uses `main` as its working branch. Development changes are made directly on `main`; the normal project workflow does not depend on PR, feature or release branches.

Every push to `main` runs JAGLINK CI. Ordinary commits do not publish. A commit is release-eligible only when its subject begins with the exact source version as `Release <version>` and every required CI job succeeds.

The release job re-checks that the tested SHA is still the exact current `main`, verifies the complete artifact set, creates the version tag and publishes the atomic release. Existing version tags and published releases are immutable and are never moved, replaced or edited in place.

Manually runnable build/smoke workflows are diagnostic helpers only and are not release-approval mechanisms.

## Engineering rules

- Broadly reusable non-automotive primitives belong in Infiltratr Common.
- Shared automotive behaviour belongs in LINK.
- Jaguar-only definitions and behaviour stay in JAGLINK.
- Public APIs document ownership, lifetime, failure behaviour and invariants.
- Comments explain rationale and non-obvious state-machine constraints rather than obvious syntax.
- Unknown or unsafe diagnostic services are denied before transport transmission.

## Licence

Copyright © 2026 Xavier Wheaton and Shannon Smith.

JAGLINK is free software licensed under the GNU General Public License version 3 or, at your option, any later version (`GPL-3.0-or-later`).
