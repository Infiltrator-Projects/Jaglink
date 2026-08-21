<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK roadmap

## 0.1 — X400 foundation

- establish JAGLINK identity and main-only development;
- own the complete generic diagnostic source tree and Jaguar-branded public API;
- retain generic ELM327, OBD-II, telemetry, scheduler, ISO-TP and UDS coverage without an upstream diagnostics dependency;
- add X400 CAN/SCP/ISO-9141/D2B topology and provenance contracts;
- add Jaguar-specific regression tests and Linux shell identity.

## 0.2 — Read-only X400 discovery

- represent Jaguar module/network endpoints independently of transport providers;
- add safe, bounded read-only discovery for documented X400 diagnostic paths;
- preserve raw request/response evidence and classify positive, negative, no-response and invalid results;
- avoid assigning meanings to undocumented payloads until corroborated.

## 0.3 — Identity and faults

- enumerate verified control modules across the applicable X400 networks;
- decode documented module identity and fault-memory structures;
- expose module provenance and network path in the shared workspace;
- add physical-vehicle trace fixtures before promoting definitions to vehicle-verified.

## 0.4 — Jaguar live data

- add manufacturer live-data descriptors only for documented or fixture-verified values;
- merge Jaguar values into the existing parameter store, scheduler, table, dashboard and graphs;
- preserve generic OBD-II values as a separate standards-based source.

Later releases can extend the same Jaguar layer to other Ford-era Jaguar platforms without forcing X400 assumptions into the portable diagnostic core.
