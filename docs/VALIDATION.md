# Validation

## Evidence model

Vehicle diagnostics needs several evidence layers: pure protocol/unit tests, captured-traffic replay, platform/adapter integration and physical vehicle validation. These layers complement one another but are not interchangeable.

## Automated gates

- .github/workflows/ci.yml
- .github/workflows/release-policy.yml
- .github/workflows/ci-status.yml

tests/ exercises Jaguar VIN/product logic plus shared diagnostics integration, ELM327, ISO-TP, UDS, DTC knowledge, evidence, scheduler and LINK facade contracts.

## Physical/manual evidence

Real Jaguar vehicles, adapters and network lanes remain physical evidence boundaries. Hosted/shared tests demonstrate code contracts but not every vehicle topology.

A replay proves deterministic handling of that capture. It does not prove every adapter, ECU software version or vehicle topology. A simulator build proves source/platform integration, not physical Bluetooth/USB behaviour.

## Safety validation

Regression coverage must ensure that adding a codec, DID, module or transport cannot silently broaden transmit permissions. Failed/not-scanned/scanning/clean states remain semantically distinct.

## Release criterion

The exact source/dependency tree intended for release must pass the required CI gates. Release notes and documentation must reflect the evidence actually held for that revision.
