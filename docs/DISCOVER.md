<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK Discover

JAGLINK Discover is the Jaguar-branded specialist ECU/module discovery and read-only evidence/dump application built on the shared LINK Discover subsystem.

It is not a separate repository and it is not a future `JAGLINK-Reader` product. It is already the second application target inside the JAGLINK repository, alongside the main JAGLINK diagnostic application.

```text
JAGLINK repository
  |- main JAGLINK application
  `- JAGLINK Discover
       Jaguar ECU/module reader and evidence tool
```

## Ownership

JAGLINK does not own a separate generic discovery engine or Windows OpenPort/J2534 scanner implementation. Shared discovery behaviour, transport handling, safety classification, evidence writing, standard OBD inventory and generic ECU/module interrogation machinery live in LINK.

JAGLINK supplies the Jaguar-specific layer:

- Jaguar/X400 network topology and module knowledge;
- known module identities and verified endpoints;
- evidence-backed Jaguar read-only probes and identifiers;
- Jaguar-specific decoders where meaning is documented or reproducibly verified;
- JAGLINK branding, iconography and presentation.

The JAGLINK compatibility API is exposed through `include/jaglink/discover.h`, which aliases product-prefixed names to the shared `link_*` API without duplicating implementation.

## Shared Windows scanner

`jaglink-discover.exe` is created through LINK's `link_add_windows_discover` constructor and uses the same shared implementation as MBLINK Discover.

The equivalent MBLINK executable therefore shares transport, capture, safety and evidence behaviour. Differences in discovered modules, identifiers or decoded information are permitted only where the product layer supplies genuine manufacturer-specific knowledge.

## Current implementation

The shared Windows target is built as a Win32 executable because the commonly installed Tactrix OpenPort 2.0 J2534 FunctionLibrary is 32-bit. The GUI searches standard J2534 04.04 registry locations for an OpenPort entry and reads its `FunctionLibrary` value. If no entry is found, the DLL path remains editable.

The current Discover baseline provides:

- passive 500 kbit/s CAN capture;
- bounded read-only standard OBD inventory;
- deny-by-default request classification;
- JSON Lines evidence export and operator annotations;
- JAGLINK-branded Windows presentation.

This is the starting point, not the final intended scope.

## Intended evolution

JAGLINK Discover should evolve in place into the deeper Jaguar engineering reader/dumper:

```text
passive network observation
    -> standards-based inventory
    -> Jaguar-aware module discovery
    -> ECU/module identification
    -> documented read-only information acquisition
    -> structured raw/evidence dump
```

The main JAGLINK application remains the normal diagnostic experience. Discover is the specialist tool for determining what Jaguar control modules exist, identifying them, reading supported information and preserving raw evidence for analysis.

The X400 platform is particularly important here because it spans several distinct network technologies. The shared LINK engine must not assume that every module is a conventional 500 kbit/s CAN/OBD endpoint. JAGLINK owns the verified X400 topology and must supply manufacturer-specific knowledge as each network and module path becomes evidence-backed.

A "dump" here means bounded read-only acquisition of diagnostic information and raw responses. It does not imply unrestricted coding, flashing, reset, security-access or programming capability.

## Safety and evidence

The safety classifier and evidence writer are implemented in LINK under `src/discover/`. Unknown services and write/control operations remain blocked by default. Captured frames, result records and operator annotations use the same shared schema in MBLINK and JAGLINK.

Any manufacturer-specific read request must be documented or reproducibly verified, bounded and admitted through the same LINK safety boundary before it can reach the transport.

## Architecture rule

Any generic scanner, transport, safety, evidence or reader improvement belongs in LINK first. Any Jaguar-only definition or evidence-backed request belongs in JAGLINK.

Do not create a separate JAGLINK Reader repository and do not fork LINK's scanner to add Jaguar depth. Extend the shared Discover engine where the behaviour is generic and supply Jaguar knowledge from this product repository.

The full behavioural, safety and repository contract is documented in LINK's `docs/DISCOVER.md` and `docs/PRODUCT_FACES.md`.

## Build on Windows

With recursive submodules initialised:

```powershell
cmake -S . -B build-win32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DJAGLINK_BUILD_WINDOWS_DISCOVER=ON
cmake --build build-win32 --config Release --parallel
ctest --test-dir build-win32 -C Release --output-on-failure
```

The executable is `build-win32/Release/jaglink-discover.exe`.
