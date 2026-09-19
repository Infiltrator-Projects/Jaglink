# Decisions

This file records durable JAGLINK architectural choices.

## ADR-001 — Jaguar-specific code stays above LINK

**Decision.** Generic diagnostics remain in LINK; JAGLINK owns Jaguar-specific identity, module/network knowledge and interpretation.

**Rationale.** One shared protocol/safety implementation prevents product divergence.

**Consequence.** Generic improvements should reduce JAGLINK code rather than spawn another local abstraction.

## ADR-002 — X400 evidence is a baseline, not universal Jaguar truth

**Decision.** X400 and other physical evidence is recorded with scope rather than generalized automatically.

**Rationale.** Jaguar platforms and software revisions can differ materially.

**Consequence.** Unsupported applicability stays explicit until verified.

## ADR-003 — Read-only discovery remains deny-by-default

**Decision.** Deeper discovery requires explicit operator intent and may send only requests permitted by the reviewed safety policy.

**Rationale.** Engineering discovery should not become accidental control/configuration capability.

**Consequence.** Safety tests remain independent from decoder/catalogue tests.

## ADR-004 — Apple diagnostic sessions are shared through LINK

**Decision.** CoreBluetooth/session sequencing lives in LINK with a thin Jaguar adapter.

**Rationale.** Duplicated Apple session engines previously risked drift from Linux/Windows and other vehicle products.

**Consequence.** Jaguar Apple code retains only platform and manufacturer-specific behaviour.

## ADR-005 — Raw evidence outranks guessed labels

**Decision.** Preserve raw responses when manufacturer meaning is not established.

**Rationale.** Wrong interpretation is worse than explicit uncertainty.

**Consequence.** Definitions require provenance before becoming normal product output.