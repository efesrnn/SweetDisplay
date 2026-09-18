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

## 2026-09-14 — Deployment planning only

- VERIFIED: two driver/monitor INF-CAT packages plus separate software-device EXE;
  exact six-file manifest and SHA-256 values recorded in private evidence.
- SignTool 10.0.26100.6584 /pa /v: both catalogs, driver DLL and enumerator EXE fail
  with No signature found (exit 1). Three /c membership checks find the relevant
  catalog entries but cannot verify trust because the catalogs are unsigned.
- INF and PE imports establish vendor UMDF-only code, using Microsoft inbox kernel
  components. Three inbox SYS files pass /a /kp; IddCx and two local Debug CRT DLLs
  pass /a /pa. The separate EXE requires Debug CRT; it was not executed.
- Read-only current OS and active Code Integrity observations stored privately;
  no host fingerprint reintroduced into public source documentation.
- Updated Turkish DEPLOYMENT_WINDOWS with PnP versus binary-signing distinctions,
  official-source A-D comparison, account/cost constraints and attestation plan.
  Direct commercial user-mode signing is distinguished from obsolete kernel cross-signing;
  this IDD's exact acceptance for that alternative is not claimed verified.
- No rebuild, package mutation, certificate generation/import, store change,
  PnPUtil, driver installation, device enumeration, boot/security change or phone action.
- Next: owner reviews plan; account/EV eligibility, submission acceptance and actual
  signed-package runtime success remain unverified. Stop before deployment.

## 2026-09-14 — Development signing strategy correction (planning only)

- Corrected the earlier attestation-first recommendation: Microsoft distinguishes
  UMDF/PnP package trust from the kernel TESTSIGNING/Secure Boot restrictions.
- Proposed minimum local development configuration: trust one short-lived local
  signing certificate, sign the staging DLL and both catalogs; retain Secure Boot
  and HVCI ON, TESTSIGNING OFF. This is a documented-policy inference, not a verified
  installation/runtime result for this particular IddCx package.
- Prepared certificate, signing, trust-store, package/device installation commands
  and exact-identity rollback in DEPLOYMENT_WINDOWS; release signing is separate.
- No commands from those examples were executed. No certificate, package, device,
  boot configuration, security setting or phone state was changed.
- Attestation/WHQL retained only as future release work. Await explicit approval
  before the proposed local development signing and installation experiment.
- Validation: all nine documented PowerShell blocks parsed successfully without
  executing their contents; all six original package hashes remain unchanged.
  Publication checker passed 63 working-tree files, index and 50 reachable history
  blobs; git diff --check passed. Only deployment/status/test-log documentation changed.

## 2026-09-15 — Approved development installation: stopped during preflight

The owner approved local development certificate trust, signing and installation
of the two SweetDisplay packages, with Secure Boot/HVCI ON and TESTSIGNING OFF,
no boot changes, and an explicit stop-on-failure instruction. Approval remains
valid for that scope; no new signing certificate or driver package was created.

Commands below use repository-relative paths. Output containing a personal path
is summarized with environment-variable notation rather than published verbatim.
These were read-only inventory commands, followed only by documentation updates.

| Command / expression | Observed result |
|---|---|
| Get-Content -LiteralPath 'SweetDisplay\AGENTS.md' | Read operating constraints successfully |
| Get-Content -LiteralPath 'SweetDisplay\STATUS.md' | Read last verified milestones successfully |
| Get-Content -LiteralPath 'SweetDisplay\docs\DEPLOYMENT_WINDOWS.md' -Raw | Read approved plan and rollback successfully |
| Get-ChildItem -LiteralPath 'SweetDisplay\.local' -File \| Select-Object Name | Private operator-notes file listed; no signing-state file listed |
| [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator) | False |
| Test-Path -LiteralPath 'D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe' | False; previous EWDK location unavailable |
| Repeat WindowsPrincipal administrator check with require_escalated execution | Administrator=False; sandbox escalation is not Windows UAC elevation |
| Get-PSDrive -PSProvider FileSystem \| Select-Object Name, Root | C: and G: filesystem drives; Temp rooted in the user's temporary directory |
| Get-Command signtool.exe, Inf2Cat.exe -ErrorAction SilentlyContinue \| Select-Object Name, Source | Neither tool resolved through command lookup |
| Get-ChildItem -LiteralPath 'C:\Program Files (x86)\Windows Kits\10\bin' -Filter signtool.exe -File -Recurse -ErrorAction SilentlyContinue \| Select-Object -ExpandProperty FullName | SignTool found in SDK 10.0.26100.0 arm64, x64 and x86 subdirectories |
| Same Get-ChildItem SDK search with -Filter Inf2Cat.exe | No result in that location; not proof the file is absent everywhere |
| Get-ChildItem -LiteralPath '.' -File -Filter '*.iso' \| Select-Object Name, Length | No root ISO shown |
| Get-ChildItem -LiteralPath 'SweetDisplay' -File -Filter '*.iso' \| Select-Object Name, Length | EWDK ISO entry listed, formatted name truncated; image not mounted by this attempt |
| Get-ChildItem -LiteralPath '.' -File -Filter '*.iso' \| Select-Object -ExpandProperty FullName | No root ISO shown |
| Test-Path -LiteralPath 'G:\Program Files\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe' | Access denied, followed by False; STOP condition |
| Get-ChildItem -LiteralPath 'SweetDisplay\.local' -Filter '*sign*' -File \| Select-Object Name | No result; same already-dispatched read-only batch |
| Get-Content -LiteralPath 'SweetDisplay\windows\driver\SweetDisplayDevice\main.cpp' -ErrorAction SilentlyContinue | Helper source read successfully in the same batch; helper was not executed |

Exact reported failure:

~~~text
Access to the path 'G:\Program Files\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe' is denied.
~~~

No numeric Windows error code was emitted by this tool result. The enclosing
PowerShell process returned exit 0 because Test-Path emitted a non-terminating
error; this is not a successful access check. No elevated UAC process was started.
This is a preflight environment/access failure, not a driver-signature rejection
and not evidence that Secure Boot or HVCI prevents the UMDF driver from loading.

Not executed: New-SelfSignedCertificate, SignTool sign/verify, Inf2Cat,
Import-Certificate, any certificate-store write, any PnPUtil command,
SweetDisplayDevice, device installation, Device Manager/Settings checks or
extension/mode activation. Thus package trust, Driver Store presence, display name,
2400x1080@60 mode availability and extension have no new runtime result.

No boot configuration, Secure Boot, HVCI, signature enforcement or phone state
was changed. Active protection flags were not re-queried in this attempt.
Only STATUS.md, TEST_LOG.md and the deployment document's dated attempt note
were updated after stopping. No build/source/package edits were made.

Post-stop documentation checks:
- git -C SweetDisplay diff --check: exit 0; only line-ending conversion warnings.
- & (Get-Command pwsh).Source -NoProfile -File 'SweetDisplay\scripts\windows\Test-PublicRepository.ps1':
  exit 0, PASS; 63 publishable working-tree files, index and 50 reachable history blobs.
- Documentation writes used Get-Content/String.Replace/WriteAllText and Add-Content
  on the three named Markdown files only. Deployment commands remain unexecuted.

## 2026-09-15 — D: preflight retry PASSED

User corrected the previous G: probe: G: is not the EWDK volume. Rechecked only
verified D: tool locations. Existing local-development approval remains in force.

| Command / check | Result |
|---|---|
| Get-Item on D: Inf2Cat x86 and SignTool x64/x86 | All three exist; SignTool 10.0.26100.6584 |
| WindowsPrincipal.IsInRole(Administrator), initial process | False; expected UAC elevation needed |
| Start-Process powershell.exe -NoProfile -File .local/development-preflight.ps1 -Verb RunAs -WindowStyle Hidden | Elevated preflight exit 0 |
| WindowsPrincipal.IsInRole(Administrator), elevated process | True |
| Confirm-SecureBootUEFI | True |
| NtQuerySystemInformation(103), active CI flags | 0xF401; enforcement ON, TESTSIGNING OFF, HVCI ON, UMCI OFF |
| Get-ItemPropertyValue CI/State HVCIEnabled | 1 |
| bcdedit.exe /enum '{current}' | Exit 0; no test-signing or no-integrity-check override; read only |
| Signing state and CurrentUser/My project certificate lookup | No prior signing state/certificate |
| Get-WindowsDriver -Online, exact SweetDisplay provider/INF filter | No existing SweetDisplay packages |
| Get-PnpDevice, exact project instance-prefix filter | No existing SweetDisplay devices |
| Get-FileHash -Algorithm SHA256 against prior manifest | All six original package hashes match |

Exact executed read-only script, per-command result ledger and transcript are in
ignored .local/development-preflight.ps1, development-preflight-result.json and
development-preflight-transcript.txt. Public results omit host BCD identifiers.
Private install script prepared and parsed; no syntax errors. No boot/security
writes or phone commands. Next: previously approved signing/trust/two-package staging.

## 2026-09-15 — Development signing and Driver Store staging VERIFIED

Elevated .local/development-install.ps1 completed with exit 0. Fresh security
preflight passed again before any mutation. One 90-day nonexportable local
code-signing key/certificate was created in CurrentUser/My; only its public
certificate was exported and trusted in LocalMachine/Root and TrustedPublisher.
SweetDisplay DLL and both catalogs were signed in a separate ignored staging
copy. Original build files and standalone helper EXE were not signed or changed.

SignTool sign /fd SHA256 /s My /sha1 $thumb: DLL and both catalogs exit 0.
Inf2Cat /driver:$driverStage /os:10_GE_X64 /uselocaltime: exit 0.
Inf2Cat /driver:$monitorStage /os:10_GE_X64 /uselocaltime: exit 0.
Get-Item of exact certificate thumbprint in all three stores: identity verified.
SignTool verify /pa /v: DLL and both catalogs exit 0.
SignTool verify /pa /v /c: driver INF, driver DLL and monitor INF exit 0.
pnputil.exe /add-driver $driverInf: exit 0; published name oem80.inf.
pnputil.exe /add-driver $monitorInf: exit 0; published name oem81.inf.
Get-WindowsDriver -Online with exact provider/INF filtering: exactly two packages.
Signed package hashes saved privately. Helper/device runtime not tested at this checkpoint.

Every executed PowerShell step and native command/argument/output/exit code is
recorded in ignored .local/development-install-result.json; script and transcript
are adjacent. Thumbprint, staging identifier and machine paths stay private.
No boot/security enforcement settings or phone state changed.

## 2026-09-15 — Installed-file verification and device checkpoint; STOPPED

Installed Driver Store CAT/INF/DLL checks all passed via SignTool; trust in all
three certificate stores was rechecked. The documented helper was started by
an elevated launcher, with stdout/stderr redirected to ignored local logs.
The filtered snapshot contained a Monitor named SweetDisplay AMOLED, Status OK,
ProblemCode 0. That snapshot does not separately verify the Display adapter:
the subsequent broader Display/Monitor query was denied and work stopped.

### Executed native command ledger

Symbols below preserve repository privacy: $repo is the repository root,
$stage the unique ignored signing directory, and $thumb the recorded local
certificate thumbprint. Exact expanded arguments and complete native outputs
are preserved in .local/development-install-result.json and development-device-result.json.

| Phase | Command with local identifiers substituted | Exit |
|---|---|---:|
| Signing/staging | C:\WINDOWS\System32\WindowsPowerShell\v1.0\powershell.exe "-NoProfile" "-File" "$repo\.local\development-preflight.ps1" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "sign" "/fd" "SHA256" "/s" "My" "/sha1" "$thumb" "$stage\driver\SweetDisplayDriver.dll" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe "/driver:$stage\driver" "/os:10_GE_X64" "/uselocaltime" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe "/driver:$stage\monitor" "/os:10_GE_X64" "/uselocaltime" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "sign" "/fd" "SHA256" "/s" "My" "/sha1" "$thumb" "$stage\driver\sweetdisplaydriver.cat" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "sign" "/fd" "SHA256" "/s" "My" "/sha1" "$thumb" "$stage\monitor\sweetdisplaymonitor.cat" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "$stage\driver\SweetDisplayDriver.dll" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "$stage\driver\sweetdisplaydriver.cat" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "$stage\monitor\sweetdisplaymonitor.cat" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "/c" "$stage\driver\sweetdisplaydriver.cat" "$stage\driver\SweetDisplayDriver.inf" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "/c" "$stage\driver\sweetdisplaydriver.cat" "$stage\driver\SweetDisplayDriver.dll" | 0 |
| Signing/staging | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "/c" "$stage\monitor\sweetdisplaymonitor.cat" "$stage\monitor\SweetDisplayMonitor.inf" | 0 |
| Signing/staging | pnputil.exe "/add-driver" "$stage\driver\SweetDisplayDriver.inf" | 0 |
| Signing/staging | pnputil.exe "/add-driver" "$stage\monitor\SweetDisplayMonitor.inf" | 0 |
| Installed-file/device | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "$DriverStoreDriver\sweetdisplaydriver.cat" | 0 |
| Installed-file/device | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "/c" "$DriverStoreDriver\sweetdisplaydriver.cat" "$DriverStoreDriver\sweetdisplaydriver.inf" | 0 |
| Installed-file/device | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "$DriverStoreDriver\SweetDisplayDriver.dll" | 0 |
| Installed-file/device | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "/c" "$DriverStoreDriver\sweetdisplaydriver.cat" "$DriverStoreDriver\SweetDisplayDriver.dll" | 0 |
| Installed-file/device | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "$DriverStoreMonitor\sweetdisplaymonitor.cat" | 0 |
| Installed-file/device | D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe "verify" "/pa" "/v" "/c" "$DriverStoreMonitor\sweetdisplaymonitor.cat" "$DriverStoreMonitor\sweetdisplaymonitor.inf" | 0 |

