<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# JAGLINK Discover

`jaglink-discover` is the Windows OpenPort 2.0 / SAE J2534 discovery front end for JAGLINK 0.2.0. It is intentionally a forensic, read-only discovery tool rather than a programming or actuator-control utility.

## Supported discovery path

The Windows target is built as a Win32 executable because the commonly installed Tactrix OpenPort 2.0 J2534 FunctionLibrary is 32-bit. At startup the GUI searches the standard J2534 04.04 registry locations for an OpenPort entry and reads its `FunctionLibrary` value. If no entry is found, the DLL path remains editable and defaults to the conventional OpenPort 2.0 installation path.

The primary capture mode opens the J2534 device and connects using the CAN protocol at exactly 500000 bit/s. Capture mode calls only `PassThruReadMsgs`; it does not call `PassThruWriteMsgs`, so ordinary passive capture has no transmit path.

The bounded OBD inventory temporarily reconnects using ISO 15765 at 500000 bit/s, installs response flow-control filters for the conventional 11-bit OBD response range, and sends only these six standard information requests:

- Mode 01 PID 00 — supported current-data PIDs;
- Mode 09 PID 00 — supported vehicle-information PIDs;
- Mode 09 PID 02 — VIN;
- Mode 09 PID 04 — calibration ID;
- Mode 09 PID 06 — calibration verification numbers;
- Mode 09 PID 0A — ECU name.

Each request is safety-classified before the J2534 transmit function can be reached. The inventory is finite and returns to passive CAN capture when it finishes.

## Deny-by-default safety policy

The portable classifier in `src/discover/safety.c` admits only explicit read-only services. Standard OBD read services 01, 03, 07, 09 and 0A are classified read-only. UDS ReadDTCInformation (19) and ReadDataByIdentifier (22) are also classified read-only for future evidence tooling.

Everything else is blocked unless explicitly classified. In particular the classifier blocks OBD DTC clearing and control operations, UDS ECU reset, ClearDiagnosticInformation, SecurityAccess, Authentication, WriteDataByIdentifier, InputOutputControlByIdentifier, CommunicationControl, RoutineControl, WriteMemoryByAddress, RequestDownload, RequestUpload, TransferData and RequestTransferExit. Unknown service identifiers are blocked by default.

The Windows inventory itself uses only OBD modes 01 and 09.

## Evidence JSON Lines

Every captured or inventory frame can be written to a timestamped `.jsonl` evidence stream. Frame records contain:

```json
{"type":"frame","timestamp_ns":123456789,"direction":"rx","protocol":"CAN","can_id":"0x000007E8","data":"4100...","annotation":"passive 500 kbit/s capture"}
```

Operator notes are separate records:

```json
{"type":"annotation","timestamp_ns":123456790,"text":"Ignition switched from II to 0"}
```

Strings are JSON-escaped by the portable evidence writer. The GUI keeps a session evidence file in the Windows temporary directory and the **Export JSONL** button copies the flushed evidence to an operator-selected path.

## Build on Windows

Use a Visual Studio developer environment with the repository and its submodule checked out:

```powershell
cmake -S . -B build-win32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DJAGLINK_BUILD_WINDOWS_DISCOVER=ON
cmake --build build-win32 --config Release --parallel
ctest --test-dir build-win32 -C Release --output-on-failure
```

The executable is `build-win32/Release/jaglink-discover.exe`.

## Portable tests

`jaglink-test-discover-safety` verifies the allowlist and all explicit high-risk deny classes. `jaglink-test-evidence` verifies JSONL frame output, timestamping, hexadecimal payload encoding, annotations and JSON string escaping. Both tests run on the normal portable C11 CI matrix and in the Windows build.
