<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Contributing to JAGLINK

JAGLINK is the Jaguar product layer over LINK. Preserve the hierarchy **Common → LINK → JAGLINK** and keep product-neutral behaviour out of this repository.

## Ownership rules

- LINK owns transports, OBD, ISO-TP, UDS, diagnostic sequencing, safety/evidence and common application behaviour.
- Common owns broadly reusable non-automotive primitives beneath LINK.
- JAGLINK owns Jaguar identity, VIN interpretation, network/module evidence, manufacturer definitions and Jaguar-specific presentation.
- Do not create private protocol/session copies in Swift, Objective-C or product C code.
- Unknown manufacturer identifiers remain experimental until verified.
- Decode support does not grant transmit permission.

## Languages and platform boundaries

Use C/C++ for first-party portable/native behaviour where suitable; neither is preferred by policy. Swift and Objective-C are appropriate at Apple UI/CoreBluetooth boundaries. Platform layers should adapt LINK/JAGLINK contracts rather than redefine them.

## Build and test

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Protocol behaviour should remain testable without a vehicle. Sanitised physical captures are valuable regression inputs but must not be treated as universal Jaguar evidence.

## Documentation and evidence

Use `docs/README.md` as the map. Update canonical documents when ownership, design, decisions, support or validation changes. Jaguar-specific evidence belongs in the specialist Jaguar/Discover documents.

## Repository policy

`main` is the development/release branch; published tags/releases are immutable. Keep commits focused and dependency identities exact.

Participation standards remain in [.github/CODE_OF_CONDUCT.md](.github/CODE_OF_CONDUCT.md).