### PowerShell step results

Exact commands are retained in each named step of the private JSON ledgers and
the adjacent executed .ps1 files. These include New-SelfSignedCertificate,
Export-Certificate, Copy-Item, Import-Certificate, exact-thumbprint Get-Item,
Get-FileHash, Get-WindowsDriver, Start-Process and PnP property queries.

| Phase | Step | Result |
|---|---|---|
| Signing/staging | Fresh read-only preflight | PASS |
| Signing/staging | Create local code-signing certificate | PASS |
| Signing/staging | Copy exact package inputs to staging | PASS |
| Signing/staging | Sign SweetDisplay DLL | PASS |
| Signing/staging | Generate driver catalog | PASS |
| Signing/staging | Generate monitor catalog | PASS |
| Signing/staging | Sign driver catalog | PASS |
| Signing/staging | Sign monitor catalog | PASS |
| Signing/staging | Trust local development certificate in Root | PASS |
| Signing/staging | Trust local development certificate in TrustedPublisher | PASS |
| Signing/staging | Verify exact certificate stores | PASS |
| Signing/staging | Verify DLL signature | PASS |
| Signing/staging | Verify driver catalog signature | PASS |
| Signing/staging | Verify monitor catalog signature | PASS |
| Signing/staging | Verify driver INF catalog membership | PASS |
| Signing/staging | Verify driver DLL catalog membership | PASS |
| Signing/staging | Verify monitor INF catalog membership | PASS |
| Signing/staging | Save signed package hashes | PASS |
| Signing/staging | Stage SweetDisplay driver package | PASS |
| Signing/staging | Stage SweetDisplay monitor package | PASS |
| Signing/staging | Verify both Driver Store packages | PASS |
| Installed-file/device | Verify installed catalog signatures and contents | PASS |
| Installed-file/device | Post-install certificate trust | PASS |
| Installed-file/device | Start documented software-device helper | PASS |
| Installed-file/device | Check prototype device status and problem codes | PASS |

### UI and final failed read

- Computer-use SKILL.md, guidance.md, api.md and confirmations.md read before UI work.
- Imported @oai/sky; sky.list_apps located Windows Settings and the running helper.
- sky.launch_app for the returned Windows Settings app ID; sky.list_apps confirmed
  one Settings window. No Display-page state or screenshot was read yet.
- Read private device-status JSON: SweetDisplay AMOLED / Monitor / OK / ProblemCode 0.
- Read helper stdout/stderr files: empty at that time; no callback text claimed verified.
- Additional command: Get-PnpDevice -Class Display,Monitor with SweetDisplay name/ID
  filtering, followed by Get-PnpDeviceProperty DEVPKEY_Device_ProblemCode.
- Exact failure: Get-PnpDevice: Access denied. Enclosing shell exit 1; no numeric
  native Windows error code returned. ErrorActionPreference=Stop prevented the later
  statements in that batch. No retry in a different execution context was made.

The failure belongs to an additional non-elevated read, not SignTool, PnPUtil,
the elevated device-launch phase or a driver-reported problem code. No conclusion
about a signature-policy rejection follows from it. Stop-on-first-error honored.

Remaining: separately verify the Display adapter, Settings display name, available
2400x1080 @ 60 Hz mode and actual desktop extension. No runtime success claimed
for those gates. The helper was left running; oem80.inf/oem81.inf and the local
certificate/trust entries remain installed. No rollback was executed.

Secure Boot/HVCI ON, TESTSIGNING OFF and normal enforcement ON were verified before
mutation; no command changed them. No boot writes, enforcement weakening, firmware
operations, phone commands or public signing submission occurred.

Post-stop documentation validation: git -C SweetDisplay diff --check exited 0.
The publication checker was run with pwsh -NoProfile -File
scripts/windows/Test-PublicRepository.ps1; execution result recorded below.
Publication checker: exit 0, PASS; 63 working-tree files, index and 50 reachable history blobs. Ignored signing material and raw ledgers were not published.

## PHASE 1 — source inspection and diagnostic build

VERIFIED: the original RunCore acquired IddCx surfaces but its processing section
was a TODO; it released each surface without frame evidence. Added opt-in ETW
metadata and one-shot staging readback inside that exact successful acquire path.
The driver refuses protected surfaces; no file-system ACL or IPC interface was added.
PHASE 2 has not begun. ETW payload is diagnostic-only, not the future phone protocol.

Build-FrameProbe.cmd D: via the WDK application project: exit 0, no warnings/errors.
Build-FrameDiagnostics.cmd D: driver/API/signability/catalog checks exit 0, no
warnings/errors. Initial direct-cl probe could not locate windows.h; fixed by
using the established WDK MSBuild toolset. Initial Inf2Cat rejected the local-date
DriverVer while UTC was still the prior day (22.9.7/MSB6006); fixed with the WDK
Inf2CatUseLocalTime option, with no clock or security change.

Fresh baseline inspection: helper absent; old Display/Monitor nodes non-present;
only the two physical paths active. Secure Boot/HVCI remained enabled. This explains
why the previously operational Display 3 was not active at the beginning of this run.

Explicit source-change deployment: reused the existing certificate and trust,
preserved the baseline signed package and signing ledger, signed only the changed
DLL and driver CAT. All four SignTool checks passed. PnPUtil /add-driver /install
updated only the changed driver: oem82.inf. Baseline oem80.inf retained; monitor
package oem81.inf unchanged. Existing helper started; both present device classes
Display and Monitor have Status OK / ProblemCode 0. Source-update execution exit 0.

QueryDisplayConfig now shows the active SWT0001 indirect target at 2400x1080, 60/1,
position 4480,0. Its current GDI name is DISPLAY11; GDI numbering is not the Settings
screen number. The probe selects the unique SWT0001 indirect target, not a guessed
GDI number. Private topology files contain exact OS instance and adapter identifiers.
Exact source-update commands/output: .local/phase1-update.ps1 and phase1-update-result.json.
No phone work, boot changes, new certificates or trust-store modifications.

## PHASE 1 — real frame reception VERIFIED

Scope: PHASE 1 only. No SweetDisplayHost, encoder, continuous transport, simulator,
touch or phone daemon implementation was started. Owner's working Display 3
baseline was respected; the source change was solely diagnostic instrumentation.

Executed capture sequence (exact private arguments/results in phase1/execution.json):
- Elevated powershell.exe -NoProfile -File .local/phase1-capture.ps1: exit 0.
- SweetDisplayFrameProbe.exe --pattern evidenceDir hexNonce: real Win32 pattern
  window created, then moved via SetWindowPos to the queried SWT0001 source rectangle.
- Waited three seconds; pattern process remained running.
- SweetDisplayFrameProbe.exe --capture evidenceDir hexNonce 20: exit 0.
- ETW StartTrace/OpenTrace/EnableTraceEx2/ProcessTrace/ControlTrace used only the
  project's diagnostic provider; enable/stop/ProcessTrace exit codes all 0.
- Get-CimInstance Win32_Process for ETW producer: WUDFHost.exe confirmed.
- Confirm-SecureBootUEFI=True; HVCIEnabled=1 after capture. Active CI pre-update
  flags confirmed TESTSIGNING OFF and normal enforcement ON; no policy writes.
- CloseMainWindow on the test-pattern process returned True. No driver/helper
  shutdown, security change or phone operation was part of that cleanup.

Measured results:
- 771 frames, IDs 1 through 771, 19.996027 seconds, 38.507650 FPS.
- Every frame: 2400x1080, DXGI_FORMAT_B8G8R8A8_UNORM (87).
- Acquisition QPC, QPC frequency, present QPC, frame ID, presentation-frame number,
  dimensions, format, interval and producer PID logged for every frame.
- IDs and acquisition QPC strictly increasing; ID gaps 0.
- Presentation-frame numbers 269 through 1039.
- Frame interval mean 25.968866 ms; minimum 8.1247 ms; maximum 55.4893 ms.
- ETW events lost 0; real-time buffers lost 0; no driver diagnostic error.
- Exactly one diagnostic-frame.bmp, 10,368,054 bytes. Frame ID 1; paint tick 117.
- Pixel nonce matches the independently drawn test nonce; pixel tick matches
  pattern.csv tick 117. Whole-image chunk hash verified before BMP write.
- BMP SHA-256 and full numeric checks recorded in private verification.json.
- Visual inspection confirmed the expected color bands, moving magenta rectangle,
  test code and tick 117. The BMP viewer hit a base64 transport error; decoded
  the same BMP into an in-memory PNG for inspection, without another image file
  or another capture. Exactly one diagnostic image remains on disk.

Identity verification: QueryDisplayConfig's unique active SWT0001 indirect target
was at 2400x1080, refresh 60/1, separate desktop position. At capture time its GDI
name was DISPLAY11. Windows Settings numbers do not equal GDI suffixes. The owner
had confirmed this SweetDisplay target as Display 3. Probe selection was by the
monitor hardware identity and output type; it did not assume GDI DISPLAY3.
The dumped pixels entered only through the driver's acquired IddCx surface,
not a screen-grab API or direct pattern-to-consumer shortcut.

Additional UI attempt: Windows Settings was selected using computer-use APIs.
The first accessibility result was null; screenshot/activation showed an older
advanced-display list. On resumption, the Settings window was reopened but user
interaction/minimization prevented a fresh label check. No display setting was
changed. This UI observation was not used as proof of the current label.
Final native inventory still contained the active SweetDisplay target; physical
screen geometry matched the pre-update inventory. No second frame was captured.

Limit: 60 Hz mode is VERIFIED; sustained 60 FPS capture/encoding is NOT YET TESTED.
The measured 38.51 FPS is this diagnostic run, not a throughput limit diagnosis.
All underlying command ledgers, topology and images are ignored local evidence.
PHASE 1 methodology and official API references: docs/FRAME_RECEPTION.md.
PHASE 2 remains NOT YET TESTED and has not begun.

Final PHASE 1 checks: git diff --check exit 0; public-repository checker exit 0
(70 publishable files, index, 50 reachable history blobs). One unchanged BMP
confirmed by SHA-256; STATUS vocabulary is exactly the requested set. No commit,
push, second capture, signing change or PHASE 2 action during final verification.

## 2026-09-15 — PHASE 2 architecture/build/initial GPU test

- VERIFIED: architecture documented in FRAME_HANDOFF.md before coding.
- VERIFIED: existing PHASE 1 rollback artifact signatures; baseline PnP problem 0
  for both devices and active SWT0001 2400x1080@60. Existing helper relaunched.
- Initial build attempt failed with MSB6001 (duplicate PATH/Path in child process
  environment). Normalizing that child environment fixed the invocation; no system
  environment or security configuration was changed.
- VERIFIED: Host Release x64 and driver Debug x64 build; 2,033 production metadata
  and bounded-queue checks pass. InfVerif /u, Inf2Cat and Universal API validation pass.
- VERIFIED: existing development certificate reused to sign only changed DLL/CAT;
  SignTool /pa plus catalog membership checks pass. Driver updated to oem83.inf;
  monitor remains oem81.inf; original/PHASE 1 packages retained. Both PnP problem 0.
- VERIFIED: 8.001657-second smoke run delivered 314 real BGRA8 2400x1080 frames,
  39.241872 FPS; all 314 pattern nonces match, 309 sample changes. IDs/QPC ordered,
  zero drops/invalid/busy, high-water 1. Receive-metadata age avg 1.427867 ms.
- PHASE 2 full acceptance: NOT YET TESTED until long run/reconnect/security checks.
- Exact commands and raw outputs: ignored .local/phase2-*.ps1/result.json and
  docs/evidence/private/phase2/{baseline,smoke,build-1.txt,build-2.txt,infverif.txt,apivalidator.txt}.

## 2026-09-15 — PHASE 2 long-run data and interrupted test automation

- VERIFIED: acceptance-3 raw data independently rechecked against its SWT0001
  pattern log. Session 1: 35.0099536 s, source 1,360, received 1,355,
  38.703279 FPS, contention drops 5, high-water 2. Session 2: 35.0032315 s,
  source 1,355, received 1,352, 38.625005 FPS, contention drops 2 and stale
  Host-queue drop 1, high-water 3. Both runs: invalid/busy 0, correct BGRA8
  2400x1080 dimensions, ordered IDs/QPC, exact source/drop accounting and every
  sampled nonce/counter correlated with the pattern. Same source epoch across
  clean Host reconnect. Private partial-verification.json records these checks.
- The enclosing test runner did not complete: an initial Start-Process exit-code
  value was empty; subsequent runs stalled while serializing decorated
  Get-Content stdout objects. Native exit-code handling and plain File.ReadAllText
  ledger strings are now used. No driver reinstallation was involved in these
  harness corrections. Prior incomplete execution records remain evidence of
  incomplete tests and are not promoted to PASS.
