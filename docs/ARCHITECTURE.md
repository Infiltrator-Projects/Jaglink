# Architecture

## Purpose

JAGLINK is the Jaguar product face over LINK, owning Jaguar-specific identity, X400/vehicle evidence, module discovery and interpretation while reusing the shared diagnostics engine.

## System decomposition

- Jaguar product core/facade
- LINK shared diagnostics engine
- Linux application
- native iPhone application
- Windows Discover path
- Jaguar-specific evidence and regression tests

## Ownership boundaries

Generic protocol/session/safety/application mechanics belong in LINK. Jaguar VIN interpretation, module identities, endpoints, probes and manufacturer evidence belong in JAGLINK.

Shared code flows downward through explicit dependencies. Product repositories should not copy shared protocol/session/application logic merely to customise manufacturer content. Conversely, manufacturer-specific evidence must not leak into LINK/Common abstractions.

## Contract boundaries

Protocol decoding, request planning, transport capability, safety permission and manufacturer interpretation are separate concerns. A decoder being able to represent a service does not imply that the product is allowed to transmit it.

## Source of truth

Code and tests define executable behaviour. Pinned gitlinks define dependency identity. Specialist evidence documents define narrower manufacturer/protocol facts and must not conflict with the ownership model above.

## Specialist documentation

- docs/JAGUAR.md
- docs/ORIGIN.md
- docs/DISCOVER.md
- docs/APPLE.md
