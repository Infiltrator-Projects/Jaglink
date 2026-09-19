<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Security

## Supported source

Security fixes target current `main` and, where appropriate, the latest release.

## Reporting

Do not publish vulnerabilities that could expose users, vehicles, credentials, private data, build infrastructure or signing material.

Use GitHub private vulnerability reporting when available. Otherwise contact `infiltratr@yandex.com` with the subject `JAGLINK security report`.

Include the exact revision/dependency chain, platform, adapter/vehicle context, impact, reproduction and sanitised evidence.

## Automotive boundary

Builds, simulators and captured exchanges do not establish universal real-vehicle safety. Treat request permissions, routing, malformed adapter traffic and session/security handling as security-sensitive.

## Response

Reproduce safely, add regression coverage where practical, correct the underlying contract and validate physical hardware where the defect crosses that boundary. Do not perform testing that may endanger people, vehicles or third-party systems.

## Disclosure

Public details should follow a fix or clear mitigation and identify affected/corrected release identities.