- The session was interrupted by the tool approval-review usage limit. After
  resuming, no hung test runner remained. A new acceptance-4 elevation request
  was canceled by Windows with "Operation canceled by the user"; the script did
  not start and no acceptance-4 data was created. The existing helper remained.
- NOT YET TESTED: deliberate slow-consumer run, GPU-lease crash cleanup,
  post-crash reconnect and final security/PnP/failure-event audit. Full PHASE 2
  is not VERIFIED until these finish. Awaiting the owner to accept a fresh UAC
  prompt; no certificate, boot, phone or installed-package changes are needed.

## 2026-09-15 — PHASE 2 VERIFIED, final acceptance

Owner approved reopening UAC; the completed acceptance-4 run used the existing
helper and installed PHASE 2 driver. No reinstall, resign, trust-store, boot,
HVCI, signature-enforcement or phone change was performed during final testing.

VERIFIED commands/results:

- Elevated Test-FrameHandoff.ps1 -RunName acceptance-4: PASS, native exit 0.
- Verify-FrameHandoffEvidence.ps1 against that directory: PASS. The independent
  verifier checked every received frame's geometry/flags, ordered IDs/QPC,
  actual pattern counter correlation, source/drop accounting and reconnect epoch.
- Normal stream: 35.0008727 s, source 1,359, received 1,357; 38.770462 FPS.
  Contention drops 2; producer replacements, stale drops, busy and invalid all 0;
  high-water 2. Interval average 25.786398 ms, min 8.467900, max 62.117700.
  Metadata receive age avg 1.509300 ms, max 5.191900. Pattern changes 1,335.
- Clean reconnect: 35.0022855 s, source 1,352, received 1,351; 38.597480 FPS.
  Contention drop 1; other loss categories 0; high-water 2. Interval average
  25.903817 ms, min 7.800500, max 62.347500. Metadata receive age average
  1.495693 ms, max 6.899200. Pattern changes 1,316.
- Deliberately slow Host: 12.0941605 s, source 469, received 115, 9.508721 FPS.
  Producer replaced 123; Host discarded 228 stale queued frames. Three frames
  remained ready at measurement end and were retired on disconnect. Accounting:
  469 = 115 + 123 + 228 + 3. High-water exactly 3; held at end, busy, invalid,
  contention all 0. This is an intentional dropping-policy test, not a throughput claim.
- Absent Host: 155 new source frames in four seconds. Forced test-Host exit
  after its GPU-lease-held marker: expected exit 0xFFFFFFFF. The following
  absent-Host check reported connected=0 and 158 new frames in four seconds.
- Fresh process after crash: 461/461 frames in 12.0030273 s, 38.406978 FPS,
  no drops or invalid frames, high-water 2. Driver epoch unchanged across all
  runs and frame IDs increased through reconnect. No reboot needed.
- All received frames: 2400x1080, BGRA8 (87); every nonce matched and each sampled
  paint counter correlated with the SWT0001 pattern log. No full BMP was created.
- Before/after health: both classes OK / PnP problem 0; active SWT0001 2400x1080@60;
  driver package retained, monitor package unchanged. Secure Boot ON, HVCI ON,
  TESTSIGNING OFF, normal CI enforcement ON. Boot timestamp unchanged.
- Failure-event audit: 0 matching WUDFHost/SweetDisplay application crashes or
  selected system driver/bugcheck events in the test interval. A separate bounded
  event query explicitly persisted the empty array; its initial PowerShell 7
  DateTime reparse was corrected to a DateTime cast before the query ran.
- Existing staged DLL signature and catalog/INF membership reverified with
  SignTool /pa /v: exit 0 each. The original driver-only update also verified
  DLL catalog membership; existing certificate/trust was reused throughout.
- Final Host-only build added total-drop/contention fields to the console summary
  using counters already verified in JSON. Release x64 build succeeded; frame
  transfer code and installed driver were unchanged by this final reporting edit.
  Build-FrameHandoff.cmd D: host-only is available for independent Host iteration.
- Shared production metadata/queue executable: PASS, 2,033 checks. Parser checks
  and final public repository hygiene run complete the software verification.

VERIFIED rate observation: test pattern paints ~39.83/39.84 per second; acquisition
38.83/38.63, delivery 38.77/38.60, retained acquisition fraction 99.85%/99.93%.
SUSPECTED: timer/DwmFlush/desktop scheduling limits this particular changed-content
workload. Sustained 60 FPS capability remains NOT YET TESTED; 60 Hz mode alone
is not evidence of it. Metadata-receive age excludes the subsequent GPU mutex
wait and sample Map and is not an encoding or end-to-end display latency.

Private evidence: acceptance-4 execution.json, verification.json, rate-observation,
per-session CSV/JSON, pattern log, health/topology and failure-events.json; parent
build/signature/rollback logs. Incomplete acceptance-1/2/3 logs remain incomplete
historical attempts. Exactly the original PHASE 1 BMP remains. Public docs contain
summary findings; local identifiers and raw logs remain ignored. No push occurred.

PHASE 2 is VERIFIED and work stops here. No H.264, USB, simulator, touch, camera
or phone implementation was started. Future work needs a separate instruction.

## 2026-09-16 — PHASE 3A initial capacity experiments (PARTIAL)

Owner authorized PC-only capacity/soak, then later encoding/protocol/simulator
subject to measured gates. Known-good Host sources/binary and signed driver
package copied/hashed into ignored phase3/baseline. Driver source, installed
packages, signing/trust and Windows boot/security were not modified.

Commands/results, with raw arguments/results retained in each private execution.json:

- Build-Phase3Tools.cmd D: builds Release x64 Host, GPU pattern and metadata probe.
  Builds 1–5 succeeded. It does not build, sign or install the driver.
- Test-Phase3Capacity.ps1 -RunName gpu-capacity-1 -Seconds 60:
  FAIL before streaming: pattern GetBuffer(1) -> DXGI_ERROR_INVALID_CALL,
  code 2289696769 / 0x887A0001. D3D11 buffer-zero view fixed in test tool only.
- gpu-capacity-2, 60 seconds, DXGI-paced Present(1,0): source 3,394, received
  3,337 / 60.013459 s = 55.604194 FPS. One duplicate-content frame; distinct
  pattern rate 55.587531. 57 Host stale drops, no other drops, queue peak 3.
  Source interval p50 16.72, p95 33.04, p99 34.12, max 43.2447 ms (rounded
  percentiles; exact distributions in verification.json). Pattern about 132.84/s.
- First gpu-capacity-3 launch: UAC cancelled, Windows reported 'operation was
  cancelled by the user'; the test did not start. The owner explicitly approved
  reopening UAC. Successful retry: QPC-limited pattern 60.000/s, received 2,808 /
  60.0049358 s = 46.796150 FPS, source 48.246031 FPS, 87 Host stale drops,
  other drop categories 0, peak 3. Every received pattern counter was distinct.
  Source interval p50 16.8656, p95 34.7535, p99 49.9535, max 50.7189 ms.
- Test-Phase3PacingMatrix.ps1 ran three bounded experiments, each with 15 seconds
  without a connected Host, 30 seconds streaming and 10-second fresh reconnect:
  - pacing-60-sync0: source 53.019875 FPS, received 1,509 / 30.0076148 s =
    50.287236 FPS, 81 stale drops, one pending, peak 2. No-Host source 54.599296.
  - pacing-120-sync0: source 56.423525 FPS, received 1,619 / 30.0052151 s =
    53.957287 FPS, 73 stale drops, one pending, peak 3. No-Host source 56.530831.
  - pacing-120-no-sample: source 55.459988 FPS, received 1,602 / 30.0036129 s =
    53.393570 FPS, 61 stale drops, one pending, peak 2. No-Host source 55.397324.
    This last row intentionally disables all pixel sampling; it is a timing
    control, not independent pixel-content validation.
- Verify-Phase3Capacity.ps1 passed each completed successful experiment. Checks:
  source/drop/pending accounting, per-frame 2400x1080 BGRA8, ordered IDs/QPC,
  bounded queues, same driver epoch across reconnect, all health/security samples
  and no selected failure events. Sampled runs additionally correlate every nonce
  and nondecreasing counter to the actual SWT0001 GPU pattern log.
- FrameProbe --metadata adds ETW keyword 1 only, explicitly excluding the capture
  keyword; capture events are rejected by the metadata consumer. No BMP written.
  Before soak: 844 frames / 14.999457 s (first-to-last), 56.202034 FPS, zero ETW
  loss and zero presentation-number gaps. Target presentation intervals average
  17.793594 ms, range 16.6666–50.0 ms. Acquire precedes the target by 6.747976 ms
  on average, with worst late acquisition 0.3565 ms. These are planned display
  times, not measurements of arrival before acquisition.

VERIFIED: the old ~38.5 FPS run was not an established throughput ceiling; a
changed workload delivers 55.60 FPS without changing the driver architecture.
The no-Host and metadata controls show an upstream presentation/acquisition
cadence shortfall before shared GPU transfer, plus separate Host stale drops.
SUSPECTED: exact desktop scheduling cause. 60 FPS is NOT YET VERIFIED. The small
sample's completion interval includes synchronization and Map, not isolated GPU
copy time. No synthetic duplicate frames are added to any throughput count.

Soak command started: Test-Phase3Capacity.ps1 -RunName soak-1 -Seconds 1810
-PatternFps 0 -SyncInterval 1 -InspectBefore 15 -MetadataSeconds 15.
Outcome pending. Resources every ~5 seconds, queue/state telemetry each second,
health every ~30 seconds; fresh-process reconnect follows. No soak success is
claimed before final evidence review. PHASE 3B–3F have not begun.

Additional PHASE 3A verification while soak continues:

- Existing FrameHandoffTests.exe: PASS, 2,033 production metadata/queue checks.
- Existing MonitorConfigTests.exe: PASS, EDID identity/checksum/DTD, exact 60 Hz,
  aspect ratios and mode-buffer bounds.
- Verify-Phase3Capacity.Tests.ps1 against copies of completed gpu-capacity-3:
  PASS, 13 acceptance/rejection checks. Original evidence untouched. Tests reject
  duplicate IDs, regressing QPC, wrong counter/nonce/geometry, wrong gap accounting,
  unbounded queue, unaccounted source count, changed test-signing bit, short soak,
  failed execution and forced pattern shutdown. Fixtures remain under ignored out/.
- Four new PowerShell scripts parse successfully.
- Public repository check: PASS, 88 publishable working-tree files, index and
  50 reachable history blobs. This is a pattern check, not a publication guarantee.
- SHA256 comparison: existing staged driver DLL/INF/CAT and shared ABI header
  exactly match the PHASE 2 backup. Private baseline-preservation.json records
  the comparison; tested-artifact-hashes.json records current Host/tool inputs.
- Metadata-only target-time distribution: 789 intervals of one 60-Hz refresh,
  51 of two refreshes, 3 of three refreshes. Presentation numbers themselves are
  contiguous. These 57 unscheduled refresh slots are not Host queue-drop counts.

## 2026-09-16 — PHASE 3A soak-1 completed duration, acceptance FAIL

Command: Test-Phase3Capacity.ps1 -RunName soak-1 -Seconds 1810 -PatternFps 0
-SyncInterval 1 -InspectBefore 15 -MetadataSeconds 15.

Exact Host result: exit 1, `SweetDisplayHost ERROR: integration validation failed;
inspect report; code=13 (0x0000000D)`. Wrapper records Host exit=1 and stops.
No fresh-process reconnect or later phase was launched. The pattern closed
normally. Host had already disconnected cleanly before final validation failed.

Observed results, not an acceptance pass:

- Runtime 1,810.0019503 s; first-to-last received source timestamp span
  1,809.9833452 s. Source 100,399; received/acknowledged 98,329.
  Source 55.469001 FPS; raw receive 54.325356 FPS; 2400x1080 BGRA8 throughout.
- Exact accounting: 100,399 = 98,329 + 44 producer replacements + 1,996 stale
  Host drops + 30 contention drops. Busy, invalid metadata and pending at end 0;
  queue peak 3. All metadata IDs/QPC ordered; telemetry connection/epoch bounded.
- Source intervals p50 16.7310, p95 33.3764, p99 33.9865, max 162.0360 ms.
  Receive intervals p50 15.9342, p95 31.4779, p99 39.2472, max 163.1443 ms.
  Diagnostic sample-completion intervals p50 16.3180, p95 34.4687, p99 45.6943,
  max 163.2996 ms. Mutex wait p50/p95/p99/max: 0.0478/0.0762/0.1010/0.7612 ms.
  Sample GPU copy/Map mean 4.843282 ms, p95 17.5844, max 162.8368 ms. These
  include synchronization; isolated full-frame GPU-copy timing remains unmeasured.
- Content FAIL: only 98,155/98,329 nonce matches. All 174 mismatches decode
  nonce=0 and counter=0 during 90.7196575–94.3163347 s. At 90.6019812 s a separate
  matching-nonce frame regresses counter 16,148 -> 16,122. Independent forensic
  analysis finds 98,111 distinct matching-nonce counters, 54.204914/s, but does not
  waive the mismatch/regression. Raw hashes vary during most of the bad interval;
  pixel samples were not retained, so dimming/overlay vs another content problem
  cannot be proven. Transient desktop interference is SUSPECTED only.
