<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK Discover

JAGLINK Discover is the JAGLINK-branded face of the shared LINK Discover subsystem.

JAGLINK does not own a separate discovery engine or Windows OpenPort/J2534 scanner implementation. Shared discovery behaviour, safety classification, evidence writing and the Windows scanner source live in LINK.

The JAGLINK compatibility API is exposed through `include/jaglink/discover.h`, which aliases the product-prefixed names to the shared `link_*` API without duplicating implementation.

## Shared Windows scanner

`jaglink-discover.exe` is created through LINK's `link_add_windows_discover` constructor. The target compiles LINK's single `platform/windows/link-discover.c` implementation and supplies only the JAGLINK product identity.

The equivalent MBLINK executable is built from the exact same source. Any difference in capture, safety, evidence or J2534 behaviour between the two products is therefore a regression.

## Supported discovery path

The shared Windows target is built as a Win32 executable because the commonly installed Tactrix OpenPort 2.0 J2534 FunctionLibrary is 32-bit. The GUI searches standard J2534 04.04 registry locations for an OpenPort entry and reads its `FunctionLibrary` value. If no entry is found, the DLL path remains editable.

The primary capture mode opens the device and connects using CAN at 500000 bit/s. Passive capture reads traffic only and does not use the transmit path.

The bounded OBD inventory temporarily reconnects using ISO 15765 at 500000 bit/s and sends only a finite set of standard read-only information requests. Every request is passed through LINK's deny-by-default safety classifier before the J2534 transmit function can be reached.

## Safety and evidence

The safety classifier and JSONL evidence writer are implemented in LINK under `src/discover/`. Unknown services and write/control operations remain blocked by default. Captured frames and operator annotations use the same evidence schema in MBLINK and JAGLINK.

The full behavioural contract, allowed product-face differences and regression rules are documented in LINK's `docs/DISCOVER.md` and `docs/PRODUCT_FACES.md`.

## Build on Windows

With recursive submodules initialised:

```powershell
cmake -S . -B build-win32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DJAGLINK_BUILD_WINDOWS_DISCOVER=ON
cmake --build build-win32 --config Release --parallel
ctest --test-dir build-win32 -C Release --output-on-failure
```

The executable is `build-win32/Release/jaglink-discover.exe`.
