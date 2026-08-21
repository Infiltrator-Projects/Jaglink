<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Upstream boundary

JAGLINK 0.1.0 is derived from MBLINK 0.7.12 at commit `e760b6ec05897c87e3531d68649b121403fcdec8`.

The pinned upstream tree supplies the already-tested protocol-neutral and standards-based implementation files used by the JAGLINK target: ELM327, OBD-II, parameter storage, scheduling, telemetry, ISO-TP and UDS. JAGLINK does not compile MBLINK's `src/mercedes` directory or Mercedes test suite.

JAGLINK owns its project identity, Jaguar profiles, Jaguar provenance, manufacturer-specific behaviour and user-facing Jaguar applications. The inherited public C ABI remains `mblink_*` at this stage so the fork can reuse the tested core without an ABI-only rewrite.

The nested upstream dependency pins Infiltratr Common 1.10.0 at `182e64cb8b8992879e443b941565058166fe0161`; JAGLINK verifies both pins at configure time when Git metadata is available.
