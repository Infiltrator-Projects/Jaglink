<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Fork origin

JAGLINK began from the generic diagnostic foundations in MBLINK 0.7.12 at commit `e760b6ec05897c87e3531d68649b121403fcdec8`.

That commit records provenance only. JAGLINK now owns its copied-and-forked ELM327, OBD-II, parameter storage, scheduling, telemetry, ISO-TP and UDS implementation files.

The repository does not clone, pin, import, compile or load MBLINK, and its public C ABI and Apple classes use the JAGLINK/JagLink namespace. No Mercedes-specific source or test is present.

Infiltratr Common is a separate shared library used across Shannon Smith's applications. JAGLINK pins Common 1.10.0 directly at `src/infiltratr-common`, commit `182e64cb8b8992879e443b941565058166fe0161`.
