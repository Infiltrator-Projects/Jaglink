<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Jaguar X400 diagnostics foundation

JAGLINK's first manufacturer target is the Jaguar X-Type, platform code X400, covering model years 2001–2009. The platform is Jaguar's Ford CD132-derived architecture related to the contemporary Mondeo.

## Shared Apple session architecture

The iPhone face no longer owns a separate copy of the ELM327/diagnostic-session engine. LINK 0.14.16 owns CoreBluetooth session lifecycle, ELM command scheduling, standard VIN/PID/DTC flow, telemetry history/favourites, CSV recording, simulation and prompt-safe recovery after an interrupted manufacturer extension. JAGLINK's Apple controller is now a thin product adapter that keeps only Jaguar VIN interpretation and Jaguar-facing presentation.

## Offline X400 VIN decoder

JAGLINK now decodes the X-TYPE/X400 VIN offline before applying manufacturer-specific diagnostic assumptions. This uses Jaguar Cars' own global X400 VIN layout rather than Mercedes-style Baumuster rules.

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

These facts are represented in `jaglink_jaguar_x400_profile()` as **source-corroborated**. They are topology and speed evidence only. They do not establish Jaguar module addresses, proprietary service identifiers, data formulas or security procedures.

Reference publications:

- Jaguar Cars, `Introduction to X-TYPE`, Service Training, Student Guide, 10 January 2002.
- Jaguar Cars, `Jaguar X-TYPE 2002 Electrical Guide`, publication S 2002 X-TYPE Issue 2, December 2001.
- Original Technical Publications catalogue, `Jaguar X-Type 2001 to 2009 (JTP1021)` / service manual `X400WKSM`.

## Verification states

`candidate` means plausible but insufficiently corroborated. `source-corroborated` means independent protocol/service evidence agrees but no JAGLINK vehicle regression fixture yet exists. `vehicle-verified` is reserved for exact request/response behaviour backed by a reproducible physical X400 capture committed as a regression fixture.

## Current boundary

0.2.0 adds read-only OpenPort 2.0/J2534 discovery without adding unverified Jaguar-specific request formats. It can passively capture the documented 500 kbit/s CAN network and run a strictly bounded standard OBD inventory. Evidence is recorded as timestamped JSON Lines with operator annotations, and a deny-by-default classifier blocks unsafe or unknown diagnostic services before transmission. Generic OBD-II remains available through JAGLINK's portable engine.

The next manufacturer work remains evidence-led: preserve reproducible X400 captures, corroborate module behaviour, and only then promote proprietary Jaguar definitions into the portable manufacturer layer.
