<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Fork origin and dependency ownership

JAGLINK began from the generic diagnostic foundations in MBLINK 0.7.12 at commit `e760b6ec05897c87e3531d68649b121403fcdec8`. That commit records provenance only; JAGLINK does not depend on or load MBLINK.

Shared automotive functionality is progressively being promoted into LINK instead of remaining as parallel product implementations.

As of JAGLINK 0.2.5 / LINK 0.6.0, LINK is the source of truth for:

- workspace structure;
- Classical-CAN ISO-TP;
- parameter definitions, storage/history and formatting;
- parameter scheduling;
- telemetry storage/CSV;
- Discover safety/evidence;
- the Windows OpenPort 2.0/J2534 scanner.

JAGLINK retains product-prefixed compatibility adaptors where its existing public API delegates into those LINK contracts. ELM327, standard OBD-II and UDS remain product-local migration candidates at this release.

The repository pins only LINK at top level. LINK pins Infiltratr Common 1.10.0 beneath it, preserving the dependency hierarchy `Common -> LINK -> JAGLINK`.

Jaguar/X400 definitions and genuinely Jaguar-specific diagnostic behaviour remain owned by JAGLINK.
