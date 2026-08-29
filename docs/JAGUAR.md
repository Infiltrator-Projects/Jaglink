<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Jaguar X400 diagnostics foundation

JAGLINK's first manufacturer target is the Jaguar X-Type, platform code X400, covering model years 2001–2009. The platform is Jaguar's Ford CD132-derived architecture related to the contemporary Mondeo.

## Shared Apple session architecture

The iPhone face no longer owns a separate copy of the ELM327/diagnostic-session engine. LINK owns CoreBluetooth session lifecycle, ELM command scheduling, standard VIN/PID/DTC flow, telemetry history/favourites, CSV recording, simulation and prompt-safe recovery after an interrupted manufacturer extension. JAGLINK's Apple controller is now a thin product adapter that keeps only Jaguar VIN interpretation and Jaguar-facing presentation.

JAGLINK pins the exact LINK revision at `src/link`; documentation does not maintain a second hard-coded LINK version number that can drift from that gitlink.

## Offline X400 VIN decoder

JAGLINK decodes the X-TYPE/X400 VIN offline before applying manufacturer-specific diagnostic assumptions. This uses Jaguar Cars' own global X400 VIN layout rather than Mercedes-style Baumuster rules.

For X400, the fields are distributed across the VIN:

- positions 1-3: `SAJ` Jaguar WMI;
- position 4: market / airbag specification;
- position 5: drivetrain, transmission and steering combination;
- positions 6-7: X-TYPE body/series code (saloon/estate and High/Entry/Sport);
- position 8: Jaguar ECS emissions code;
- position 9: check digit;
- position 10: model year code;
- position 11: Halewood assembly/engine-line code;
- positions 12-17: production serial.

The position-11 engine-line codes documented by Jaguar distinguish 3.0 V6, 2.5 V6, 2.0 V6, 2.0 inline-four diesel and 2.2 TDCi inline-four diesel. Engine capacities and catalogue power are stored from Jaguar's published X-TYPE engine-data tables. Unknown or later codes are preserved but never guessed.

The normal diagnostic flow obtains a standard SAE Mode 09 PID 02 VIN through LINK. LINK owns only that generic acquisition. JAGLINK owns the Jaguar-specific interpretation and presents the resulting X400 body, drivetrain, transmission, steering, engine, model year, Halewood build identity and production serial. If an early vehicle returns `NO DATA` for Mode 09, diagnostics continue normally without treating VIN absence as a fault.

Jaguar's published Heritage Trust X400 development VIN `SAJAD56L64WD78435` is used as one regression example: Rest-of-World market, AWD/manual/RHD, Sport Series estate, 2004 model year, Halewood 3.0 V6 line, serial `D78435`.

## Network evidence

Jaguar's 2002 `Introduction to X-TYPE` service-training material gives the following communication-speed summary:

| Network | Documented role | Nominal speed |
| --- | --- | ---: |
| CAN | Engine, transmission, braking | 500 kbit/s |
| SCP | Lower-speed body systems | 41.6 kbit/s |
| Serial Data Link (ISO 9141) | DLC/ECM and diagnostic modules outside CAN/SCP | 10.4 kbit/s |
| D2B | In-car entertainment | 5.6 Mbit/s |

The Jaguar X-TYPE 2002 Electrical Guide independently describes CAN for high-speed powertrain communications, SCP for slower body systems, D2B optical audio, and technician access through the Data Link Connector. The training material also identifies the Audio Unit as the D2B network gateway.

These topology facts are represented in `jaglink_jaguar_x400_profile()` as **source-corroborated**. Topology/speed evidence alone does not establish proprietary request formats, byte layouts, scaling or security procedures.

## Source-corroborated CAN diagnostic endpoints

The Jaguar electrical-guide diagnostic data is represented separately from the topology evidence. JAGLINK currently carries these source-corroborated CAN diagnostic endpoint pairs:

| Module | Request CAN ID | Response CAN ID | State |
| --- | ---: | ---: | --- |
| Air conditioning control module | `0x7C4` | `0x7C5` | source-corroborated |
| Engine control module | `0x7E8` | `0x7EC` | source-corroborated |
| Transmission control module | `0x7E9` | `0x7ED` | source-corroborated |
| Instrument cluster | `0x7EA` | `0x7EE` | source-corroborated |
| ABS / DSC control module | `0x7EB` | `0x7EF` | source-corroborated |

These are discovery targets, not vehicle-verified claims. JAGLINK does not promote them to `vehicle-verified` until a reproducible physical X400 capture demonstrates the expected route and response behaviour.

## Factory DTC evidence

JAGLINK also carries an initial source-corroborated X400 manufacturer DTC set with module and category provenance. Current entries include:

- instrument cluster: `B1202`, `B1204`, `B1205`, `B1213`;
- restraints: `B1231`;
- ABS/DSC: `C1095`, `C1137`, `C1145`, `C1155`, `C1956`, `U1900`;
- GECM/network: `U1041`, `U1135`, `U1147`, `U1262`;
- instrument-cluster network: a module-specific `U1900` entry.

The catalogue preserves the module distinction where the same code can occur in more than one factory context. These definitions remain source-corroborated until physical evidence verifies the reporting module and behaviour.

## Fuel/trip-computer evidence

Jaguar's X-TYPE electrical-guide CAN message matrix identifies CAN ID `0x44D` as `CAN FUEL USED`, transmitted from the ECM for trip-computer calculations. JAGLINK represents this as a source-corroborated manufacturer signal.

The numerical byte layout and scaling are not yet vehicle-verified. The definition therefore has `decoder_verified = false` and must not be presented as a measured numerical fuel-used value until a reproducible capture establishes the encoding.

## Verification states

`candidate` means plausible but insufficiently corroborated. `source-corroborated` means independent protocol/service evidence agrees but no JAGLINK vehicle regression fixture yet exists. `vehicle-verified` is reserved for exact request/response behaviour backed by a reproducible physical X400 capture committed as a regression fixture.

## Current boundary

JAGLINK Discover provides read-only OpenPort 2.0/J2534 discovery without inventing Jaguar-specific request formats. It can passively capture the documented 500 kbit/s CAN network and run a strictly bounded standard OBD inventory. Evidence is recorded as timestamped JSON Lines with operator annotations, and a deny-by-default classifier blocks unsafe or unknown diagnostic services before transmission. Generic OBD-II remains available through JAGLINK's portable engine.

The Jaguar knowledge layer is now ahead of that generic Discover baseline: topology, five CAN diagnostic endpoint pairs, a first factory DTC set and the `0x44D` fuel-used signal identity are source-corroborated in the product repository. The next manufacturer work is to execute only defensible read-only probes, preserve reproducible X400 captures, corroborate module behaviour and then promote individual definitions to vehicle-verified.

Reference publications:

- Jaguar Cars, `Introduction to X-TYPE`, Service Training, Student Guide, 10 January 2002.
- Jaguar Cars, `Jaguar X-TYPE 2002 Electrical Guide`, publication S 2002 X-TYPE Issue 2, December 2001.
- Original Technical Publications catalogue, `Jaguar X-Type 2001 to 2009 (JTP1021)` / service manual `X400WKSM`.