- After 120 s warm-up, CPU one-core equivalent: Driver 2.84%, Host 1.97%, Pattern
  3.64% (whole-machine logical capacity ~0.09%, 0.06%, 0.11%). Per-process 3D GPU
  engine mean/peak: Driver 3.96%/5%, Host 0.33%/2%, Pattern 9.50%/14%.
- Private-byte end deltas after warm-up: Driver -28,672; Host -57,344; Pattern 0.
  Driver handle range 933–935, sampled end 933; Host 763–767, end 765; Pattern
  769–771, end 769. No continuing resource growth observed over this interval.
  Post-cleanup Driver handles 930, then 928 after settling; private bytes settle
  to 35,172,352. This demonstrates cleanup, not an unlimited-duration leak proof.
- 55 before/periodic elevated health samples: Secure Boot ON, HVCI ON, TESTSIGNING
  OFF and normal CI enforcement ON; both PnP problem codes 0. Read-only post-fail
  queries also show PnP 0/0, same boot, Secure Boot/HVCI registry enabled and zero
  selected WUDFHost/SweetDisplay crash or system driver/bugcheck events.
- Initial post-failure resource inspection from the restricted shell returned
  Access denied. The identical read-only query through tool sandbox escalation
  succeeded without a UAC/configuration change. No security restriction was disabled.

Private evidence: soak-1/execution.json, main CSV/JSON/stdout/stderr, pattern
log, source-metadata, periodic health/resource/GPU CSV, nonce-mismatches.json,
forensic-analysis.json and post-failure health/resource files. The forensic output
is explicitly FAIL and does not substitute for the strict acceptance verifier.
Exactly the original PHASE 1 BMP remains; no additional frame dump was made.

PHASE 3A remains PARTIAL. 60 FPS and a clean-content soak are not VERIFIED.
PHASE 3B/3C/3D/3E are BLOCKED at the owner's gate; encoder/protocol/simulator/touch
were not started. PHASE 3F is PARTIAL only for existing and new 3A checks, not a
completed future pipeline test suite. Suggested next bounded test: fail fast at
first mismatch, retain only the existing tiny diagnostic sample and log desktop
transitions, then repeat. This follow-up was not run; no driver/architecture,
security/trust/boot or phone changes were made to work around the failure.

Final closeout checks: the strict Verify-Phase3Capacity.ps1 with
-MinimumStreamSeconds 1800 rejects soak-1 with 'Execution did not pass', as
required. The 13 positive/negative verifier tests pass again after the final
analysis changes; four script parsers pass. Latest repository hygiene: PASS,
88 publishable working-tree files, index and 50 reachable history blobs. Exactly
one BMP remains. No test processes from this run remain active; the original
device helper/deployment was retained. No commit or push was performed.

## PHASE 3A.1 first-failure diagnostic — PARTIAL

Owner requested first-failure diagnosis before any repeat soak or encoding.
Added opt-in Host failure detection, bounded producer ledger, 120-row history,
one 656×96 crop on the same held texture, desktop transition observation and
benign own-window controls. No driver or shared frame-ABI change was made.
Only Host/tools/tests were built; no deployment/signing/install command ran.

Commands and results (raw command arguments/output remain private):

- `Build-Phase3Tools.cmd D:`: exit 0, Release x64 Host/pattern/FrameProbe builds.
- `Build-HandoffTests.cmd D:`: exit 0; `FrameHandoffTests.exe`: 2,033 existing
  production metadata/queue checks plus 16 oracle/ledger checks PASS.
- `Verify-Phase3Capacity.Tests.ps1 -EvidenceDirectory <gpu-capacity-2>`:
  13 positive/negative checks PASS. An initial invocation without the required
  EvidenceDirectory parameter was rejected before testing, then corrected.
- UAC launch of `Test-FirstFailDiagnostic.ps1 -RunName controlled-cover-1
  -Seconds 30 -ControlCase cover`: first attempt cancelled before execution.
  After owner's continue request, retry ran and stopped at the first intentional
  content mismatch: Host exit 1, code 13 / 0x0000000D,
  `first content-oracle failure; bounded evidence frozen`.
- Source-relative first mismatch 8.1152188 s, frame 404593, presentation 404842,
  epoch 798362475321, slot 0. 464 rows, only the last invalid; 120-row history.
  Crop is uniformly RGB(48,48,48), matching the controlled grey window. Independent
  BMP decoding/hash checks match the original two rows sampled while holding the
  same keyed mutex. Control begin preceded acquisition by 10.5326 ms.
- `Verify-FirstFailEvidence.ps1 -EvidenceDirectory <controlled-cover-1>
  -QpcFrequency 10000000`: evidence VERIFIED, classification
  CONTROLLED_COMPOSITOR_REPLACEMENT, acceptance still CONTENT_ORACLE_FAILURE,
  CleanPatternShutdown false, HistoricalCause UNKNOWN. QPC frequency was queried
  from the current machine; producer final report is missing after forced cleanup.
- Cleanup defect: Process.CloseMainWindow selected our overlay, leaving our
  pattern running. Runner killed only its own pattern after 5 s. The corrected
  runner posts WM_CLOSE to the recorded producer HWND only after verifying the
  owning PID; also records producer exit status. Correction NOT YET TESTED.
- `controlled-cover-2` retry: Start-Process/RunAs failed with
  `İşlem kullanıcı tarafından iptal edildi.` before the harness ran. No second
  evidence directory or capture was created. Asked owner whether to reopen UAC;
  no automatic permission bypass or security change attempted.

Before/after both PnP problem codes 0, selected failure event count 0, same boot,
Secure Boot/HVCI ON, TESTSIGNING OFF, normal signature enforcement ON. The old
174-frame failure and counter regression remain unresolved. Historical cause
UNKNOWN; controlled old-counter/minimize/foreground tests, 5+5+10-minute observe
sessions and corrected cleanup are NOT YET TESTED. No repeat soak/3B work started.
No live PHASE 2 full regression suite was rerun because no pipeline bug was fixed;
its existing acceptance remains historical, not a new runtime claim.

Evidence, limitations and exact next steps: docs/PHASE3_FIRST_FAIL.md. Raw records
and the single new crop remain under ignored docs/evidence/private/phase3a1.

Closeout: both new PowerShell script parsers PASS. SHA-256 comparisons with this
turn's baseline confirm staged driver DLL/INF/CAT and shared frame ABI unchanged.
Public repository check PASS: 96 publishable working-tree files, index and 50
reachable history blobs. No commit/push. Further runtime work awaits the user's
response to reopening the cancelled UAC prompt.

### Authorized UAC retry — controlled-cover-2

Owner explicitly authorized reopening UAC, the second controlled test and live
cleanup verification. `Test-FirstFailDiagnostic.ps1 -RunName controlled-cover-2
-Seconds 30 -ControlCase cover` ran. Host stopped intentionally with code 13,
exit 1 at the first zero nonce/counter: source-relative 8.039831 s, frame 405938,
presentation 406196, slot 0, epoch 798362475321. 445 received rows, one invalid,
120-row history and exactly one 656×96 crop. Independent evidence verifier and
visual inspection confirm uniform RGB(48,48,48) with identical sample/crop hash.
Control-to-acquisition delay: 17.7856 ms. Clean producer shutdown VERIFIED:
WM_DESTROY recorded, exit 0, no forced kill or cleanup error. Both PnP problems 0,
failure events 0, security/boot unchanged. This proves the harness cleanup fix,
not a fix or explanation for the historical soak failure.

`controlled-snapshot-1` (30-second bound, snapshot control) also stopped at the
first intended failure: nonce unchanged, counter 1403 → 1374, reason flag 4,
frame 406655 / presentation 406918 / slot 0, source-relative 8.1830632 s.
449 rows, one invalid. Independent crop decoding matches the explicitly painted
old counter 1374; control-to-acquisition delay 15.8471 ms. Same-held-texture crop
and row hash match. Producer generation stable; clean shutdown exit 0 and health
checks pass. Classification CONTROLLED_OLD_CONTENT_ON_DESKTOP. This shows genuine
desktop old-content rendering can cause a regression, not that the historical
16148 → 16122 had that cause. No pipeline repair or gate change follows.

`controlled-minimize-1` (30-second bound) stopped on zero nonce/counter at
8.2069614 s, frame 407254 / presentation 407521 / slot 0, 457 received rows.
The single crop is a dark nonuniform gradient; same-held-texture sample hash
matches. Minimized=1 was observed before acquisition, 5.6719 ms after control
begin. Producer exit 0, selected failure events 0 and unchanged health. Automatic
classification remains UNEXPLAINED_CONTENT_FAILURE rather than inferring a
particular background/animation solely from minimized state.

Added `Test-FirstFailShortChecks.ps1` to bound the final series to foreground
30 seconds and observe 300 seconds under one initial UAC. It invokes the existing
harness in separate child processes and stops at the first nonzero exit or
verification failure. `Verify-FirstFailEvidence.ps1` additionally checks complete
session duration, acknowledgment, exact drop accounting and queue telemetry for
the no-anomaly branch; that complete-session branch was not exercised here.

`Test-FirstFailShortChecks.ps1 -RunPrefix short-1` stopped in its first session:
Host exit 1 / code 13 at source-relative 17.354111 s, frame 408355, presentation
408622, slot 1, epoch 798362475321, observed nonce/counter 0/0 after counter 2349.
900 rows, only the last invalid; 921 source = 900 received + 21 Host stale drops
at the captured state. Queue peak 3, producer replacements/busy/invalid 0. The
existing first-fail recorder saved exactly one 656×96 crop and 120-row history.
The five-minute `short-1-observe` directory/session was never created.

The own-window SetForegroundWindow call had returned 0, so this was not a valid
successful foreground-control experiment. The content failure happened about
9.200 seconds later, amid other foreground transitions. No retry or policy
bypass was attempted. At failure the pattern was visible, not minimized/cloaked;
Default desktop and 2400×1080@60 SWT0001 coordinates unchanged. Post-test bounded
process-name lookup identifies the recorded foreground PID as Explorer.

Visual inspection, independent BMP decode and hash verification pass. The full
BMP is byte-for-byte identical to the minimize experiment's BMP even with a fresh
Host/shared-resource generation. The owner then explicitly confirmed performing
Show Desktop at approximately the queried test time. A private review records
DESKTOP_REPLACEMENT_STRONGLY_CORROBORATED with that confirmation. The raw execution
and automatic classifier remain failure; no invalid interval is filtered. This
is strong evidence for actual desktop replacement in the latest incident, not
proof of the old soak's cause. Historical cause remains UNKNOWN; PHASE 3A.1 PARTIAL.

All four new sessions closed the producer with exit 0 and without forced stop.
Both PnP problem codes 0, selected failure events 0, boot/security unchanged;
driver package and frame ABI hashes match baseline. No driver reinstallation,
signing, trust/boot/security change, phone operation or PHASE 3B work occurred.
Four new diagnostic images (one per failed session) remain ignored; no continuous
screen recording or full-frame CPU video path was introduced. No diagnostics
process remains. Extra 300/300/600-second observation and repeat soak NOT YET TESTED.

Final validation after this retry: 3 diagnostic PowerShell parsers PASS; 2,033
production metadata/queue checks and 16 oracle/ledger checks PASS; 13 capacity
verifier fixtures PASS; all 4 new saved first-failure sessions pass independent
evidence-consistency checks while retaining failed content-test outcomes. Public
hygiene PASS: 97 publishable files, index and 50 reachable history blobs. Total
PHASE 3A.1 crops: 5, including the original cover-1; all remain private/ignored.
This retry changed only diagnostic orchestration/evidence-verification scripts
and documentation, not C++ video-path sources or the deployed driver. No commit/push.

## Corrected A–E acceptance — preparation

Owner authorized classified semantics, a five-minute observation and, only after
that passes, a fresh >=1,800-second soak before stopping ahead of PHASE 3B.
Added `--classified` to Host diagnostics: A valid pattern, B independently
corroborated non-pattern desktop rows, C independently corroborated old pattern
content, D independent integrity failure, E unknown. B/C require two exact RGB
matches against live screen-DC reads of the same two 640-pixel rows. No reference
images or normal-path full-frame readback; no baseline hash whitelist or automatic
acceptance from window visibility. Unknown content retains one bounded capture
and stops. Old raw runs and strict verifier remain unchanged.

See docs/CLASSIFIED_ACCEPTANCE.md for sampling/timing limits and supported APIs.
Frame/resource/ack/presentation/accounting invariants remain fatal; bounded
accounting re-read handles existing atomics leading the guarded queue snapshot.
The harness supports classified mode, 1,810-second duration and fresh-process
15-second reconnect; stages stop before the next stage on any failure.

Build-Phase3Tools.cmd D: and Build-HandoffTests.cmd D: exit 0. Tests: 2,033
production checks, 16 prior diagnostic checks and 17 new classification checks
PASS. Three new/changed PowerShell scripts parse. Staged driver DLL/INF/CAT and
unchanged frame ABI SHA-256 match baseline. No driver build/deployment/signing.

