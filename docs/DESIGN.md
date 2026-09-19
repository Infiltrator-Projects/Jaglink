# Design

## First-principles position

JAGLINK treats standards, captures, public documentation and mature diagnostic tools as evidence. Product behaviour is implemented from explicit contracts rather than by copying another tool or assuming undocumented manufacturer behaviour.

## Goals

- keep Jaguar-specific knowledge evidence-backed
- make the product face thin over LINK rather than forking shared code
- preserve read-only/deny-by-default discovery safety
- keep dependency identity pinned and reproducible

## Non-goals

A Jaguar-specific gap is not filled by copying another brand's assumptions. Simulator or generic OBD success does not prove an unverified Jaguar manufacturer request.

## Safety model

Read-only and write-capable actions are intentionally distinct. Capability discovery, protocol support and operator permission are not interchangeable. Unknown or failed scan states remain different from a clean result.

## Dependency policy

Generic automotive behaviour belongs in LINK; broadly reusable non-automotive mechanics belong in Common; manufacturer-specific behaviour belongs in the product face. Exact dependency revisions are pinned so later upstream changes cannot silently redefine a reviewed product.

## Evidence rule

A human-readable interpretation must be traceable to a standard, capture, verified public source or reproducible vehicle observation. Where evidence is incomplete, preserve raw values and uncertainty rather than inventing a label.

## Change quality

Newness is not a reason to replace a proven path. A change should improve fidelity, safety, coverage, performance or maintainability and include regression evidence for the contract it changes.
