<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Origin and ownership

JAGLINK is a thin Jaguar/X400 product face over LINK. The dependency hierarchy is Infiltratr Common → LINK → JAGLINK.

LINK owns the shared workspace, ISO-TP, byte-stream transport ABI, ELM327 framing/parser/initialisation, ELM-managed CAN, ELM session/probe, parameter/store/scheduler/telemetry runtime, Discover safety/evidence and the shared Windows OpenPort/J2534 scanner.

JAGLINK owns Jaguar identity, X400-specific definitions, evidence-gated Jaguar behaviour, branding and product presentation. Product compatibility façades preserve the historical `jaglink_*` API while delegating shared runtime behaviour to LINK.

Standard OBD-II, UDS and generic diagnostic sequencing have now migrated to
LINK. JAGLINK keeps only compatibility façades over those shared APIs; future
migration work is limited to genuinely reusable shell/packaging mechanics that
can move without carrying Jaguar definitions or branding into LINK.
