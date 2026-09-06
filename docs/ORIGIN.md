<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Origin and ownership

JAGLINK is a thin Jaguar/X400 product face over LINK. The dependency hierarchy is Infiltratr Common → LINK → JAGLINK.

LINK owns the shared automotive application behaviour used across the LINK family: the operator-task workspace and information architecture, ISO-TP, byte-stream transport ABI, ELM327 framing/parser/initialisation, ELM-managed CAN, ELM session/probe, standard OBD-II, generic DTC knowledge, product-neutral UDS/KWP/DoIP foundations, parameter/store/scheduler/telemetry runtime, portable diagnostic sequencing, Discover safety/evidence, shared platform-shell behaviour and the Windows OpenPort/J2534 scanner.

JAGLINK owns Jaguar identity, X400-specific definitions, evidence-gated Jaguar behaviour, branding, manufacturer-specific content and product presentation choices that are not shared application behaviour. Product compatibility façades preserve the historical `jaglink_*` API while delegating shared runtime behaviour to LINK.

Protocols such as OBD-II and UDS are diagnostic data sources beneath LINK's shared operator-task interface rather than separate primary navigation destinations. JAGLINK may contribute Jaguar-specific topology, module scans, parameters, tests and service procedures without forking that shared task structure.

The original migration of standard OBD-II, UDS and generic diagnostic sequencing into LINK is complete. Ongoing work is consolidation only: remove historical compatibility façades and duplicated shell, CI, packaging or presentation mechanics whenever they can move into LINK without carrying Jaguar definitions, branding or manufacturer-specific behaviour with them.
