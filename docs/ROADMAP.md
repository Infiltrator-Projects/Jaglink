<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK roadmap

JAGLINK is one Jaguar product family containing both the normal JAGLINK diagnostic application and the specialist JAGLINK Discover application. Discover is not a separate repository or future `JAGLINK-Reader`; it is the existing branded ECU/module discovery and read-only evidence/dump target and should evolve in place.

## Completed foundation

- JAGLINK product identity and canonical Jaguar+OBD branding;
- X400 CAN/SCP/ISO-9141/D2B topology and provenance contracts;
- shared LINK workspace, ISO-TP, parameter/store/scheduler/telemetry runtime;
- shared LINK Discover safety/evidence and OpenPort/J2534 scanner;
- C/GTK4 Linux shell with standard About dialog;
- aligned iPhone About experience and two-author attribution;
- unsigned physical-device IPA, Linux DEB/RUN and Windows Discover release jobs.

## Current architecture

```text
Infiltratr Common
        |
       LINK
        |
     JAGLINK
      /   \
 main app  Discover
            |
       Jaguar ECU/module
       reader + evidence
```

The main application remains the normal Jaguar diagnostic experience. JAGLINK Discover is the engineering-oriented reader used for deeper module discovery, identification, bounded read-only acquisition and structured raw/evidence dumping.

Generic reader/scanner mechanics belong in LINK. Jaguar/X400 topology, module definitions, verified endpoints, evidence-backed read-only requests and decoders belong in JAGLINK.

## Shared-engine consolidation

Generic automotive behaviour should continue moving into LINK until the product layer contains only genuinely Jaguar-specific implementation or thin compatibility facades.

Current shared ownership already includes ELM327, standard OBD-II, generic UDS, ISO-TP, diagnostic sequencing, Discover safety/evidence and the Windows scanner shell. Remaining consolidation should focus on reusable Apple transport/controller glue and genuinely shared application-shell/packaging structure rather than reintroducing product copies.

Each migration must leave LINK as the source of truth and reduce the product copy to manufacturer-specific code or a compatibility adaptor.

## JAGLINK Discover completion track

Current baseline:

- shared LINK Windows OpenPort/J2534 shell;
- passive 500 kbit/s CAN capture;
- bounded read-only standard OBD inventory;
- deny-by-default request classification;
- structured evidence export and operator annotations;
- JAGLINK branding and Jaguar product identity.

Intended evolution:

```text
passive network observation
    -> standards-based inventory
    -> Jaguar-aware module discovery
    -> ECU/module identification
    -> documented read-only information acquisition
    -> structured raw/evidence dump
```

Next work:

1. Define a shared LINK module-discovery/result model that can represent multiple network technologies rather than assuming a single CAN/OBD bus.
2. Feed JAGLINK's verified X400 CAN/SCP/ISO-9141/D2B topology into that model without hard-coding Jaguar knowledge into LINK.
3. Enumerate only evidence-backed Jaguar module endpoints and preserve positive, negative, no-response, unsupported and blocked states distinctly.
4. Add bounded identity and documented read-only information acquisition for verified modules.
5. Produce a structured Discover dump containing raw requests/responses, module identity, network path, result status, timestamps and product/profile provenance.
6. Preserve raw evidence whenever a proprietary field cannot yet be decoded confidently.
7. Keep reset, security access, routines, DTC clearing, coding, programming and firmware-write operations outside the Discover allowlist unless a separately reviewed product capability explicitly requires them.

Discover remains part of this repository. A separate JAGLINK Reader repository would duplicate the existing product boundary and is not part of the roadmap.

## Jaguar diagnostic milestones

### Read-only X400 discovery

- represent verified Jaguar module/network endpoints independently of transport providers;
- preserve raw request/response evidence;
- classify positive, negative, no-response, unsupported and invalid results;
- avoid assigning meanings to undocumented payloads until corroborated;
- expose this depth through JAGLINK Discover rather than a parallel reader application.

### Identity and faults

- enumerate verified control modules across applicable X400 networks;
- decode documented identity and fault-memory structures;
- expose module provenance/network path in the shared workspace;
- add physical-vehicle trace fixtures before promoting definitions to vehicle-verified;
- reuse the same verified identities/definitions in both Discover and the main JAGLINK application where appropriate.

### Jaguar live data

- add manufacturer descriptors only for documented or fixture-verified values;
- merge Jaguar values into LINK's shared parameter/store/scheduler/telemetry runtime;
- preserve generic OBD-II values as a separate standards-based source.

## Development principle

Each feature begins at the lowest reusable layer that can correctly own it. A second application target does not justify a second repository or a duplicated protocol/scanner implementation. If both MBLINK and JAGLINK need the behaviour, implement it in LINK; if it is genuinely Jaguar-specific, keep it here.
