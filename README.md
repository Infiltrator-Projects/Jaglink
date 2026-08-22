<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK

JAGLINK is the Jaguar X-Type X400 product face built on the shared LINK vehicle-diagnostics engine.

**Current release: 0.2.12.**

## Dependency hierarchy

```text
Infiltratr Common
        ↓
       LINK
        ↓
     JAGLINK
 Jaguar/X400 face
```

JAGLINK pins LINK 0.9.1 at `src/link`. LINK owns Infiltratr Common 1.11.0 beneath it, so JAGLINK carries no second top-level Common dependency.

LINK owns the shared workspace, ISO-TP, byte-stream transport ABI, ELM327 framing/parser/initialisation, ELM-managed CAN, ELM session/probe, parameter/store/scheduler/telemetry runtime, standard OBD-II, UDS, the portable diagnostic-flow controller, Discover safety/evidence and the native Windows OpenPort 2.0/J2534 scanner shell. JAGLINK retains compatibility façades plus Jaguar identity, X400 definitions and genuinely Jaguar-specific behaviour.

## X400 foundation

The current Jaguar layer represents the source-corroborated X400 network topology without inventing undocumented module addresses, PIDs or request formats:

- CAN — 500 kbit/s;
- SCP — 41.6 kbit/s;
- ISO 9141 diagnostic serial link — 10.4 kbit/s;
- D2B optical infotainment network — 5.6 Mbit/s.

Manufacturer-specific meanings remain evidence-gated until documentation or reproducible vehicle captures establish them.

## Linux

The Linux shell is C/GTK4. Its canonical Jaguar+OBD emblem is embedded as a GResource and explicitly registered before GTK constructs either the main-window badge or About dialog, so installed and development launches use the same branding deterministically.

## Windows OpenPort/J2534 Discover

`jaglink-discover` is the Jaguar face of LINK's shared read-only scanner. LINK 0.9.1 supplies the same modern native Windows interaction model used by MBLINK: Common Controls v6 styling, File/Help menu, native Task Dialog About screen, DPI-aware layout, evidence log/annotations and UTF-8-safe status rendering.

The Windows EXE uses the exact `AppIcon-60@3x.png` from the iPhone asset catalogue as its canonical product image. LINK wraps those PNG bytes into the Windows resource and embeds version/copyright metadata at build time; JAGLINK no longer maintains an independent `.ico` file.

The EXE is linked against the static MSVC runtime. Windows CI also starts the built executable, waits for a `JAGLINK Discover` main window, verifies that it remains alive, and closes it cleanly. A release cannot publish merely because the EXE compiled.

## iPhone

The native project is `app/ios/JAGLINK.xcodeproj`. SwiftUI is confined to the native presentation edge; portable diagnostic behaviour remains C-first. The About experience follows the same dedicated modal structure as MBLINK while retaining Jaguar styling and the canonical Jaguar+OBD emblem.

## Release assets

A successful release must publish all of these top-level assets before CI can report success:

- `JAGLINK-X.Y.Z-unsigned.ipa`
- `JAGLINK-X.Y.Z-linux-amd64.deb`
- `JAGLINK-X.Y.Z-linux-native.run`
- `JAGLINK-X.Y.Z-windows-discover.exe`
- `JAGLINK-X.Y.Z-source.zip`
- `SHA256SUMS.txt`

The release workflow refuses stale runs, conflicting version tags or incomplete asset sets.

## Engineering policy

- C is the default implementation language; C++ is used where it materially improves the design.
- Platform-required languages stay at narrow UI/interop boundaries.
- Shared automotive behaviour belongs in LINK; broadly reusable primitives belong in Infiltratr Common.
- Jaguar-only definitions and behaviour stay in JAGLINK.
- Public APIs and non-obvious implementation decisions document contracts, ownership, lifetime, invariants, failure behaviour and rationale without narrating obvious syntax.
- Unknown or unsafe diagnostic services are denied before transport transmission.

## Licence

Copyright © 2026 Xavier Wheaton and Shannon Smith.

JAGLINK is licensed under `GPL-3.0-or-later`.