First `Test-ClassifiedPhase3.ps1 -Stage Pilot -RunPrefix classified-pilot-1` UAC
launch cancelled with `İşlem kullanıcı tarafından iptal edildi.` before execution.
After owner explicitly asked to reopen it, launch retried with the same unused
prefix. Pilot/observation/soak runtime results follow separately; no pass is
inferred from compilation. No security/phone changes made.

### Classified pilot results — VERIFIED

Authorized UAC retry ran `Test-ClassifiedPhase3.ps1 -Stage Pilot
-RunPrefix classified-pilot-1`; stage exit 0 and PASS. Both independent evidence
verifications passed. Cover: 20.0222568 s, A458/B488/C0/D0/E0, 946 received,
source 56.037639 FPS / Host 47.247421 FPS. Exact 1,122 source = 946 acknowledged
+ 5 producer replacements + 169 Host stale + 2 pending, queue peak 3. No image
was saved because the legitimate grey content was independently corroborated.
Snapshot: A461/B0/C474/D0/E0, 935 received; old content stayed classified C without
resetting the high-water counter. Both producer exits 0, no forced stop, both PnP
problems 0, selected failure events 0, unchanged boot/security. These pilots test
classification and its reference-read overhead, not maximum transport capacity.

Snapshot pilot detail: 20.0136177 s, source 55.612135 FPS / Host 46.718190 FPS.
Exact 1,113 = 935 acknowledged + 9 producer replacements + 167 Host stale +
1 contention + 1 pending; queue peak 3. Independent RGB checks accepted 474 old
content frames as C. Both pilots wrote zero BMPs and retained every frame row.
11 positive/negative classified-evidence fixtures PASS, including rejection of
D/E counts, invalid slot, duplicate ID, wrong frame association, unavailable or
mismatched reference, drop mismatch, forced shutdown and altered security flags.

`Test-ClassifiedPhase3.ps1 -Stage Observation -RunPrefix classified-observation-1`
was launched via Start-Process/RunAs, but UAC returned
`İşlem kullanıcı tarafından iptal edildi.` before script execution. No observation
stage/duration/reconnect results exist and no soak began. User was asked whether
to reopen one UAC session for the remaining authorized sequence.

Prepared `Test-ClassifiedObservationAndSoak.ps1`: only fixed Observation and Soak
stages, no deployment/boot/security/phone actions or arbitrary-command interface.
After a passing 300-second stage, it waits at most five minutes for the calling
agent's resource/evidence review tied to that verification report's SHA-256, then
can run the already-authorized 1,810-second stage without another UAC. No stage
is allowed to skip verification. This controller parses but is NOT YET TESTED.
No currently running diagnostic process; old soak remains UNKNOWN.

Closeout while awaiting UAC retry response: hygiene PASS, 104 publishable files,
index and 50 history blobs. No Host/pattern/consent process remains. No commit or
push; the newly requested observation and soak are still NOT YET TESTED.

### Classified five-minute observation — VERIFIED

Owner reaffirmed the one-UAC sequence. `Test-ClassifiedObservationAndSoak.ps1
-RunPrefix classified-flow-1` started successfully. Observation stage and its
independent verifier PASS. Main: 300.0063274 s; 16,456 source / 16,435 received,
source 54.852176 FPS / Host 54.782178 FPS. A16435/B0/C0/D0/E0; no classified content
transition, no saved images, queue peak 2, pending/held 0. Exact accounting:
16,456 = 16,435 acknowledged + 1 Host stale + 20 contention; producer/busy/invalid 0.

Fresh-process reconnect: 15.001639 s, 824 source / 823 acknowledged, A823 only,
54.860672 Host FPS, one contention drop, peak 2; same epoch, advancing IDs.
Clean producer exit 0, no forced termination; both PnP codes 0, selected failure
events 0, same boot, Secure Boot/HVCI ON, TESTSIGNING OFF. Live attempts to read
the open classification CSV returned a sharing violation; no lock bypass or
test change was made. Complete closed evidence verified successfully afterward.

Resource review after 120-second warmup (33 samples per role): driver private
delta 0 and handles fixed 934; Host private delta -57,344 B, handles 773–777,
ending 773; pattern private delta -28,672 B, handles 788–794, ending 788. All
quarter medians flat, fitted slopes nonpositive. No continuing growth observed.
CPU averages are 3.50% driver / 2.78% Host / 4.24% pattern of one logical processor.
The report's scope remains sampled-region content plus metadata/resource invariants.

PHASE 3A.1 corrected five-minute gate VERIFIED; old soak stays UNKNOWN. Reviewed
verification-report hash will be supplied to the bounded controller to start the
already-authorized fresh 1,810-second soak. PHASE 3B remains prohibited.

## 2026-09-16 — fresh PHASE 3A.2 completed; independent final review VERIFIED

The observation's hash-bound review released the same UAC controller. Its fixed
Soak stage ran with prefix `classified-flow-1-soak`, then completed its 15-second
fresh Host reconnect, producer-window close, health checks and verifier. Controller
outcome is TESTS_PASS_RESOURCE_REVIEW_REQUIRED, stage PASS, execution PASS_CLASSIFIED,
with no controller error. Completion was confirmed from existing artifacts after
the owner's request to inspect the background run. No new soak or supplementary
live test was started. The controller completed at 13:30:49 UTC.

Final review first read controller/stage/execution summaries and listed artifacts.
It recorded a SHA256/length manifest of 130 original observation/soak/controller/
stage files in a new ignored `phase3a1/classified-flow-1-review-1` directory.
The verifier/resource analyzer gained only an optional `-ReportPath` output
destination, allowing independent analysis without replacing original reports.
Acceptance logic was unchanged. Review commands, relative to the repository:

```powershell
$case = 'docs/evidence/private/phase3a1/classified-flow-1-soak-observe'
$review = 'docs/evidence/private/phase3a1/classified-flow-1-review-1'
./scripts/windows/Verify-ClassifiedEvidence.ps1 -EvidenceDirectory $case -MinimumSeconds 1810 -RequireReconnect -ReportPath "$review/independent-verification.json"
./scripts/windows/Measure-ClassifiedResources.ps1 -EvidenceDirectory $case -ReportPath "$review/resource-trends.json"
./docs/evidence/private/phase3a1/summarize-closed-classified.ps1 -Directory $case -ReportPath "$review/timing-review.json"
./scripts/windows/Verify-ClassifiedEvidence.Tests.ps1 -EvidenceDirectory docs/evidence/private/phase3a1/classified-pilot-1-cover
```

Independent verifier PASS_CLASSIFIED; all 11 acceptance/rejection fixtures PASS.
SHA256/length comparison after review: all 130 original files unchanged. Full
test commands, timestamps, exit codes and stdout remain in original private
execution/transcript files; review outputs are separate. A monitoring attempt
to read live frame/pattern CSVs encountered sharing violations; no lock bypass
or change to the running test occurred. Closed-file analysis passed. A later
process query's absent Host/pattern names were expected after clean exit; a
complete process inventory confirmed only the existing device helper remained.

Main soak: **1810.0007551 seconds**, source100898, received/acknowledged100749,
source55.744728 FPS / Host55.662408 FPS; distinct valid pattern counters100745,
55.660198/s. A100749/B0/C0/D0/E0, zero transitions, no unclassified events or
actual integrity failures. Exact accounting: **100898 = 100749 + 29 producer
replacements + 14 Host stale + 106 contention**. Busy/invalid/pending/held all0.
Queue peak3, slots0/1/2 only, maximum slot index2. No saved image. Metadata,
presentation/ID/QPC ordering, texture identity and classification associations pass.

Fresh-process reconnect: **15.0017173 seconds**, source832 / received790,
source55.460317 / Host52.660638 FPS, A790 only. Exact accounting832=790+34producer
replacements+8Hoststale; peak3, no busy/invalid/contention/pending/held. The same
driver epoch continues and source IDs advance beyond the main session. Main and
fresh Host clean disconnect true; all Host/producer exits0. PID-checked producer
window close succeeded without forced termination. No missing completion step.

After120-second warmup, 304 resource samples per role. Driver private delta+4096B,
quarter medians35241984/35241984/35246080/35246080: a small step then plateau;
handles936–938, all quarter medians936, delta0. Host private delta-61440B,
handles774–778/delta-2; pattern private delta-90112B, handles789–797/delta-8.
Host/pattern medians flatten or decline. Driver cleanup handles933, later settled
931 and private35233792B. No continuing memory/handle growth observed. This is
bounded-duration evidence, not a proof against arbitrarily small/future leaks.
CPU averages driver3.2783%, Host2.5154%, pattern3.7197% of one logical processor.

All56 health snapshots: both PnP problem codes0, same boot, SecureBootON/HVCION,
TESTSIGNINGOFF, CIflags62465. Selected failure events0; no observed driver crash/
bugcheck. Staged DLL/INF/CAT and shared ABI hashes match PHASE2 baseline. No
driver deployment, security/boot/certificate change or phone operation occurred.

Detailed timing percentiles, resource ranges and scope are in PHASE3A_RESULTS.md.
PHASE3A.1 and the fresh PHASE3A.2 are VERIFIED; therefore PHASE3A is VERIFIED at
the measured achievable rate. Sustained60FPS is not proven. Historical soak
cause remains UNKNOWN and its original failed-oracle report is preserved.
PHASE3B has not started; stop/report at this gate as requested.

Final `./scripts/windows/Test-PublicRepository.ps1`: PASS, 106 publishable
working-tree files, index and 50 reachable history blobs checked. Raw evidence,
review manifests, local process details and previous crops remain ignored. No
commit/push or history rewrite was performed.

## 2026-09-16 — PHASE 3B implementation/preflight PARTIAL, live test BLOCKED

The owner's latest attached request explicitly starts PHASE 3B only, preserving
the VERIFIED 3A baseline and old UNKNOWN soak. Read STATUS, PHASE3A_RESULTS,
CLASSIFIED_ACCEPTANCE, TEST_LOG, current Host/shared queue code and local rules.
Recorded 560 prior PHASE3/3A.1 evidence file hashes in the new ignored phase3b
baseline, and copied the existing Host source/project/executable and shared ABI.
No old run was rewritten or regenerated. Driver installation stayed unchanged.

Preflight commands/results (raw logs in `phase3b/preflight-1`):

- Existing `SweetDisplayEncoderProbe.exe`: hardware-only enumeration S_OK,
  two AMDh264Encoder registrations and one NVIDIA H.264 MFT registration.
- Read-only Win32_VideoController CIM query: Access denied in the sandbox;
  no configuration change attempted. DXGI enumeration provided adapter facts.
- `Build-EncoderProbe.cmd D:` initially failed missing Windows SDK headers, then
  WRL headers. Explicit EWDK um/shared/ucrt/winrt include/lib paths fixed this;
  final build passes. All failed build logs retained.
- `EncoderDeviceProbe.exe`: same-adapter AMD activation S_OK, async=1,
  D3D11-aware=1, DXGI manager S_OK, 800x360@10 NV12/H264 type negotiation S_OK.
  NVIDIA ActivateObject failed 0x8000FFFF; cause UNKNOWN. Probe redundantly called
  activation ShutdownObject after successful IMFShutdown, returning 0x80004005
  on AMD. This is not a clean-shutdown acceptance result or real encoding proof.
- Host MSBuild attempts failed command-line trailing-quote parsing and duplicate
  PATH/Path environment handling (MSB1008/MSB6001). An isolated direct EWDK compiler
  build avoids these launcher issues without changing machine environment settings.
  Corrected new-source header/type errors; final `Build-EncodeHost.cmd D:` passes
  without warnings. Output is separate from the known-good Host executable.
- `Build-H264Decoder.cmd D:` passes without warnings.
- `Test-H264DecoderMalformed.ps1`: four malformed AU length/truncation cases
  correctly rejected. This is not valid real-bitstream/decode-content proof.
- New harness and independent verifier PowerShell parse checks pass.

Implementation: opt-in Host GPU-native scaling/BGRA->NV12, same-device DXGI
manager, hardware-only MFT, async events, fixed four-surface tracked-sample pool,
four-frame output bound, separate rate/backpressure accounting, H264/AU and
per-frame timing records. GPU-backed sample interface/texture identity is checked
without a full-frame readback. Offline decoder uses a separate Microsoft H264
MFT; verifier requires exact PTS/frame/nonce/counter correspondence and retains
all existing A–E and shared-texture checks. Live behavior NOT YET TESTED.

Prepared first live command:
`Test-HardwareEncoding.ps1 -RunName encode-800-1 -Seconds 20 -Classified
-Width 800 -Height 360 -Rate 10 -Bitrate 2000000`.
Launching this via Start-Process / RunAs / hidden PowerShell was rejected by
automatic approval review BEFORE execution. First reason: prior stop-before-3B
instruction. After re-reading the latest attachment's explicit 3B authorization,
the same bounded request was resubmitted; second reason: attachment not accepted
as trusted authorization. No alternative elevation/workaround was used. Asked
for explicit in-chat PHASE 3B/UAC confirmation; pending at this checkpoint.

