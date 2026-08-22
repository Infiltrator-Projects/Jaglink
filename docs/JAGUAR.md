<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Jaguar X400 diagnostics foundation

JAGLINK's first manufacturer target is the Jaguar X-Type, platform code X400, covering model years 2001–2009. The platform is Jaguar's Ford CD132-derived architecture related to the contemporary Mondeo.

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
