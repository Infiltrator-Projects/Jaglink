<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK roadmap

## Completed foundation

- JAGLINK product identity and canonical Jaguar+OBD branding;
- X400 CAN/SCP/ISO-9141/D2B topology and provenance contracts;
- shared LINK workspace, ISO-TP, parameter/store/scheduler/telemetry runtime;
- shared LINK Discover safety/evidence and OpenPort/J2534 scanner;
- C/GTK4 Linux shell with standard About dialog;
- aligned iPhone About experience and two-author attribution;
- unsigned physical-device IPA, Linux DEB/RUN and Windows Discover release jobs.

## Next shared-engine migrations

1. move the generic ELM327 command/parser/session/CAN/probe implementation into LINK;
2. move standard OBD-II into LINK;
3. move generic UDS into LINK;
4. consolidate common Apple transport/controller glue;
5. consolidate genuinely shared application-shell and packaging structure.

Each migration must leave LINK as the source of truth and reduce the product copy to manufacturer-specific code or a compatibility adaptor.

## Jaguar diagnostic milestones

### Read-only X400 discovery

- represent verified Jaguar module/network endpoints independently of transport providers;
- preserve raw request/response evidence;
- classify positive, negative, no-response and invalid results;
- avoid assigning meanings to undocumented payloads until corroborated.

### Identity and faults

- enumerate verified control modules across applicable X400 networks;
- decode documented identity and fault-memory structures;
- expose module provenance/network path in the shared workspace;
- add physical-vehicle trace fixtures before promoting definitions to vehicle-verified.

### Jaguar live data

- add manufacturer descriptors only for documented or fixture-verified values;
- merge Jaguar values into LINK's shared parameter/store/scheduler/telemetry runtime;
- preserve generic OBD-II values as a separate standards-based source.