Therefore no live input/output FPS, bitstream acceptance, correspondence, sustained
stability, reconnect, clean shutdown or new PnP/security health result is claimed.
PHASE 3B remains PARTIAL with live validation BLOCKED. PHASE 3A remains VERIFIED;
historical soak UNKNOWN. PHASE 3C/transport/simulator/phone work not started.
See PHASE3B_RESULTS.md for current scope, evidence, limitations and pending gates.

PHASE 3B checkpoint preservation check: 560/560 prior evidence files unchanged;
known-good Host executable and shared ABI unchanged; no live-run directory.
`Test-PublicRepository.ps1`: PASS, 116 publishable files, index and 50 reachable
history blobs. No push/release. PHASE 3B live test is still awaiting in-chat approval.

## 2026-09-17 — first real PHASE 3B low-resolution gate VERIFIED

Explicit in-chat PHASE 3B/test-UAC authorization arrived; the bounded RunAs launch
was accepted. `encode-800-1` failed before frame reception because the executable
was one directory deeper than the existing private-evidence guard expects. Error:
`First-fail evidence must be inside repository ignored docs/evidence/private`.
Host exit1, producer exit0, health cleanup passed. Preserve this failed run.
The guard was NOT changed. Build/harness target moved logically to `out/encode-host`
by compiling a separate new output; no old binary/evidence was moved or removed.

`encode-800-2` completed using the corrected location, unchanged security guard,
same driver and real Display3. Main duration20.0017116s; source1105, received1104,
source55.245272 / Host55.195276 FPS. A1104 only; upstream1 Host-stale drop, peak2,
no producer/contention/busy/invalid/pending/held. Encoder input1104=accepted200+
rate-limited904+backpressure0. Output200 access units,9.999144 FPS,5,001,650bytes,
2,000,488.798bit/s,10IDRs/keyframes,encoder queue/pool peak1,invalid0,pending0.
Startup184.4544ms,encode latency average4.475331ms/peak19.0161ms;
conversion completion average3.771711ms/peak16.1131ms (not isolated GPU time).
Selected AMD hardware-only async/D3D11 MFT, DXGI manager, actual NV12 GPU resource
identity and CPUAccessFlags0 verified; no software fallback. Both Host and producer
exit0, encoder drained/Shutdown succeeded, tracked surface leases released.
PnP0/0,SecureBootON,HVCION,TESTSIGNINGOFF,CIflags62465; selected failureevents0.

Independent decode attempt1 failed resolution check; attempt2 retained detailed
metadata: coded buffer800x368, explicit minimum/geometric visible aperture800x360,
offset0. Corrected the offline decoder to validate documented visible aperture
and bounds instead of mistaking macroblock padding for picture height. Previous
failed output files remain untouched. `host/decode-3` decoded all200 actual access
units; `Verify-HardwareEncoding.ps1 -DecodeSubdirectory decode-3` passed exact
PTS/frame/nonce/counter association for all200 decoded frames and byte/drop totals.
This is the actual live bitstream, not a synthetic/offline-only encoding test.
Wrong expected visible size800x400 still rejected; four malformed-AU tests pass.
Reference: Microsoft MF_MT_MINIMUM_DISPLAY_APERTURE / Picture Aspect Ratio docs.

Short-run Host private bytes50,384,896–54,026,240; handles1257–1531 (startup included).
Driver private35,233,792–35,291,136;handles931–934. This20s sample cannot establish
long-run resource stability. Per-process integer GPU counters:3Dmean1.25%,peak2%;
Video Codec0 samples0%, not independent utilization proof at this low load.
Hardware-path claim rests on hardware-only MFT contract plus actual DXGI GPU input
and valid output; no ASIC utilization percentage is inferred from the name alone.

800x360@10 short gate VERIFIED. Next selected progressive mode1280x576@30,
then1920x864@30,2400x1080@30,2400x1080@60 target, each separately verified before
advancing. A fixed one-UAC controller will stop on any failed live/decode/content
gate, then run600s at the highest passing configuration plus fresh-process
15s reconnect. Final resource review still required. PHASE3B remains PARTIAL.

Progressive controller launch: Windows consent ended with `İşlem kullanıcı
tarafından iptal edildi` before controller creation. The launch wrapper did not
use Stop error handling, so it printed an empty PID afterward; this is NOT a
successful launch. A separate failure record preserves the cancellation and
confirms `progressive-1-controller` is absent. No higher mode ran. Asked whether
to reopen the consent UI; the existing PHASE 3B authorization remains valid.

Eight independent encoding-evidence fixtures pass: valid run, software fallback,
lost input accounting, queue overflow, false byte total, wrong decoded counter,
wrong PTS and invalid visible-aperture bounds. Original live evidence untouched.
Short-run CPU resource review: Host7.7695%, Driver2.1380%, Pattern3.4005% of one
logical processor over about16.1s (four samples/startup included). No sustained
memory claim is made from this short test. Final preservation: all560 prior
evidence hashes and staged driver/ABI baseline match. Repository hygiene PASS:
118 publishable files, index and50 history blobs. No push/release or PHASE3C work.

### PHASE 3B progressive launch retry — 2026-09-17

Owner explicitly authorized reopening UAC and continuing the progressive tests.
Ran Start-Process for Windows PowerShell, Test-HardwareEncodingProgressive.ps1
-RunPrefix progressive-1, with -Verb RunAs -WindowStyle Hidden -PassThru and
ErrorActionPreference=Stop. Windows consent.exe was observed while the launcher
waited. Start-Process then failed with `İşlem kullanıcı tarafından iptal edildi.`;
launcher exit code 1, no elevated PID returned. The native numeric error code was
not exposed by this output. Do not interpret the error as missing chat permission
or infer whether the owner clicked Cancel versus another consent failure.

Separate private uac-progressive-2-failure.json preserves the command/result.
No progressive-1 controller/stage directory exists; no new encode test ran.
The VERIFIED 800x360 result was neither rerun nor overwritten. No test Host,
pattern or consent process remained after the failure. Read-only SHA256 checks:
all560 prior evidence files unchanged, staged driver/ABI unchanged, known-good
Host executable unchanged. No driver/security/phone operation occurred. Current
security was not freshly reverified by an elevated harness because launch failed;
the last verified baseline remains Secure Boot ON, HVCI ON, TESTSIGNING OFF.

PHASE 3B remains PARTIAL; higher modes, sustained test and encoding reconnect
remain NOT YET TESTED. PHASE 3C was not started. Successful Windows UAC consent
is required to execute the already-authorized test sequence.

### Manual PHASE 3B launch preparation — no test executed

Owner reports no UAC prompt was visible and no No/Cancel action was taken.
Preserve the original Start-Process error as an OS launch result, not a user
refusal. Cause of invisible consent remains UNKNOWN. RunAs retries are prohibited;
no UAC, boot, certificate or security policy modification is authorized.

Read the progressive controller, individual live runner, Host argument parser,
pattern argument parser and independent verifier. Next planned stage is
1280x576 / target30FPS / 5000000 bit/s / 30 seconds, real Display3 source.
Prepared only Test-HardwareEncoding.ps1 with RunName progressive-1-1280-30,
Seconds30, ControlCase observe, Classified, Width1280, Height576, Rate30,
Bitrate5000000. Reconnect omitted; no progressive controller or soak launch.
Windows PowerShell, live script, encode Host, pattern and decoder files exist.
PowerShell AST parse of the absolute-path command passed without executing it.
Target evidence directory and progressive controller directory are both absent;
the unchanged runner rejects an existing run directory before creating files.
The runner generates one shared nonzero hex nonce for pattern and Host and logs
both exact process argument strings into execution.json. Pattern arguments are
run-directory, nonce,150,0,1,observe. Host arguments are --output run-directory/host,
--seconds30, --first-fail, --nonce generated-nonce, --classified, --encode,
--encode-width1280, --encode-height576, --encode-fps30, --encode-bitrate5000000
(spaces between option names and values in the actual runner).

No test, elevation, rebuild or evidence mutation performed. Existing 800x360
PASS_ENCODE_DECODE still records200 outputs/decoded frames. PHASE3B remains
PARTIAL. After manual execution inspect new evidence, independently decode the
actual bitstream and verify source timestamp/nonce/counter correspondence before
accepting the stage or selecting another mode. No PHASE3C work.

### PHASE 3B manual1280x576 run — VERIFIED short stage

Read completed progressive-1-1280-30/execution.json: PASS_CLASSIFIED, Host and
pattern exit0, no forced stop, failureevents0. No new live test or elevation
performed by the agent. Original800x360 and PHASE3A evidence untouched.
Ran out/phase3b/decoder/DecodeH264Evidence.exe with the new run's host directory,
1280,576; checked no decode/report destination already existed. Decode exit0:
879inputs=879decoded,18318433bytes, coded and visible1280x576. Then ran
Verify-HardwareEncoding.ps1 -EvidenceDirectory phase3b/progressive-1-1280-30
-MinimumSeconds30: PASS_ENCODE_DECODE; every decoded PTS/nonce/counter matched
its input and source record, including bounds and exact byte/drop accounting.
All new decode/verification files belong only to this new run.

Duration30.0008517s. Source1601/53.365152FPS;Host1540/51.331876FPS;
encoder879accepted=879outputs=879decoded/29.299168FPS. Exact upstream
1601=1540+4producer+57Hoststale; encoder1540=879accepted+661rate+0backpressure.
Shared queue peak3; encoder queue/pool peak1/1. A1540 B/C/D/E0, invalid0,
pending0, held0. Bitrate4884776.787854bit/s;15keyframes/IDRs. Startup189.7684ms;
latencymean4.649266ms,p50/p95/p994.1/7.2/15.1ms,peak27.5568ms.
GPU-conversion completion walltimeavg4.074951ms,peak63.6476ms, not isolated
GPU execution duration. AMDhardware-only/D3D11-aware path, actualNV12GPU
resource identity/CPUaccess0; no fallback; MF setup/drain/shutdown S_OK.

Six resource samples/28.45s include startup: Host private53727232..57348096,
handles1536->1262;driver private35258368..35311616,handles931..934.
CPU one-core Host16.147%,driver2.363%,pattern2.253%. Saved separate short
resource-review.json; no sustained stability claim. Before/periodic/after
PnP0/0,SecureBootON,HVCION,TESTSIGNINGOFF,CIflags62465,sameboot,events0.
Clean shutdown VERIFIED for this run; encoding reconnect NOT YET TESTED.

Next plan mode1920x864,target30FPS,10000000bit/s,30seconds,observe,classified,
RunName progressive-1-1920-30; directory confirmed absent. Only a manual command
will be supplied. Existing script rejects reused directories. No new soak,
800 rerun, automatic RunAs or PHASE3C. PHASE3B remains PARTIAL; actual maximum
encoder capacity and sustained60FPS are unproven.

### Reboot preflight failure —1920 stage did not start

Owner reports a reboot before running progressive-1-1920-30. Read original
execution.json and both health snapshots without modification: ERROR with
SweetDisplay PnP health failure, same cleanup error, Commands empty, host
subdirectory empty. Both Devices arrays empty; security checks pass with
SecureBoot=true,HVCI=1,CIstatus0,CIflags62465. Boot timestamp changed relative to
previous successful stages. No Host/pattern/helper process found in initial
process inventory. Separate live unelevated Get-PnpDevice returned Access denied;
not used as evidence that the installed package is missing or defective.

Read SweetDisplayDevice/main.cpp and deployment documentation. Existing helper
uses SwDeviceCreate, holds its handle until X/exit, does not set persistent
lifetime, and is not a startup service. Microsoft SwDeviceSetLifetime documents
SwDeviceLifetimeHandle as the default. Missing present nodes after reboot is
consistent with the helper ending. Existing package registration after reboot
has not been independently rechecked; do not infer deletion or reinstall.
Proposed manual recovery: run existing out/windows/x64/Debug/SweetDisplayDevice/
bin/SweetDisplayDevice.exe with no arguments in Administrator PowerShell, keep
it open, and verify both nodes/problem codes before a fresh short test.
No recovery/elevation/install/security/phone action executed by the agent.
Failed1920 evidence preserved. Retry name progressive-2-1920-30 currently absent;
no retry started. Prior800/1280 VERIFIED unchanged; PHASE3B remains PARTIAL.
Reference: https://learn.microsoft.com/en-us/windows/win32/api/swdevice/nf-swdevice-swdevicesetlifetime

### PHASE 3B fresh1920 retry — VERIFIED short stage

Owner reopened helper and explicitly authorized UAC plus one1920 test, stopping
before any next stage. Initial unelevated PnP query returned Access denied;
helper was running. Snapshot hashed81 existing800/1280/failed1920 evidence files.
New RunName progressive-2-1920-30 absent. Started Windows PowerShell via RunAs,
-NoProfile -File Test-HardwareEncoding.ps1 -RunName progressive-2-1920-30
-Seconds30 -ControlCase observe -Classified -Width1920 -Height864 -Rate30
-Bitrate10000000 (normal spaces between options and values in launch.json).
UAC succeeded. Runner's unchanged health gate checked two OK/problem0 devices,
same installedoem83/oem81 packages and SecureBootON/HVCION/TESTSIGNINGOFF before
pattern/Host startup. No driver/package/ABI/certificate/security/phone change.

