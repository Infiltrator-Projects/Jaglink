<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK

JAGLINK is a standalone, C-first, open-source Jaguar diagnostics platform for the **Jaguar X-Type (X400), 2001–2009**, the Jaguar/Ford CD132 platform related to the contemporary Mondeo.

**Current release: 0.1.0 — X400 foundation.**

The repository contains its own Jaguar-branded public C API, portable protocol implementation, native iPhone application, and native Linux application. It has no MBLINK source, submodule, build-time dependency, runtime dependency, API namespace, or user-facing identity. Infiltratr Common 1.10.0 is the only directly pinned shared-library submodule.

JAGLINK includes ELM327 transport/session handling, standard OBD-II, telemetry, scheduling, ISO-TP and UDS foundations together with its Jaguar manufacturer layer under `src/jaguar`. It models the source-corroborated X400 network topology without inventing module addresses, proprietary PIDs or request formats that have not been verified.

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

Infiltratr Common 1.10.0 is pinned directly at `src/infiltratr-common`, commit `182e64cb8b8992879e443b941565058166fe0161`.

## iPhone

The native project is `app/ios/JAGLINK.xcodeproj`. Its Jaguar-branded CoreBluetooth provider and diagnostics controller are maintained in this repository. The 0.1.0 app provides adapter connection, generic OBD-II capability discovery, generic fault scans, live parameters, X400 network provenance and CSV export.

## Linux

```sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DJAGLINK_BUILD_LINUX_APP=ON
cmake --build build-linux --target jaglink-linux
./build-linux/jaglink-linux
```

## Engineering policy

- `main` is the development branch.
- Generic diagnostic behaviour stays portable C.
- Jaguar definitions live in JAGLINK, not in the ELM/BLE provider.
- Manufacturer-specific requests remain unimplemented until source evidence and/or reproducible vehicle captures establish their meaning.
- The source, public API, Apple classes, build targets and user interfaces use the JAGLINK/JagLink namespace.

See `docs/JAGUAR.md`, `docs/APPLE.md`, `docs/ORIGIN.md` and `docs/ROADMAP.md`.

## Licence

Copyright (C) 2026 Shannon Smith.

JAGLINK is licensed under `GPL-3.0-or-later`.
