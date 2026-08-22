<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK

JAGLINK is the Jaguar X-Type (X400, 2001–2009) product face of the shared LINK vehicle-diagnostics application engine. The X400 is the Jaguar/Ford CD132 platform related to the contemporary Mondeo.

**Current release: 0.2.1 — shared LINK Discover/OpenPort scanner plus unified Jaguar+OBD branding.**

The dependency model is:

```text
Infiltratr Common
        ↓
       LINK
        ↓
     JAGLINK
 Jaguar/X400 face
```

JAGLINK pins LINK 0.4.0 and Infiltratr Common 1.10.0. LINK owns generic vehicle-diagnostics application behaviour shared with MBLINK, including Discover safety/evidence behaviour and the Windows OpenPort 2.0/J2534 scanner source. JAGLINK owns only Jaguar identity, Jaguar/X400 definitions and genuinely Jaguar-specific diagnostic behaviour. There is no separate Jaguar fork of the shared Discover/scanner implementation.

The canonical JAGLINK product emblem is the Jaguar face with an OBD-II connector beneath it. The same artwork is used for the iPhone AppIcon/product asset, Linux application icon and Windows `jaglink-discover.exe` icon.

JAGLINK includes ELM327 transport/session handling, standard OBD-II, telemetry, scheduling, ISO-TP and UDS foundations together with its Jaguar manufacturer layer under `src/jaguar`. During the LINK migration, generic components still present in the product tree are progressively promoted into LINK rather than maintained as independent implementations. It models the source-corroborated X400 network topology without inventing module addresses, proprietary PIDs or request formats that have not been verified.

## X400 network foundation

Jaguar service training and the 2002 X-Type Electrical Guide describe four relevant vehicle networks:

- CAN — 500 kbit/s — engine, transmission and braking systems;
- SCP — 41.6 kbit/s — lower-speed body systems;
- ISO 9141 serial data link — 10.4 kbit/s — diagnostic link/ECM and diagnostic-capable modules outside CAN/SCP;
- D2B optical — 5.6 Mbit/s — in-car entertainment, with the audio unit acting as a network gateway.

These are represented as network definitions and provenance, not as claims that every network is already implemented by the adapter/provider layer.

## Build the portable core

```sh
git clone --recurse-submodules https://github.com/The-First-Infiltrator/Jaglink.git
cd Jaglink
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

If cloned without submodules:

```sh
git submodule update --init --recursive
```

The product currently pins:

- LINK 0.4.0 at `src/link`, commit `64779f2847f7a7262941d18889d1ffa663c20077`;
- Infiltratr Common 1.10.0 at `src/infiltratr-common`, commit `182e64cb8b8992879e443b941565058166fe0161`.

## Windows OpenPort 2.0 / J2534 discovery

`jaglink-discover` is the Jaguar-branded build of LINK's shared read-only Windows discovery tool. MBLINK and JAGLINK compile the same LINK scanner source; only product identity/resources differ. It supports passive 500 kbit/s CAN capture, a finite standard OBD information inventory, timestamped JSON Lines evidence and operator annotations. LINK's portable safety classifier blocks writes, resets, security/authentication access, routines, DTC clearing, programming/transfer services and all unknown services by default.

Build it as Win32 for compatibility with the common 32-bit OpenPort 2.0 J2534 driver:

```powershell
cmake -S . -B build-win32 -A Win32 -DJAGLINK_BUILD_WINDOWS_DISCOVER=ON
cmake --build build-win32 --config Release --parallel
ctest --test-dir build-win32 -C Release --output-on-failure
```

See `docs/DISCOVER.md` and LINK's `docs/DISCOVER.md` for the shared capture, OBD inventory, safety, evidence and product-face rules.

## iPhone

The native project is `app/ios/JAGLINK.xcodeproj`. Its AppIcon is built from the canonical Jaguar+OBD emblem. The Jaguar-branded CoreBluetooth provider and diagnostics controller provide adapter connection, generic OBD-II capability discovery, generic fault scans, live parameters, X400 network provenance and CSV export.

On macOS, `bash ./scripts/build-ios-ipa.sh` builds an unsigned physical-device IPA and its SHA-256 checksum. GitHub Actions also builds and validates the iOS simulator and physical-device IPA.

## Linux

```sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DJAGLINK_BUILD_LINUX_APP=ON
cmake --build build-linux --target jaglink-linux
./build-linux/jaglink
```

The Debian package installs the canonical Jaguar+OBD application icon and desktop entry. Published releases contain a normal Debian `.deb` and `JAGLINK-<version>-linux-native.run`. The `.run` file is self-extracting source and builds JAGLINK natively on the target machine.

## Release assets

A successful `main` release publishes the unsigned iPhone IPA and checksum, Linux `.deb`, native Linux `.run`, branded `jaglink-discover.exe`, and package checksums. All required build/test jobs must pass before the release job can publish those files.

## Engineering policy

- `main` is the development branch.
- Generic cross-product diagnostic/application behaviour belongs in LINK; more broadly reusable primitives belong in Infiltratr Common.
- Jaguar definitions and Jaguar-only diagnostic behaviour live in JAGLINK.
- Product branding is a face/configuration layer and must not fork shared behaviour.
- Discovery and evidence tooling is read-only by default; unsafe or unknown diagnostic services are rejected before transport transmission.
- Manufacturer-specific requests remain unimplemented until source evidence and/or reproducible vehicle captures establish their meaning.

See `docs/JAGUAR.md`, `docs/APPLE.md`, `docs/DISCOVER.md`, `docs/ORIGIN.md` and `docs/ROADMAP.md`.

## Licence

Copyright (C) 2026 Shannon Smith.

JAGLINK is licensed under `GPL-3.0-or-later`.