Execution PASS_CLASSIFIED, duration30.0037516s,Host/pattern exit0, no forced stop,
selected failure-events0. Source1695/56.492935FPS,Host1686/56.192973FPS,
accepted894=outputs894,29.796274FPS. Exact upstream1695=1686+0producer+4stale+
5contention;encoder1686=894accepted+790intentionalrate+2backpressure.
Sharedqueuepeak2,encoderqueue/poolpeak1/1,A1686,B/C/D/E0,invalid/pending/held0.
37,256,023bytes,bitrate9,933,697.224716bit/s,15keyframes/IDRs. Startup203.1429ms;
encode latencymean6.374272,p505.9,p9510.7,p9915.6,peak20.5087ms. Conversion
completionwalltimemean3.624872,peak16.4157ms; not isolated GPU execution time.
AMD hardware MFT path preserved, GPU-backedNV12 inputCPUaccess0, no software
fallback. MF drain/Shutdown S_OK. Observed throughput is not maximum capacity.

Only after live completion ran DecodeH264Evidence.exe <new-run>/host 1920 864,
with absent decode destinations checked first. PASS_DECODE894 actual outputs,
coded/visible1920x864. Verify-HardwareEncoding.ps1 -EvidenceDirectory <new-run>
-MinimumSeconds30 returned PASS_ENCODE_DECODE. All894 decodedPTS/nonce/counter
match input/source records, exact bytes/counts/order and queue checks pass.
Source target selected by SWT0001 identity at2400x1080@60; after reboot its
GDI name is DISPLAY10. Windows Settings numeric label not re-read. This is the
same intended SweetDisplay target, not an assumption based on display number.

Five resource samples include startup: Host34,172,928bytes/964handles initially,
then70,934,528/1254 at all four subsequent samples11.97..28.05s. Driver private
35,487,744..35,639,296,handles928..933. CPU one-coreHost16.729%,driver3.079%,
pattern4.422%;GPU3Dmean6.4%peak7%,VideoCodec0mean6.8%peak7%. Short plateau only;
sustained resource gate still pending. Saved new resource-review.json.
Before/periodic/after PnP0/0,sameboot,CIflags62465,SecureBootON,HVCION,TESTOFF.
SHA256 confirms81 prior3B files and560 prior3A files unchanged; driver/ABI and
known-goodHost unchanged. Failed1920 and historicalUNKNOWN soak preserved.
PHASE3B remains PARTIAL. Next2400x1080,target30FPS,15Mbps,30s NOT STARTED.
No soak, encoding reconnect, next resolution or PHASE3C started.

### PHASE 3B2400x1080@30 target — VERIFIED short stage; STOP before60

Owner authorized only2400x1080,target30FPS,15Mbps,30seconds. Existing helper
running. Preserved SHA256manifest118 files from successful800/1280/1920 and
failed1920. Fresh RunName progressive-1-2400-30 and separate launch/preflight
record directory checked absent. No source/binary rebuild or package change.
UAC launched Windows PowerShell -NoProfile -File Test-HardwareEncoding.ps1
-RunName progressive-1-2400-30 -Seconds30 -ControlCase observe -Classified
-Width2400 -Height1080 -Rate30 -Bitrate15000000 (actual arguments with normal
spaces retained in preflight-2400-30-1/launch.json). Existing health gate checked
adapter and monitor OK/problem0/0, oem83/oem81, SecureBootON,HVCION,TESTOFF before
any live frames. No health acceptance change or security policy modification.

PASS_CLASSIFIED:30.0116845s;source1686/56.178120FPS,Host1684/56.111479FPS,
submitted890=encoded890,actual29.655116FPS. Exact upstream1686=1684+0producer+
1stale+1contention; encoder1684=890accepted+794rate-limit+0backpressure.
Sharedpeak2,encoderpeak1,poolpeak1,A1684,B/C/D/E0,invalid/pending/held0.
Bytes55,631,030,bitrate14,829,165.620477bit/s,15keyframes/IDRs.
Latencymean6.943102ms,p506.7,p958.2,p9911.8,max23.7973;startup186.6423ms.
Conversion completionwalltimemean2.363925,max21.6283ms,not isolatedGPUtime.
Actual AMDhardware-only async/D3D11 MFT and GPUinputNV12 CPUaccess0 verified;
no software fallback/fullframeCPUreadback. LowlatencyON,B0,GOP60,CBR15Mbps.

After live completion and checking all decode destinations absent, ran
DecodeH264Evidence.exe <new-run>/host 2400 1080. Exit0,PASS_DECODE890/890,
55,631,030bytes;coded2400x1088,explicit boundedvisible2400x1080 at0,0.
Verify-HardwareEncoding.ps1 -EvidenceDirectory <new-run> -MinimumSeconds30
returned PASS_ENCODE_DECODE: all890 decodedPTS/nonce/counter match source/input,
exact IDs/timestamps/bytes/drop counts and bounded queue checks pass.
Decoder is independent offline verification of this actual live bitstream.

Host/pattern exit0, no forced stop, clean encoder drain/Shutdown and shared
texture disconnect, selected failureevents0. Before/periodic/after healthyPnP0/0,
samepackages/boot,CIflags62465,SecureBootON,HVCION,TESTOFF. Six resourcesamples
include startup: Host33,345,536bytes/947handles then83,845,120..83,849,216/1254
from6.997s onward. Driver35,606,528..35,647,488bytes,930..933handles. CPU one-core
Host11.999%,driver2.822%,pattern4.260%;GPU3Dmean/peak3%/3%,VideoCodec0mean9.833%
peak10%. Short-run bounded observations only; sustained resource gate pending.
Saved new resource-review and preservation-check. All118 previous3B files and
560 prior3A files unchanged, staged driver/ABI and known-goodHost unchanged.

2400x1080@30 TARGET short gate VERIFIED at measured29.655116FPS. Not maximum
encoder capacity or sustained exact30/60FPS proof. PHASE3B remains PARTIAL.
2400x1080@60, sustained, reconnect andPHASE3C NOT STARTED. Stop as requested.

### PHASE 3B60-target short capacity observation — VERIFIED at measured rate

Owner authorized2400x1080,target60FPS,30000000bit/s,30seconds only. Helper active.
Newcapacity-1-2400-60 andpreflight-2400-60-1 absent before creation. Preserved
SHA256 manifest of all209 earlier3B files. Launched Windows PowerShell with
-NoProfile -File Test-HardwareEncoding.ps1 -RunName capacity-1-2400-60
-Seconds30 -ControlCase observe -Classified -Width2400 -Height1080 -Rate60
-Bitrate30000000 via authorizedRunAs; exact spaced arguments inlaunch.json.
No fixed progressive controller used (its old25Mbps plan is not this30Mbps run).
Unchanged preflight verified PnPhealthy0/0 and security before frame startup.

LivePASS_CLASSIFIED,30.008907s. Source1682=Host1682/56.050025FPS;submitted1572=
encoded1572/52.384447FPS. Dropsproducer0,stale0,contention0,rate110,backpressure0.
Exact1682=1572+110;sharedqueuepeak2,encoder/poolpeak1/1;A1682,B/C/D/E0;
invalid/pending/held0. Bytes98,260,076,actualbitrate26,194,909.664654bit/s.
Encode latencyp506.6,p958.2,p9910.0,max17.9433,mean6.854583ms;startup178.0558ms.
Conversion completionwalltimemean1.971667,max17.885ms,not isolatedGPUduration.
14keyframes/IDRs. AMDh264Encoder sameCLSID,hardware-only async/D3D11DXGI path,
NV12GPU CPUAccess0, no software fallback/fullframeCPUreadback. LowlatencyON,B0,
GOP120,CBR30Mbps readback. MF drain/end/Shutdown S_OK.

After live pass, checked fresh decode paths, ran DecodeH264Evidence.exe
<new-run>/host 2400 1080: PASS_DECODE1572/1572,98,260,076bytes,visible2400x1080
inside bounded coded2400x1088. Verify-HardwareEncoding.ps1 -EvidenceDirectory
<new-run> -MinimumSeconds30 PASS_ENCODE_DECODE. Every decodedPTS/nonce/counter
matches input/source, exact counts/bytes/order/bounds pass. Decoded media-equivalent
FPS52.384447 is count/live duration, not offline decoder wall-clock throughput.

Independent exact-integer replay of existing Host fixed60Hz-bin selector against
all1682 sourceQPC records reproduced all110 rate drops, zero mismatches.110bins
have two real source frames; selector drops second arrivals0.0043..4.3044ms before
nextbin(mean1.9253). This is6.5398%Hostinput, never submitted to encoder. Thus
source cadence below60 plus Host rate-selector policy explains observed gap;
encoder saturation NOT OBSERVED. Strict all-source SOURCE-LIMITED acceptance is
NOT YET TESTED because110 sourceframes never reached MFT. No ENCODER-LIMITED
queue/backpressure evidence:0pressure,peak1,all submitted complete. Four7.5s
latency means6.850/6.881/6.791/6.897ms do not accumulate. IDRindices0,120..1560,
120codedframes each,media-timegaps2.200..2.392s. Savedcapacity-analysis.json.
No selector modification, duplicate frames or additional live run performed.

Six resource samples/startup included:Host33,447,936bytes/959handles then
85,536,768..85,544,960bytes/1254handles;driver35,561,472..35,622,912,926..931handles.
CPU onecoreHost18.520%,driver2.710%,pattern4.093%;GPU3Dmean5%peak6%,VideoCodec0
mean17.833%peak21%. Short resource review only, not sustained/leak or capacity
proof. Host/pattern exit0,no forced kill,cleandrain/disconnect,events0.
Before/periodic/afterPnP0/0,samepackages/boot,CIflags62465,SecureBootON,HVCION,
TESTOFF. SHA256all209 prior3B files and560 prior3A files unchanged; stageddriver/
ABI/known-goodHost unchanged. Original failed/UNKNOWN runs remain intact.

Short measured-capacity/integrity result VERIFIED at52.384447FPS, not60FPS or
all-sourcecapacity. PHASE3B PARTIAL. Stop: no sustained,reconnect orPHASE3C run.

### Uncapped-source control — VERIFIED for all unique Host-received frames

Preserved250 previous3B evidence files and original source/binary hashes before
editing. Added Settings.uncappedSubmission defaultfalse, --encode-uncapped CLI
and diagnostic submission_rate_limiter=disabled marker. Only wraps existing
submission time-bin selector; unchangedsource/MFTfps60/bitrate30Mbps/timestamp/
GPUconversion/backpressure/token/queue/4NV12/3shared limits. Added isolated
Build-UncappedEncodeHost.cmd outputout/encode-host-control. Runner's opt-in
-UncappedSubmission chooses newbinary; default uses unchangedoldencode-host.
First build failed because D:EWDK absent. Keptbuild-control-1.txt; mounted existing
EWDK ISOread-only (no install), D:restored, build-control-2 passedwithoutwarnings.
OldHost,decoder,pattern binaries unchanged. Invalid uncapped-without-encode
CLI returns87/0x57 beforedeviceaccess; negative guard checkpassed.

Single authorized UAC launch: powershell.exe -NoProfile -File
Test-HardwareEncoding.ps1 -RunName uncapped-1-2400 -Seconds30 -ControlCase observe
-Classified -Width2400 -Height1080 -Rate60 -Bitrate30000000 -UncappedSubmission.
Exact spaced args/buildhashes inpreflight-uncapped-1. Newrun absent beforelaunch.
Existing healthgate passed PnP0/0 andsecurity beforeframes. No deployment change.

LivePASS_CLASSIFIED30.0096792s. Source1654/55.115551FPS;Host1637/54.549067FPS;
submitted1637=encoded1637. Rate0/backpressure0;producer0/stale17/contention0.
Exact1654=1637+17;1637=1637submitted+0rate+0pressure. Sharedpeak3,encoder/pool1/1,
A1637,B/C/D/E0,invalid/pending/held0. Latencyp506.6,p958.2,p9910.4,max13.5382ms,
mean6.829216ms;startup170.5762ms. Conversioncompletionmean2.666908,max23.2895ms
(includesCPU scheduling,notisolatedGPUexecution). Source-to-submitmean9.867293,
max28.6689ms.102,322,966bytes,actual27,277,323.511009bit/s,14IDR/keyframes.

Ran independent DecodeH264Evidence.exe <new-run>/host 2400 1080 after checking
freshdestinations:PASS_DECODE1637/1637,actualbitstreamaccepted,coded2400x1088 with
explicitvisible2400x1080. Verify-HardwareEncoding.ps1 -EvidenceDirectory <new-run>
-MinimumSeconds30 PASS_ENCODE_DECODE;allPTS/nonce/counter/sourceframes matched.
Additionaluncapped-acceptance.json verifiesdisablemarker,runnerflag,uniqueframeIDs,
allinputdecisionsaccepted,Host=submitted=encoded=decoded,rate/backpressure0.
Four7.5s latencymeans6.803/6.833/6.895/6.786ms,outputs409/409/411/408;noaccumulation.
IDRinterval120encodedframes,media-time2.101..2.292s. No duplicates/interpolation.

