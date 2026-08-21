<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK

JAGLINK is a C-first, open-source Jaguar diagnostics platform derived from MBLINK. The initial vehicle family is the **Jaguar X-Type (X400), 2001–2009**, the Jaguar/Ford CD132 platform related to the Mondeo.

**Current development release: 0.1.0 — X400 foundation.**

JAGLINK pins the known-good MBLINK 0.7.12 source snapshot at `e760b6ec05897c87e3531d68649b121403fcdec8` under `upstream/mblink`. The JAGLINK builds deliberately select only MBLINK's reusable protocol-neutral and generic diagnostic layers: ELM327, standard OBD-II, telemetry, scheduling, ISO-TP and UDS. Mercedes-Benz manufacturer sources and tests are not part of JAGLINK.

JAGLINK adds its own Jaguar manufacturer layer under `src/jaguar`, its own iPhone diagnostic controller/UI, and a Jaguar-branded Linux shell. It currently models the source-corroborated X400 network topology without inventing module addresses, proprietary PIDs or request formats that have not been verified.

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

The pinned MBLINK snapshot recursively pins Infiltratr Common 1.10.0 at `182e64cb8b8992879e443b941565058166fe0161`.

## iPhone

The native project is `app/ios/JAGLINK.xcodeproj`. It reuses the generic MBLINK CoreBluetooth provider but uses JAGLINK's own controller and does not compile the Mercedes manufacturer layer. The 0.1.0 app provides adapter connection, generic OBD-II capability discovery, generic fault scans, live parameters, X400 network provenance and CSV export.

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
- MBLINK remains untouched by JAGLINK development.

See `docs/JAGUAR.md`, `docs/APPLE.md`, `docs/UPSTREAM.md` and `docs/ROADMAP.md`.

## Licence

Copyright (C) 2026 Shannon Smith.

JAGLINK is licensed under `GPL-3.0-or-later`.
