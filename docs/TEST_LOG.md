# Public validation summary

Raw command output, machine inventory and original development notes are retained
locally under ignored docs/evidence/private. This document records engineering
results without personal paths, device identifiers or host security fingerprints.

## 2026-09-13 — Windows build milestones

- Initial Microsoft sample build failed with MSB8020 because WDK driver toolsets
  were missing. The self-contained EWDK supplied a supported toolchain.
- EWDK 26100.6584 / VS 2022 Build Tools 17.14.5: unchanged pinned Microsoft sample
  built successfully, exit 0, 0 warnings/errors; SignMode=Off and normal catalog/
  code-analysis stages enabled. Earlier assembly trust/loading failures resolved
  after an explicitly approved, ISO-specific unblock. No security policy changed.
- SweetDisplay Debug x64 driver, enumerator and catalogs built successfully.
  Build-SweetDisplay.cmd end-to-end exit 0; driver build 0 warnings/errors.
- Independent EDID decoding confirmed checksum, synthetic zero-serial identity,
  preferred 2400x1080 DTD and 172.8 MHz / (2560 * 1125) = 60 Hz.
  Shared mode-buffer decision tests covered count-only, insufficient capacity,
  null buffer, exact capacity and larger capacity. Live IddCx callbacks untested.
- Companion monitor INF passed InfVerif /w and Inf2Cat without errors/warnings.
- Encoder discovery probe built and returned hardware H.264 registrations.
  Activation, actual encoding, capture and end-to-end latency are untested.
- Packages remain unsigned and uninstalled. Actual Settings name, extension,
  active display mode, phone video, touch and camera remain unverified.

## 2026-09-14 — Approved repository hygiene

- Preserved original documents in a hash-verified ignored local backup.
- Moved raw evidence and the downloaded Xiaomi Makefile into private storage.
  Kept public upstream URLs/commit pins and the required Microsoft MS-PL license.
- Archived obsolete source-directory build output under ignored out/legacy-builds.
  The existing mounted ISO stays in place and is excluded by the ISO rule.
- Replaced operator conversation/workstation details with public engineering summaries;
  removed a hard-coded personal Fastboot path; retained runtime state locally.
- Added private/build/firmware/signing exclusions and a publication checker.
  Final checker result is recorded in STATUS and REPOSITORY_HYGIENE.
- No commit, remote push, history rewrite, driver installation or phone action.

Final hygiene verification: 63 publishable files (about 147 KiB total; largest
about 26 KiB), nine representative ignored paths verified, Markdown local links
and PowerShell syntax passed. Required MS-PL file is byte-identical to the pinned
upstream license and the upstream source diff is empty. The publication checker
passed working-tree/index/history inspection; main repository history is empty.