AMDhardware-onlysameMFT/CLSID,async/D3D11DXGI,NV12defaultGPU CPUAccess0,pathkept;
no softwarefallback/fullframeCPUreadback. Sixresourcesamples/startupincluded:
Host34,418,688bytes/964handles then86,568,960/1254 flatfrom6.964s;driver35,659,776
bytesflat,handles928..931. CPUonecoreHost22.543%,driver2.487%,pattern3.869%;GPU3D
mean5.5%peak6%,VideoCodec0mean18.667%peak21%. Shortplateau,not sustainedproof.
Host/patternexit0,noforcedstop,cleanencoder/sharedshutdown,events0. Before/periodic/
afterPnP0/0,samepackages/boot,CIflags62465,SecureBootON,HVCION,TESTOFF. Helperkept.
SHA256250prior3B/560prior3Afiles,driver/ABI,oldHost/decoder/pattern unchanged.

PASS scope: everyuniqueHostframeencoded/decodedat54.549067FPS; no observedMFT
saturation.17upstreamHoststaledrops remainexplicit; notall1654IddCxencoded and
not60FPS. Exactupstreamstaleschedulingcause notisolated; no throughput ceiling
attributed toconversion/MFTfromthisrun. PHASE3B PARTIAL. Stoppedafteronecontrol;
no sustained,reconnect,PHASE3C,driver/security/phone work.

### Sustained uncapped validation started — 2026-09-18

Owner authorized >=1800s sustained encoding, independent decode/content check,
then clean and forced-termination recovery if sustained acceptance passes. No
short progressive repeat, no binary changes. Snapshot all302 prior3B files and
five known-good binaries in preflight-sustained-1 before any new evidence.
Runner-only telemetry addition: WorkingSetBytes next to PrivateBytes/Handles.
No live queue/MFT/source policy change. Free space about59GB before launch;
compressed H264/AU evidence stays private/ignored, no raw full-frame video.

Existing helper running. Launched existing Test-HardwareEncoding.ps1 with
RunName sustained-1-uncapped,Seconds1810,observe,Classified,2400x1080,MFT60,
30000000bit/s,UncappedSubmission; no immediate Reconnect. Existing verified
out/encode-host-control binary used unchanged. Health gate passed PnP0/0,
oem83/oem81,SecureBootON,HVCION,TESTOFF,CIflags62465 before frames. Exact launch
arguments/PID in preflight-sustained-1/launch.json. Initial Host~30FPS versus
source~56FPS caused accounted upstream stale/producer drops; around five minutes
Host returned~55..59FPS. Do not classify upstream stale as encoder corruption.
Final encoder counts, decode and resource/latency analysis pending while running.

Prepared but NOT EXECUTED Test-HardwareEncodingRecovery.ps1. It refuses to run
without sustained >=1800s, PASS_ENCODE_DECODE, all-Host acceptance and explicit
resource/latency review pass. Planned clean20s+fresh15s validation, active encoding
process termination, driver-absent-Host progress check, fresh20s encode/decode.
Only owns/kills crash-test Host; helper/driver continuity required. All distinct
run directories, no reinstall/elevation bypass. Independent source/PTS/marker
verification remains unchanged. Added offline Measure-UncappedEncoding and
WorkingSetBytes-aware resource analysis. Parser checks pass; recovery gate rejects
unfinished current evidence before creating controller. Valid1637-frame fixture
passes; deliberately dropped Host frame fixture rejects; originals untouched.
No recovery has run yet; PHASE3B stays PARTIAL until all required gates pass.

### Recover interrupted sustained run and verify artifacts — 2026-09-18

Read-only process/file inventory found sustained-1-uncapped completed in the
background, not still running:1810.0008772s, PASS_CLASSIFIED, Host/pattern exit0,
no forced cleanup, failure-events0. Existing helper retained. No replacement
sustained run was started and no old evidence was reused/overwritten.

After checking output paths absent, ran DecodeH264Evidence.exe on that run's
host directory with expected2400/1080: PASS_DECODE93,069/93,069. Stream contains
5,817,406,610 bytes; coded2400x1088, explicit visible2400x1080. Then ran:

```powershell
& scripts/windows/Verify-HardwareEncoding.ps1 -EvidenceDirectory docs/evidence/private/phase3b/sustained-1-uncapped -MinimumSeconds 1800
& scripts/windows/Measure-UncappedEncoding.ps1 -EvidenceDirectory docs/evidence/private/phase3b/sustained-1-uncapped
```

First command PASS_ENCODE_DECODE: all decoded PTS/nonce/counter, render ledger,
geometry/slot identity, exact input/output bytes and drop accounting verified.
Second preserves FAIL_ALL_UNIQUE_HOST_FRAMES and its error because99 out of93,168
Host frames were rejected for backpressure. This stricter assertion did NOT pass.
Report and stdout/error remain private. Corrected only numeric overload selection
for fractional final time-bucket duration in the offline measurement script.

Exact totals:101797=93168+69producer+8509stale+50contention+1shared ready;
93168=93069accepted+0rate+99pressure;93069accepted=outputs=decoded. A93168,
B/C/D/E0,invalid0,encoder pending0,held0,shared peak3/encoder peak1. Source56.241409,
Host51.474008,accepted/output51.419312FPS. Actual25.712282Mbps,776IDR/keyframes,
all775 IDR gaps120encodedframes,media-time2.0160008..4.7794663s. Latency histogram
p50/p95/p99/max6.7/28.2/30.8/118.3725ms; full detailed5min table in PHASE3B_RESULTS.

Recovered first5min Host/output29.86FPS and27.27ms mean latency; later full buckets
output54.867..56.693FPS and6.707..7.069ms mean latency. Early drop bucket
producer63/stale7722/contention1 precedes encoder admission; exact scheduling
cause UNKNOWN/UNRESOLVED. No encoder pressure in that first interval. Later99
pressure decisions are isolated(max streak1), eachpending1/poolbusy1, so branch
identifies absent MFT NeedInput token rather than full pool/queue. Underlying
readiness timing UNKNOWN; do not claim every Host frame encoded or maximum60FPS.

Latest owner attachment explicitly permits unique-frame submission when bounded
pipeline allows it, with no sustained backlog/latency growth. Separate manual,
hash-bound sustained-review.json PASS_SUSTAINED_REVIEW records that scope while
keeping strict FAIL_ALL_UNIQUE_HOST_FRAMES unchanged. Recovery gate now requires
that explicit integrity/accounting/resource review and matching evidence hashes
if strict all-Host is not passed. No binary, security or content checks weakened.
An initial offline review shell failed because function R conflicted with the
built-in r/Invoke-History alias; no review file was written. Re-execution with
AssertReview completed and saved the new result. This is not a live encode failure.

After120s Host private83,525,632..83,808,256B,delta106,496; working set73,048,064..
73,674,752B,delta569,344;handles1255->1246. Last5min56samples private/working-set
endpoints equal,handles1246 throughout. Driver private delta0,handles934..936->934,
working set decreases. Resource review: bounded steps/plateaus; no continuing
growth observed. CPU one-core Host16.230%,driver2.484%; GPU VideoCodec0 mean16.955%,
peak21%;3Dmean5.564%,peak10%.55periodic plus before/after health: PnP0/0, security
unchanged,selected driver/system failure-events0. No full-frame raw video stored.

Preservation SHA256 before recovery:560 prior3A evidence files,302 prior3B files,
five binaries and4 staged-driver/ABI files unchanged. Old historical3A soak UNKNOWN.

### Clean encoder reconnect verified; crash harness sharing error retained

Using authorized UAC, launched Test-HardwareEncodingRecovery.ps1 with
SustainedEvidenceDirectory=sustained-1-uncapped,RunPrefix=sustained-1-recovery.
Exact executable, arguments and launch PID are in preflight-sustained-1/
recovery-launch.json; controller logs each child command and result.

Clean run:20.0231764s,source1120/55.935181FPS,Host=accepted=encoded=decoded654/
32.662150FPS. Exact1120=654+1producer+465stale. Fresh process15.0018052s:
source849/56.593189FPS,Host=accepted=encoded=decoded509/33.929250FPS;
849=509+1producer+339stale. Both rate/pressure0,shared3/encoder1,invalid/D/E0,
all timestamp/marker associations pass. Clean exits/drain,epoch/order pass.
These short reconnect runs also show upstream slowdown; they prove functional
encoder reinitialization, not sustained55/60FPS immediately after reconnect.

Controller then FAILed reading live crash-host/stdout.txt with ReadAllText:
the file was in use by another process (sharing conflict). It stopped before the
planned active-output kill checkpoint, killed only its own Host during cleanup,
and closed its own pattern with exit0. No successful crash recovery is claimed
for this attempt; controller/crash-tail files and verified clean run retained.
Tiny partial crash bitstream is diagnostic, not accepted clean evidence.

Fixed script's read-only log snapshot to FileStream FileShare.ReadWrite|Delete;
this accommodates the existing redirected writer without changing filesystem
permissions. Offline fixture out/phase3b/recovery-share-check-1 reproduces old
ReadAllText sharing rejection and verifies new reader while writer remains open.
PowerShell parser passes. Added VerifiedCleanEvidenceDirectory guarded by clean
decode/count/duration/health continuity so only missing crash step is retried.
RunPrefix=sustained-2-recovery,VerifiedCleanEvidenceDirectory=sustained-1-recovery-clean;
new UAC launch recorded in preflight-sustained-1/recovery-resume-launch.json.
First recovery/clean artifacts hashed beforehand; no sustained/clean rerun.

### Forced-Host recovery and final bounded-admission decision — 2026-09-18

sustained-2-recovery preserved the existing clean result. New crash Host produced
growing AU evidence3,936,256B and reported frames before forced termination,
exit0xFFFFFFFF. Instantaneous texture ownership at kill not asserted. Read-only
absent-Host probe:2.0005689s,112newsourceframes/55.984075FPS,active1,connected0.
Driver/helper unchanged, original crash pattern closes with exit0.

Fresh20.0020479s process:source1104/55.194348FPS,Host1094/54.694400FPS,
accepted=encoded=decoded1093/54.644405FPS. Exact1104=1094+10stale;
1094=1093accepted+1pressure. Producer/contention/rate/invalid/D/E0,shared peak2,
encoder peak1,pending/held0,A1094. All1093 decoded source/PTS/nonce/counter matches
pass. Actual27.325009Mbps,10IDRs,latency p50/p95/p99/max7.1/8.4/11.3/30.6172ms.

Controller stopped after independent encode/decode verification with strict
`Recovery all-Host frame failure` due to that1pressure drop. Its FAIL execution
is preserved. No additional encode test was launched. Separate offline review
under the latest owner's bounded/no-persistent-pressure scope found the event
at4.9502878s,pending1/poolbusy1,followed by valid output and no backlog. Four5s
latency means7.245/7.239/7.356/7.140ms. All admitted outputs decoded; one rejected
admission is not hidden or relabelled as upstream loss.

Completed remaining controller checks independently from closed artifacts and
read-only process/event inspection: fresh resource names (3 versus crashed3),
same epoch, first source ID after crash report, fresh process IDs, PTS0 start,
hardware MFT reinitialization, clean shared/encoder shutdown, no leftover Host/
pattern. Driver/helper retained. Nine recovery health snapshots match sustained
boot/security baseline; PnP0/0,SecureBootON,HVCION,TESTOFF. Read-only Get-WinEvent
query spanning both controllers through final review found0 relevant WUDF/driver/
pattern crashes,system driver-framework events,bugcheck/power/display failures.
Same driver post-cleanup private35,643,392B/handles931 across sustained/clean/crash.

An initial offline review compared a PowerShell7 JSON DateTime to a literal string
and rejected Health continuity; diagnostics showed identical timestamps and all
health fields passing. A diagnostic one-liner then had an empty-pipe parser error.
Neither executed any live test or wrote a final review. Corrected typed UTC-ticks
comparison in ignored preflight-sustained-1/Review-Recovery.ps1 and ran it once:
PASS_RECOVERY_BOUNDED_REVIEW, preserving both controller FAILs. Review script,
event query result and final review retained. No settings/acceptance data altered.

PHASE3B VERIFIED under latest bounded-admission scope: actual1810s hardware GPU
encoding,all admitted frames decoded/content verified,exact accounting,bounded
queues/stable observed resources and latency,clean/forced reconnect. Strict
lossless-all-Host assertions still FAIL for sustained99 and post-crash1 drops.
Early scheduling slowdown and underlying transient token timing remain UNKNOWN.
No60FPS or maximum encoder throughput claim. Updated STATUS/PHASE3B_RESULTS/
ROADMAP; all prior VERIFIED/failed/UNKNOWN runs kept. Stop before3C; no driver,
certificate,boot/security,phone/USB/transport/simulator changes or publication.

Final preservation: all560 prior3A/302 prior3B/five binary/four driver-ABI hashes
match, as do76 files from the failed first recovery plus its successful clean
run. Nine sustained-review source hashes also unchanged. Test-PublicRepository.ps1
PASS:121 publishable working-tree files,index and50 reachable history blobs.
Raw/compressed/private evidence stays ignored; no push. Four touched PowerShell
scripts parse without errors. Diff whitespace check found only an extra STATUS
EOF blank line; removed it before final verification. Only SweetDisplayDevice
remains running among project processes; no automatic later work scheduled.
