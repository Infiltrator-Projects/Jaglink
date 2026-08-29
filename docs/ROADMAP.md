<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK roadmap

JAGLINK is one Jaguar product family containing both the normal JAGLINK diagnostic application and the specialist JAGLINK Discover application. Discover is not a separate repository or future `JAGLINK-Reader`; it is the existing branded ECU/module discovery and read-only evidence/dump target and should evolve in place.

## Completed foundation

- JAGLINK product identity and canonical Jaguar+OBD branding;
- X400 CAN/SCP/ISO-9141/D2B topology and provenance contracts;
- source-corroborated X400 CAN diagnostic endpoints for climate, ECM, TCM, instrument cluster and ABS/DSC;
- a first source-corroborated Jaguar factory DTC catalogue with module/category provenance;
- source-corroborated X400 `CAN FUEL USED` signal identity at CAN ID `0x44D`, with decoding deliberately still unverified;
- shared LINK workspace, ISO-TP, parameter/store/scheduler/telemetry runtime;
- shared LINK Discover safety/evidence and OpenPort/J2534 scanner;
- shared LINK CoreBluetooth diagnostic-session engine on iPhone rather than a JAGLINK protocol copy;
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

Current shared ownership already includes CoreBluetooth session lifecycle, ELM327, standard OBD-II, generic DTC knowledge, generic UDS, ISO-TP, diagnostic sequencing, telemetry/CSV, Discover safety/evidence and the Windows scanner shell. JAGLINK's Apple controller is a thin Jaguar product adapter over that shared implementation. Remaining consolidation should focus only on genuinely reusable application-shell/packaging structure rather than reintroducing product copies.

Each migration must leave LINK as the source of truth and reduce the product copy to manufacturer-specific code or a compatibility adaptor.

## Current X400 evidence baseline

JAGLINK now has a concrete source-corroborated Jaguar knowledge layer. These definitions are useful discovery targets but remain distinct from vehicle verification.

Documented network topology:

- powertrain CAN — 500 kbit/s;
- body SCP — 41.6 kbit/s;
- serial diagnostic link / ISO 9141 — 10.4 kbit/s;
- D2B optical infotainment — 5.6 Mbit/s.

Source-corroborated CAN diagnostic endpoint pairs currently represented in `src/jaguar/jaguar.c`:

| Module | Request | Response | Verification state |
| --- | ---: | ---: | --- |
| Climate / A/CCM | `0x7C4` | `0x7C5` | source-corroborated |
| ECM | `0x7E8` | `0x7EC` | source-corroborated |
| TCM | `0x7E9` | `0x7ED` | source-corroborated |
| Instrument cluster | `0x7EA` | `0x7EE` | source-corroborated |
| ABS / DSC | `0x7EB` | `0x7EF` | source-corroborated |

The product also carries a first factory DTC set for instrument-cluster, restraints, ABS/DSC and network faults, including `B1202`, `B1204`, `B1205`, `B1213`, `B1231`, `C1095`, `C1137`, `C1145`, `C1155`, `C1956`, `U1041`, `U1135`, `U1147`, `U1262` and module-specific `U1900` entries. These remain source-corroborated until reproduced on a physical X400 fixture.

Jaguar's electrical documentation also identifies CAN ID `0x44D` as `CAN FUEL USED`, sent from the ECM for trip-computer calculations. JAGLINK preserves that identity and provenance but does not yet claim a numerical byte layout or scaling; `decoder_verified` therefore remains false.

## JAGLINK Discover completion track

Current baseline:

- shared LINK Windows OpenPort/J2534 shell;
- passive 500 kbit/s CAN capture;
- bounded read-only standard OBD inventory;
- deny-by-default request classification;
- structured evidence export and operator annotations;
- JAGLINK branding and Jaguar product identity;
- source-corroborated Jaguar/X400 topology and CAN endpoint catalogue available to the product layer.

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

1. Extend the shared LINK module-discovery/result model where necessary to represent X400's multiple network technologies without assuming a single CAN/OBD bus.
2. Feed the existing X400 CAN/SCP/ISO-9141/D2B topology and source-corroborated CAN endpoints into Discover execution without hard-coding Jaguar knowledge into LINK.
3. Capture physical X400 responses for the existing endpoint catalogue and preserve positive, negative, no-response, unsupported and blocked states distinctly.
4. Promote endpoints from source-corroborated to vehicle-verified only when reproducible physical traces exist.
5. Add bounded identity and documented read-only information acquisition for verified modules.
6. Produce a structured Discover dump containing raw requests/responses, module identity, network path, result status, timestamps and product/profile provenance.
7. Preserve raw evidence whenever a proprietary field cannot yet be decoded confidently.
8. Keep reset, security access, routines, DTC clearing, coding, programming and firmware-write operations outside the Discover allowlist unless a separately reviewed product capability explicitly requires them.

Discover remains part of this repository. A separate JAGLINK Reader repository would duplicate the existing product boundary and is not part of the roadmap.

## Jaguar diagnostic milestones

### Read-only X400 discovery

- execute the existing source-corroborated Jaguar module/network endpoints through shared transport providers;
- preserve raw request/response evidence;
- classify positive, negative, no-response, unsupported and invalid results;
- avoid assigning meanings to undocumented payloads until corroborated;
- expose this depth through JAGLINK Discover rather than a parallel reader application.

### Identity and faults

- enumerate verified control modules across applicable X400 networks;
- decode documented identity and fault-memory structures;
- expose module provenance/network path in the shared workspace;
- add physical-vehicle trace fixtures before promoting definitions to vehicle-verified;
- validate the existing source-corroborated Jaguar factory DTC catalogue against captured module evidence;
- reuse the same verified identities/definitions in both Discover and the main JAGLINK application where appropriate.

### Jaguar live data

- add manufacturer descriptors only for documented or fixture-verified values;
- determine and validate the byte layout/scaling for the documented `0x44D` `CAN FUEL USED` signal before exposing it numerically;
- merge verified Jaguar values into LINK's shared parameter/store/scheduler/telemetry runtime;
- preserve generic OBD-II values as a separate standards-based source.

## Development principle

Each feature begins at the lowest reusable layer that can correctly own it. A second application target does not justify a second repository or a duplicated protocol/scanner implementation. If both MBLINK and JAGLINK need the behaviour, implement it in LINK; if it is genuinely Jaguar-specific, keep it here.
