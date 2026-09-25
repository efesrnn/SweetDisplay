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

## 2026-09-18 — PHASE 3C implementation and first live attempt (PARTIAL)

Recovered the existing worktree and read both PHASE 3C requests. Preserved all
prior evidence, five known-good binaries and driver/shared sources in SHA256
manifests under ignored phase3c/preflight-1. No git reset, cleanup, commit or push.
No driver/deployment, certificate/trust, boot/security, phone or USB changes.

Implemented explicit binary 48-byte framing, HELLO/CAPABILITIES, strict fresh
session/sequence semantics, bounded incremental parser, CRC/source metadata
validation, bounded queue and deadline-aware localhost ByteStream. Added
IDR/SPS/PPS recovery gate after loss; no dependent-frame replay. Separate
transport worker and compressed-output callback preserve GPU-native encoding.
Simulator is a protocol endpoint only; no visible decoder/rendering implemented.

Build/test command families (run from repository root, exact output logs private):
- cmd /d /c "scripts\windows\Build-ProtocolTests.cmd D:"
- out/protocol-tests/ProtocolTests.exe
- cmd /d /c "scripts\windows\Build-Transport.cmd D: endpoint-only"
- cmd /d /c "scripts\windows\Build-Transport.cmd D:"

Initial protocol build failed for missing crtdbg.h; added explicit installed SDK
include/library paths. First transport build failed C2228 from a vexing-parse
Listener declaration; corrected brace initialization. All failed build logs were
retained, followed by successful /W4 /WX builds. Protocol tests pass 58,132 checks
on both recorded successful executions. Coverage includes all representative
fragmentation boundaries, coalescing, truncation/poisoning, malformed metadata,
length/overflow/allocation limits, handshake/version/sequence, fresh parser,
queue saturation and IDR recovery plus a seeded 20,000-mutation corpus.

Normal synthetic fixture: 400 seen =370 admitted+2 disconnected+28 resync+0
queue overflow;370 admitted=370 ACKed;sent=received370,queue peak1,drain pass.
Slow fixture:400=120+92+178+10;120=80 ACKed+30 aborted+10 unconfirmed;
fully sent=received90,queue peak3,11 sessions/10 reconnects,drain pass.
All 10 unconfirmed AUs exist at receiver (missing ACK confirmation, not receiver
loss). Every new session starts IDR/SPS/PPS. Both independent fixture-verification
reports PASS. Synthetic NAL-category fixtures are not actual video acceptance.

After these gates, authorized test-only UAC launched:
powershell.exe -NoProfile -File scripts/windows/Test-LocalTransport.ps1
  -RunName live-normal-1 -Seconds 30 -Port 48231
The unique directory was new; no old run overwritten. Actual default mode from
execution.json:2400x1080,nominal60FPS,30Mbps,uncapped submission. Existing driver
and helper retained. AMD hardware MFT remained active, GPU NV12 path, no software
fallback or full-frame CPU readback. Encoder identity and calls retained.

Live result ERROR: Host exit1, E: UNCLASSIFIED content; bounded evidence frozen;
code13(0x0000000D). Pattern counter2082->2062 with matching nonce; producer2089.
A474/B0/C0/D0/E1. Independent desktop reference did not match the held crop.
The retained crop contains visually intact binary pattern, insufficient for
contemporaneous compositor correspondence. Owner replied unsure about Win+D/
minimize/window movements. This is diagnostic context only: E remains E, cause
UNKNOWN, not proven pipeline corruption. No subsequent live run/reconnect.

Bounded first-failure image/metadata/history retained. Source-at-failure827,
producer replacements4,stale348,busy snapshot0,invalid0,shared high-water3.
Separate cumulative contention total56 cannot be treated as session loss.
No final encode/session/transport result existed after failure; none fabricated.
Host exited naturally1; pattern clean exit0; receiver failed graceful completion
and cleanup terminated it after its wait,exit0xFFFFFFFF. No clean end-to-end pass.
PnP before/after0/0,SecureBootON,HVCION,TESTSIGNINGOFF,same boot;selected relevant
crash/bugcheck events0. Existing driver/helper remained alive.

Offline, ran the unchanged decoder on each REAL captured AU file:
out/phase3b/decoder/DecodeH264Evidence.exe
  docs/evidence/private/phase3c/live-normal-1/host 2400 1080
  docs/evidence/private/phase3c/live-normal-1/host/decode-prefix
and with receiver-1 replacing host. Both PASS_DECODE474/474;visible2400x1080,
coded2400x1088,source timestamp/nonce/counter association matches.
Ran scripts/windows/Verify-TransportPrefix.ps1 -EvidenceDirectory
  docs/evidence/private/phase3c/live-normal-1
once; new completed-prefix-verification.json says PASS_COMPLETED_PREFIX_ONLY.
474 submitted=encoded=admitted=sent=received-validated=ACKed=decoded;
29,628,028 AU bytes, Host/receiver files SHA256 identical. Completed-prefix
transport drops/resync/pending0;queue and outstanding peak1. Receiver first-last
span15.4004375s,inter-frame cadence30.713413FPS,payload15.390746Mbps over that
span. This is not full-run performance/acceptance or60FPS. Transport latency
p50/p95/p99/max0.3412/0.6127/0.7101/0.9324ms. Encode wall latency
25.2610/33.5176/34.9469/36.0169ms;4 IDR/SPS/PPS outputs. The lower cadence than
prior uncapped runs remains unresolved; do not claim performance non-regression.
Three resource samples span startup to~12.8s, insufficient for a leak/plateau
claim; details in PHASE3C_RESULTS.md and private resources/GPU CSVs.

Offline-only follow-up corrected independent-clock receiver telemetry to mark
latency unavailable instead of subtracting unaligned clocks. Built separately:
cmd /d /c "scripts\windows\Build-Transport.cmd D: all review"
PASS /W4 /WX; separate *-review outputs, NOT YET TESTED live. Original live-tested
binaries unchanged. No wire-format or verified encoder-path redesign.

Updated PROTOCOL/USB_PROTOCOL/ARCHITECTURE/PHASE3C_RESULTS/STATUS/ROADMAP.
Final SHA256 preservation PASS:1,231 prior evidence files,five old binaries,
23 driver/shared source files,three live-tested 3C binaries. PHASE3B remains
VERIFIED under its original scope. Historical failures/UNKNOWNs remain intact.
PHASE3C PARTIAL: missing complete real run, clean drain, live receiver reconnect
and resolution/classification of E. Stop before PHASE3D; no visible rendering.

Final repository hygiene PASS:137 publishable working-tree files,index and125
reachable history blobs checked. Pattern scanning is not a guarantee of absence
of all secrets. Three new PowerShell scripts parse successfully. git diff --check
passes (only normal LF/CRLF conversion notices). Read-only process inspection
shows only the retained SweetDisplayDevice helper among project processes; no
Host/pattern/receiver/fixture left running. No new live test scheduled or started.

## 2026-09-18 — PHASE 3C source-counter/transport distinction

Latest owner instruction authorizes a new >=30s run and then real receiver
reconnect, only after deterministic gates. Historical live-normal-1 preserved.
New private preflight-2 snapshot hashes all previous3C evidence and binaries.

Independent historical boundary review: last five transported source IDs
120009/120011/120013/120014/120015 map to protocol sequences487..491 in one
session. AU lengths/PTS/source QPC/CRC/ACK and decoded nonce/counter match.
The two full AU capture files remain byte-identical. Failing source120018,
presentation120213, counter2062 after2082 was never submitted to encoder;
there is no failing-frame AU/FRAME/ACK or later acquired frame evidence. All474
recorded FRAME sequences increase without replay. Old full-message heartbeat
bytes were not retained; do not fabricate complete historical header proof.
Counter regression is observed source content; underlying provenance remains
UNKNOWN. Historical ERROR/E1 is unchanged, not promoted or reclassified.

Added PHASE3C-only TransportContentPolicy: E with sole regression reason4 and
valid binary pattern/nonce/producer ledger may continue provisionally. E remains
observable and separately counted in transport_counter_only. All D and other E
remain fatal. Final transport acceptance still requires strict session/sequence,
CRC/length, exact ACK and independent decode/source content correspondence.
No shared ABI/driver/PHASE3B binary changed. Added bounded complete TX/RX header
ledgers (including handshake/heartbeat/ACK), without changing wire format.

Builds use new content-v2 output directories; old/review binaries preserved:
cmd /d /c "scripts\windows\Build-ProtocolTests.cmd D: content-v2"
cmd /d /c "scripts\windows\Build-Transport.cmd D: all content-v2"
Both PASS /W4 /WX. Test-ProtocolTransport.ps1 -RunName suite-content-2 PASS:
58,155 deterministic checks, including counter2082->2062 with valid sequence,
replayed/duplicate/regressed sequence rejection, and strict legacy policy.
Normal fixture:400=370 admitted+1 disconnected+29 resync;370 sent/received/ACK;
queue peak1,outstanding1. Slow fixture:400=120+92+178+10;120=80 ACKed+30 aborted+
10 unconfirmed;90 sent/received,queue3,outstanding4,11sessions/10reconnects.
Full-message sequence, handshake, exact ACK and queue ledgers verify. These are
synthetic parser/transport fixtures, not real-video acceptance.

Added separate Verify-TransportHandoff (legacy verifier unchanged), extended
encode verification and independent Verify-LocalTransport with per-AU CRC/SHA256,
message transcript matching, ACK accounting and post-reconnect recovery checks.
A source-level review caught and corrected an interpolated variable in the new
PowerShell verifier before live use. All scripts parse. Non-elevated PnP query
returned Access denied; it is not evidence of an unhealthy device. The elevated
controller performs mandatory read-only PnP/security preflight before streaming.

Authorized test-only elevation requested once for Invoke-Phase3CValidation.ps1
-RunName flow-content-2. Windows consent process appeared; no evidence controller
had started at the initial status check. No UAC/security policy change or retry.
Controller gates35s normal -> independent decode/verification ->45s real receiver
termination/reconnect -> independent decode/verification. It stops on failure.

Elevation completed with Windows error "İşlem kullanıcı tarafından iptal edildi."
No controller PID/run directories were created. Owner rejection is UNKNOWN;
consent-process observation alone does not prove a visible prompt was declined.
No second RunAs attempt. Prepared exact manual Administrator PowerShell command
for the same controller; all three flow-content-2 run names remain unused.
Fresh live35s/decode/drain/reconnect45s gates NOT YET TESTED; PHASE3C PARTIAL.

Final follow-up preservation PASS:1,231 prior evidence files,five original
binaries,23 driver/shared files and108 prior3C evidence/build files unchanged.
Repository hygiene PASS:142 publishable files,index and125 history blobs checked;
six modified/new PowerShell scripts parse and git diff --check passes. Only
retained SweetDisplayDevice helper remains among project processes. No live
controller exists; no background test or automatic next phase was scheduled.

## 2026-09-19 — receiver-first DRAIN controller race

Owner ran Invoke-Phase3CValidation.ps1 -RunName flow-content-2 in Administrator
PowerShell. Controller stopped after normal stage: "Receiver exited before Host
drain". Reviewed closed artifacts before any mutation/live rerun. Receiver exit0,
drained=true,1792 validated frames, no protocol/socket/truncation errors. Matching
Host/receiver final DRAIN/DRAIN_ACK headers sequence1829 show successful protocol
drain. Host transport-result records1792 admitted=sent=ACKed,pending0. Original
execution nevertheless records HostForcedStop=true and no successful Host exit.
Cause is unconditional receiver.HasExited failure in controller loop; receiver
can legitimately finish after DRAIN_ACK before Host process exits. Old execution
ERROR stays unchanged; do not certify process clean exit from component summaries.

Created separate ignored shutdown-review-1 with hashes of43 original flow-content-2
controller/run files and pre-fix controller script. Added TransportShutdown helper:
require native receiver exit0,drained result,terminal DRAIN/DRAIN_ACK transcript,
then <=5000ms Host exit grace. Existing native Host exit-code check remains outside
the helper and mandatory. Missing proof,unexpected errors,nonzero exit and hangs
remain fatal. No C++/driver/encoder/deployment/binary/security changes in this fix.

Test-TransportShutdown.ps1 -RunName shutdown-tests-1 PASS9 real child-process
cases in PowerShell7. First Windows PowerShell -File invocation in sandbox was
execution-policy restricted; did not bypass/change policy. Identical command
with normal host access (no RunAs/UAC/ExecutionPolicy flag) using
-RunName shutdown-tests-winps-1 PASS9. Cases:receiver-first,both exited,receiver
error,missing result,not drained,protocol error,missing ACK,Host hang,Host error.
ProtocolTests.exe rerun:PASS58155; output in separate review folder.

Unchanged DecodeH264Evidence.exe read original host and receiver-1 AU captures;
outputs go only to shutdown-review-1/{host,receiver-1}-decode. Both PASS_DECODE
1792/1792,112011442bytes,visible2400x1080,coded2400x1088. Added isolated review
output option to Verify-LocalTransport (no normal acceptance relaxation):
-EvidenceDirectory docs/evidence/private/phase3c/flow-content-2-normal
-ReviewDirectory docs/evidence/private/phase3c/shutdown-review-1
PASS_TRANSPORT_ARTIFACTS_ONLY,FullRunAccepted=false. Every message sequence,
handshake,FRAME length/CRC,exact cumulative ACK,AU SHA256 and decoded association
passes. Private Review-Source.ps1 additionally checks all1798 source/input rows,
producer ledger,resource identity,timestamps,all1792 accepted-to-decoded content
matches,hardware GPU invariants and recorded health. PASS_SOURCE_ENCODE_ARTIFACTS_ONLY;
original ERROR/forced stop explicitly retained. No original report overwritten.

Measured35.0267594s:source1974/56.356912FPS;Host1798/51.332182FPS;
accepted=encoded=transported=ACKed=decoded1792/51.160885FPS using Host duration.
1974=1798+175stale+1ready;1798=1792+6encoder pressure+0rate. Transport loss0,
shared/encoder/transport peaks3/1/1,A1798/B0/C0/D0/E0.15IDR,25.583056Mbps.
Encode latency p50/p95/p99/max6.7/8.2/10.0/19.4057ms;transport
0.3864/0.5660/0.6060/0.9138ms. Hardware-only AMD/DXGI/NV12 proof retained.
Before/periodic/after PnP0/0,SecureBootON,HVCION,TESTOFF,same boot,relevant events0.
Pattern andreceiver exits0;Host process clean exit unverified. Reconnect not run.

Original43files SHA256 unchanged. No new live test/UAC attempt. Prepared unused
flow-content-3 normal/reconnect names for same gated35s->decode->45s controller.
STATUS/ROADMAP/PHASE3C_RESULTS updated; PHASE3C PARTIAL, stop before3D.

Final hygiene PASS:144 publishable files,index and125 history blobs; whitespace
and PowerShell syntax checks pass. Original43run files rechecked unchanged.
Only retained SweetDisplayDevice helper remains running among project processes.
No commit/push,security change,live rerun or automatic next-phase work occurred.

## 2026-09-19 — PHASE 3C final VERIFIED acceptance

Owner manually ran controller flow-content-3. Read closed artifacts first; did
not start/repeat a live test. Controller PASS/COMPLETE records normal and real
receiver reconnect as PASS_ENCODE_DECODE_TRANSPORT. Both final Host/receiver/
pattern native exit codes0, no forced final cleanup, relevant failure events0.
Reconnect intentionally terminated receiver1 only, while Host remained active.

Created separate private final-review-1, snapshotting all flow-content-3 files.
Copied only closed decoder result/CSV files into independent review directories;
re-ran Verify-LocalTransport against ORIGINAL AU/transcript data with separate
ReviewDirectory for each run. Both PASS_TRANSPORT_ARTIFACTS_ONLY (this verifier
alone deliberately cannot certify process shutdown). Private Review-Phase3C.ps1
then independently checked successful controller/process outcomes,minimum durations,
all source/input/output/decoded associations,producer ledger,slot/epoch/geometry,
all exact source/encoder accounts,hardware GPU calls,all7health snapshots and
real receiver continuity/recovery. PASS_PHASE3C_FINAL_REVIEW; PHASE3C VERIFIED.
No original report/evidence overwritten and no missing result fabricated.

Normal35.0015254s:source1977/56.483253FPS,Host1786/51.026348FPS;
1782 accepted=encoded=received=ACKed=Host/receiver decoded,50.912067encodedFPS.
1977=1786+190stale+1contention;1786=1782+4encoder pressure+0rate.
Transport1782=1782admitted=ACKed,all other decisions0,pending0.111386382AUbytes,
25.458635Mbps,15IDR,shared/encoder/transport peaks3/1/1,queue62616Bpeak.
A1786/B0/C0/D0/E0. All captured AU bytes/CRC and decoded nonce/counter/PTS match.

Reconnect45.0023031s:source2509/55.752702FPS,Host2115/46.997595FPS;
2106accepted=encoded=independently Host-decoded,46.797605encodedFPS.
2509=2115+12producer+380stale+2contention;2115=2106+9pressure+0rate.
2106seen=1908admitted+160disconnected+38resync;1908=1907ACKed+1unconfirmed.
1907received=401beforetermination+1506afternewreceiver. One unconfirmed attempted
frame149500 absent from receiver ledgers; not hidden as a successful/lost ACK.
Four expected socket events:reset10054 and3connect-timeouts10060;protocol errors0.
No queue overflow/abort/expiry;pending0,peaks3/1/1.131638464encodedbytes,
23.401196Mbps,18IDR,A2115/B0/C0/D0/E0. All2106Host outputs independently decoded
and source-correspondent. First receiver401AU capture was intentionally disabled;
its CRC/length/ACK records pass. All1506post-reconnect captured AUs byte-identical
to Host and independently decoded/content matched. Do not claim a separate first
receiver bitstream decode. Subsequent transport begins at valid IDR/SPS/PPS.

Receiver restart: new random session,HELLO/CAPABILITIES sequences1/2 beforeFRAME3,
first accepted frame149735,flags15.38dependent frames explicitly skipped after
READY;first valid recovery frame805.5547ms afterREADY. No old-session replay or
partial-byte reuse. Same Host process across receiver termination/restart; retained
helper/driver processes andsame source epoch. Fixed receiver-first shutdown branch
executed live in reconnect,then Host exited0;final DRAIN/DRAIN_ACK transcript passes.

Hardware-only AMDh264Encoder/Radeon610M,D3D11-aware asyncMFT,DXGI manager andNV12
GPU input confirmed. No software fallback/full-frame normal-path readback.
Encoder p50/p95/p99/max:normal6.8/8.2/12.6/24.6932ms,reconnect7/27.8/31.8/35.4483ms.
Transport p50/p95/p99/max:normal0.3746/0.6101/0.6828/2.0097ms,reconnect
0.3685/0.6118/0.6801/2.6576ms. Normal10s-bin encodep95~8.1-8.3ms;reconnect initial
bin33.1ms then8.3-8.6ms,no accumulating latency. Initial interval cause unestablished.
Normal Host private87240704B flat afterstartup;reconnect85909504B flat final4samples.
New receiver884736B/83handles flat;driver returns35598336B/931handles afterboth.
Short-test stability only,not long transport soak/leak freedom. GPUVideoCodec0
samples mean/max16.43/19% normal,16.44/21% reconnect. Source cadence not60uniqueFPS.

All7health snapshots PnP0/0,SecureBootON,HVCION,TESTSIGNINGOFF,CiFlags62465,
same boot;relevant crash/bugcheck events0. No driver/helper restart,deployment,
certificate/trust,boot/security,phone/USB,visible rendering,commit orpush.
Updated STATUS/PHASE3C_RESULTS/PROTOCOL/USB_PROTOCOL/ARCHITECTURE/ROADMAP.
PHASE3B remains VERIFIED;historical E/UNKNOWN/failed controller runs preserved.
Hard stop after PHASE3C: no PHASE3D work authorized or started.

Final preservation PASS:1,231 earlier evidence files,five original binaries,
23 driver/shared files,108 prior3C evidence/build files,three content-v2 binaries,
43 flow-content-2 files and117 flow-content-3 files match SHA256 snapshots.
Final hygiene PASS:144 publishable files,index and125 history blobs checked;
git diff --check passes. Only the retained SweetDisplayDevice helper remains
running among project processes. No automatic further work or PHASE3D started.

## 2026-09-19 — PHASE 3D component implementation and live preparation: PARTIAL

Continued current worktree without replacing the VERIFIED3C protocol, Host,
encoder or endpoint. Added a separate visual simulator build and Visual test/
verification adapters. `Build-VisualSimulator.cmd D:` final build7 passes /W4/WX.
Initial link failure (missing wmcodecdspuuid.lib) and occluded replay failures
remain in ignored evidence; no historical result was promoted or overwritten.

`Test-VisualComponents.ps1 -RunName components-2`: PASS_COMPONENTS. Real recorded
fixture200 received/submitted/decoded,196 accepted Presents,4 controlled minimized
frames,18 sparse rendered nonce/counter matches to the independent decoder.
Two fresh sessions and resize/minimize/restore pass. WM_CLOSE:61 received,
60 submitted,59 decoded,55 Presents,1 queue+1 decoder shutdown discard,4 minimized
frames,6 matching diagnostics, exit0. Corrupt SPS/PPS expected rejection passes.
Hardware path: Microsoft H264 MFT with non-null DXGI manager, AMD hardware D3D11,
actual IMFDXGIBuffer NV12/default/decoder-bound surfaces, zero CPU access;
no manager-detach/CPU/software fallback. Runtime surface proofs recorded.

`Test-VisualNetworkReplay.ps1 -RunName network-reconnect-1 -Reconnect`: PASS.
Recorded source only, not fresh Display3:300=246 admitted+26 disconnected+28 resync;
246=245 ACKed+1 unconfirmed. WM_CLOSE old receiver65 received,64 submitted,
63 decoded/presented and exact two shutdown discards; new receiver180 of each
with17 rendered diagnostic matches. Queue peaks1/2/1; source sender stayed active.
`Test-ProtocolTransport.ps1 -RunName phase3d-regression-final-1`: PASS58,155 checks
and normal/slow transport. All409 initial protected hashes match after changes.

Read-only health: PnP0/0, HVCI1, CI status0/flags62465, Secure Boot state registry1.
Fresh direct firmware confirmation and live crash/security checks remain pending.
Tool token is nonadministrator; no RunAs, policy, deployment, signing, certificate
or phone action. `Invoke-Phase3DValidation.ps1` is prepared for manual Administrator
PowerShell:35s live normal, independent AU/source/render verification, then45s
live receiver close/reconnect only after the normal checks pass. Unique run paths
required. Its PASS_ARTIFACTS is not automatic PHASE3D VERIFIED: final independent
review and owner observation remain. See PHASE3D_RESULTS.md for exact limitations.

Final preparation checks: repository hygiene PASS156 publishable files,index and
125 history blobs; git diff --check PASS. Additional SHA256 comparison confirms
all1,231 earlier phase evidence entries and23 driver/shared baseline entries
unchanged. Current visual executable matches the exact components-2 tested binary.
Live names visual-flow-1 / visual-flow-1-normal / visual-flow-1-reconnect do not
exist; the controller refuses reuse. No Host/pattern/simulator test process was
left running. Existing device helper remains untouched. PHASE3D remains PARTIAL.

## 2026-09-19 — first real PHASE3D visual run reviewed: PARTIAL

Owner reports visibly moving simulator content after manual visual-flow-1.
Read-only review of existing evidence found controller ERROR at the unchanged
resource gate; live reconnect was NOT YET TESTED. No new run or deployment.
Fresh review folder visual-flow-1-review-1 stores owner observation, original SHA256
manifest, independent Review.ps1/review.json and unchanged-evidence confirmation.

35.0045096s:source1930/55.135753FPS,Host1493/42.651647FPS,encoded/received/ACKed/
independently decoded1487/42.480241FPS. Simulator1327 decoded,1326 accepted Presents
(37.880834FPS over Host duration),61 changing rendered nonce/counter/PTS markers
match independent AU decode AND original Display3 samples. Presentation span
32.6451542s;receipt-to-Present p50/p95/p99/max27.9957/62.5204/77.1868/144.4038ms.
Longest presentation gap1574.3798ms; it must not be hidden by frame latency stats.

All drop accounting matches. Upstream12producer+424stale+1pending;encoder6pressure;
receiver2overflow+150resync+6queue reset+2decoder reset+1stale-generation suppressed
presentation. Received1487=presented1326+161 explicitly separated nonpresented
frames. Queue peaks shared3/encoder1/receiver3/decoder2/render1;invalid,D,E0.
Microsoft H264 MFT produced1327 decoder-bound NV12 GPU surface proofs, no CPU
fallback. Receiver combined VideoCodec0 mean6.29%/max8%,3D10.86%/15% (7 samples);
dedicated VideoDecode1 was0, not a contradictory software-decode claim.

Original resource gate935->971 handles (+36>16) failed;private growth21082112bytes
is below32MiB. Early handle plateau and decoder-reset memory steps do not prove
a leak or prove absence of one. Cause UNKNOWN; do not relax thresholds. Both
receiver overflow/recovery episodes and late Host cadence slowdown need diagnosis.
Host/receiver/pattern exited0, no forced cleanup or relevant crash events;
all normal-run health snapshots PnP0/0,SecureBootON,HVCION,CI62465/TESTOFF.
Historical ERROR/evidence remains unchanged. No source/binary/security/phone
change and no reconnect test were performed during this review. Stop before3E.

## 2026-09-20 — PHASE3D diagnostic continuation (PARTIAL)

Continued the existing worktree and resource-investigation1 without rerunning
visual-flow1. Preserved current diagnostic sources/binary in
`phase3d/resource-investigation-2` before edits. No driver/Host/protocol/GOP,
certificate, boot/security or phone changes; no commit/push.

Commands/results (repository-relative; raw logs stay ignored):

- `Build-VisualResourceProbe.cmd D:`: /W4 /WX PASS; `ResourceProbe.exe` idle45s
  in `resource-idle-1`, then1700-frame primary plus8x120-frame lifecycle sessions
  in `resource-probe-2`, both exit0. Reviewed both old/new2660-frame probes
  independently against the preserved real-AU decoder oracle:116/169 rendered
  marker matches. Idle handles settle848; active probe2 settles937 before
  repeated resets, cleanup257/258/258. Original +36 cause still UNKNOWN.
- `Build-VisualSimulator.cmd D:` now writes a separate diagnostic-v2 output;
  protected v1 SHA256 remains2107C8031C8C9C5A3F4281CA421EB1DD04D6FE93B1C8D12242D84D71D804CD73.
  All five diagnostic build logs retained. No production queue or recovery rule
  was changed. Added bounded per-thread stage memory, async PSS/type observer,
  and later PeekMessage/dispatch timing. First successful Present has its own
  checkpoint; a failed/busy/occluded attempt is not counted as success.
- `Test-VisualStageReplay.ps1 -RunName stage-replay-1 -Count1700 -IntervalMs17`
  (arguments supplied with spaces): completed1700/1700 decoded/Presented,101
  marker matches, no overflow. Sleep quantization yielded32.1255FPS, so this is
  explicitly not the intended55–56FPS control.
- Recorded fixture pacing changed to a high-resolution waitable timer; live
  source was untouched. `stage-replay-2 -Count1700 -IntervalMs18`:55.5558FPS,
  one overflow on frame6 during first-frame Map30.724ms/render41.4874ms;
  first output call~31ms;2069.8957ms IDR wait separately.1700rx=1585admit+
  1overflow+114resync;1585=1582submit+3queue-reset;1582=1581decode+1decoder-reset;
  1581=1580Present+1stale-generation suppress.56 content checks PASS.
- `stage-replay-3` with same1700/18ms input and background PSS:55.5553FPS,
  two overflows at frames6/126, the latter in first render after reset rather
  than late steady-state;1700rx=1470admit+2overflow+228resync,1470=1464submit+
  6queue-reset,1464=1462decode+2decoder-reset,1462=1460Present+2stale-generation.
  52 markers PASS. UI Present max0.6495ms; Map max39.9891ms. Both IDR waits~2070ms
  are recovery waits, not decoder latency. No incomplete trace, decode fallback
  or unbounded queue. `Review-VisualStages.ps1` reports retained per-stage
  distributions, queue waits, HRESULTs and independently checked correspondence.
- `Test-VisualComponents.ps1 -RunName diagnostic-components-1 -StageDiagnostics`:
  ERROR `Two fresh GPU sessions and resize`; original200-frame criterion fails
  at49decoded. Two overflows overlap UI-pump calls65.18/62.41ms; maximum555.14ms.
  Concurrent protocol regression is a confound, not a proven root cause. This
  failure is preserved; no test criteria changed.
- Added PeekMessage/dispatch stage separation, then new
  `diagnostic-components-2 -StageDiagnostics`: PASS replay200decoded/196Presents,
  two sessions/resize and18marker matches; WM_CLOSE59decoded/55Presents,6markers,
  exact close discards/exit0; malformed AU exits1 as required. Timing bounds
  self-test detects3excess records at262144-row bound (26.79ns/record one run).
- `Test-VisualNetworkReplay.ps1 -RunName diagnostic-reconnect-1 -Reconnect
  -StageDiagnostics`: PASS recorded localhost fresh-process/session recovery,
  old84/new60decoded+Presented,6+4marker matches, no receiver overflow. Sender
  300seen=147admitted+50disconnected+103resync;147=146ACKed+1unconfirmed;
  final pending0. Live Display3 reconnect remains NOT YET TESTED.
- `Test-ProtocolTransport.ps1 -RunName phase3d-diagnostic-regression-1`:
  PASS58,155 protocol checks and normal/slow integration.
  `Test-TransportShutdown.ps1 -RunName phase3d-diagnostic-shutdown-1`:
  PASS9 shutdown policy cases.
- SHA256 manifests:60failed-run,409protected3C/source/binary,1231earlier evidence,
  23driver/shared,5earlier binaries all match. Hygiene PASS162publishable files,
  index and125reachable history blobs. Existing evidence/baselines not rewritten.
- Read-only health: adapter/monitor Problem0/0, one existing helper; SecureBoot
  registry1,HVCIState1,CIstatus0/flags62465/TESTSIGNING_OFF. Non-admin session
  cannot substitute for the administrator UEFI check in the next preflight.

PHASE3D stays PARTIAL. Both late-live stall attribution and full historical
+36handle attribution remain UNKNOWN; +16threshold stays unchanged. Diagnostic
v2 is prepared for **one**35-second real Display3 capture in unused
`visual-diagnostic-1`, requiring the owner's Administrator PowerShell because
the current process token is not elevated. No automatic RunAs, final acceptance,
live reconnect or3E starts. Next findings must justify any asynchronous sparse
readback/UI-thread fix; then repeat all acceptance gates. GOP/IDR-request work
remains deferred. Old soak remains UNKNOWN and PHASE3B/3C stay VERIFIED.

## 2026-09-20 — Live diagnostic1 review and asynchronous sparse fix

Owner ran the prepared35-second `Test-VisualTransport.ps1` command with
`-StageDiagnostics`, outcome PASS_TRANSPORT_SOURCE. No rerun. Original file
SHA256 manifest captured before review; all supplemental outputs are under
`phase3d/visual-diagnostic-1-review-1`. No old evidence overwritten.

VERIFIED measurements from that diagnostic (not PHASE3D completion):

- Duration35.0057902s;source1908/54.505269FPS,Host1673/47.792094FPS.
  Source accounting1908=1673+234stale+1contention;producer/pending0.
- 1673encoder inputs=1661accepted+12pressure;rate-limit0.1661outputs/sent/ACKed/
  received/independently decoded at both endpoints/GPU decoded/Presented;
  output47.449293FPS. All1661AU byte/CRC/SHA identities and decoded content match.
- 67rendered samples match source nonce/counter/timestamp. A1673,B/C/D/E0,
  invalid0. Receiver overflow/resync/reset/render drops0;queue2/2/1.
- Receive-to-Present p50/p95/p99/max24.5237/56.6302/64.9943/77.1391ms;
  longest inter-Present gap70.1676ms. Blocking diagnostic Map max34.017ms,
  ProcessOutput15.810ms,Present0.417ms. No IDR recovery wait in this run.
- PSS935first-Present->938plateau;type deltaALPC-2,Event+2,IoCompletion+1,Thread+2.
  Original unchanged +3s guard:+1handle/private-46,870,528bytes,PASS.
  Separate after5s receiver handle delta0/private+126,976bytes,Host+1/private0,
  driver0/private0. Short observation only, not a universal leak-free claim.
- Before/periodic/after SecureBootON,HVCION,CI62465/TESTSIGNING_OFF,PnP0/0,same
  boot. Host/receiver/pattern exit0,no forced cleanup/failure events.

Commands: existing `DecodeH264Evidence.exe` independently decoded original Host
and receiver inputs into new review directories, each exit0/1661frames.
`Verify-VisualEncoding.ps1`, `Verify-VisualTransport.ps1`,
`Verify-VisualRendering.ps1`, and `Review-VisualStages.ps1` passed against these
artifacts. Encoding/rendering verifiers gained a separate ReviewDirectory option
only; no acceptance thresholds were relaxed. Full metrics in PHASE3D_RESULTS.

Evidence-driven source change: eliminate synchronous64-pixel diagnostic Map
from simulator execution. GPU image path, H264 protocol/GOP and frame queue
policies unchanged. One64x1 staging slot is copied/tagged at render and polled
with DO_NOT_WAIT, not overwritten while pending; completed metadata is joined
to its original render row and independently checked. Poll-not-ready/slot skips
are explicit. Shutdown drains the bounded diagnostic copy with a deadline.
No normal-path full-frame readback, no software decode or encode fallback.

Build/test commands and new evidence:

- `Build-VisualSimulator.cmd D:`: /W4 /WX PASS into separate async-v3 output.
  v1/v2 executables unchanged. V3 hash:
  6A444B4B6C0ACB28D2B7F4C130B3C4AF1FC32D4C09281971941D01D49169FCCE.
- `Test-VisualComponents.ps1 -RunName async-components-1 -AsyncDiagnostics`:
  PASS200decoded/196Presents,two sessions/resize,minimize drops explicit,
  18markers;WM_CLOSE59decoded/55Presents,6markers;malformed SPS/PPS rejected.
- `Test-VisualSampleAssociation.ps1 -RunName async-association-1` using the
  component replay fixture:13controls PASS; valid evidence accepted,12 malformed
  association/accounting/deadline cases rejected. Only copied test fixtures
  were intentionally modified; originals were retained.
- `Test-VisualStageReplay.ps1 -RunName async-stage-replay-1 -Count 1700
  -IntervalMs 18 -AsyncDiagnostics`:1700received=decoded=Presented at actual
  recorded55.554831FPS;receiver loss0,queue1/2/1,60deferred content samples PASS.
  161Map polls/101not-ready;max poll0.0099ms,render12.6038ms;no blocking Map event.
- `Test-VisualNetworkReplay.ps1 -RunName async-reconnect-1 -Reconnect
  -AsyncDiagnostics`:PASS recorded fresh-session/socket recovery,84old+60new
  Presents,6+4markers;explicit50disconnected/103resync/1unconfirmed at sender,
  correct receiver shutdown/reset accounting. Not real Display3 reconnect.
- `Build-VisualResourceProbe.cmd D:` now preserves old probe output and writes
  `out/visual-resource-probe-async-v3`. `ResourceProbe.exe ... 1700 async` in
  `async-resource-1`:exit0,3renderer/9decoder sessions,2660decoded/Presented,
  165deferred samples independently match. First-session plateau938;cleanup
  handles257/258/258,final two cleanup private byte values194,195,456 and
  193,765,376. No continuing measured trend. Historical +36 cause still UNKNOWN.
- `Test-ProtocolTransport.ps1 -RunName phase3d-async-regression-1`:PASS58,155
  deterministic checks and normal/slow integration.
  `Test-TransportShutdown.ps1 -RunName phase3d-async-shutdown-1`:PASS9cases.

New final acceptance controller option `-AsyncDiagnostics` selects tested v3,
preserves existing runs, performs35s normal with independent decode/content/
resource/timing gates, and only then45s live reconnect. `visual-flow-2` is prepared
but NOT YET TESTED; Administrator token requires owner invocation (no automatic
RunAs). PHASE3D remains PARTIAL. No3E/phone/USB/touch/driver/cert/security work.
Historical visual-flow1 and failed component results retain their original
outcomes; old soak and exact historical late stall/+36 attribution stay UNKNOWN.

Final preservation check: all51visual-diagnostic1 files,60visual-flow1 files,
409protected3C/binary/source entries,1231earlier evidence,23driver/shared and
5earlier binaries unchanged. Hygiene PASS164publishable files/index/125history
blobs. Prepared visual-flow2 names are absent and current v3 hash matches the
tested component binary. Existing evidence is not reused for the new acceptance.

## 2026-09-20 — `visual-flow-2` gated live acceptance: technical PASS, owner observation pending

The owner-launched Administrator controller completed with `PASS_ARTIFACTS`.
It ran `visual-flow-2-normal` for35.0073816s and invoked the45.0037786s real
receiver reconnect only after normal live encode/decode/transport/render gates
passed. No rerun was started. All prior evidence remains in place.

Normal:1924source/54.959837FPS;1645Host/46.990090FPS;1642AMD hardware H.264
outputs/46.904393FPS. Exact source accounting is1924=1645Host+2producer+277stale;
encoder1645=1642output+3pressure. Transport1642encoded=sent=ACKed=received; the
Microsoft H.264 D3D11 decoder produced1642 decoder-bound NV12 surfaces and1642
accepted Presents.67/67 rendered markers match independent AU decode and source
nonce/counter/PTS. Receiver overflow/resync/reset/render drops0; queue peaks
shared3/encoder1/transport1/receiver3/decoder2/render1. Presentation span
34.8994229s;receive-to-Present p50/p95/p99/max24.6746/59.6961/63.9601/88.2125ms.
Post-startup resource gate:private-46,936,064bytes,handles+1. A1645,B/C/D/E0.

Reconnect:2445source/54.328771FPS;2091Host/46.462765FPS;2084hardware outputs/
46.307223FPS. Source2445=2091Host+1producer+351stale+2contention;
encoder2091=2084output+7pressure. Sender2084=1769admitted+222disconnected+
93resync;1769=1768ACKed+1unconfirmed. Planned WM_CLOSE old receiver remained
fully accounted; Host stayed active. Fresh receiver1244=1129admitted+1bounded
overflow+114resync;1129=1126submitted+3queue-reset;1126=1125decoded+1decoder-
reset;1125=1124Presented+1stale-generation suppress.47/47 fresh-process markers
match; final drain/exit0.

Stage evidence localizes the single reconnect overflow at frame292104 to the
bounded queue filling during first recovered frame292099's cold startup:
ProcessOutput max35.3122ms followed by21.1798ms render. The asynchronous sparse
poll max is0.0149ms and Present max1.9799ms. The decoder rejects114 dependent
frames and resumes on IDR frame292226 after2167.2629ms. This is explicit bounded
reconnect recovery; normal live operation has zero receiver loss. Fresh receiver
resource gate:private-40,747,008bytes,handles+5.

Independent review created only `phase3d/visual-flow-2-review-1`. Fresh offline
decodes pass for normal Host/receiver1642/1642, reconnect Host2084 and fresh
receiver1244. Encoding, transport byte/CRC/ACK identity, decoded-source/render
association and stage accounting checks pass. A pre-review SHA256 manifest of
167 existing run files rechecks with zero changes. Before/after both stages:
PnP0/0,SecureBootON,HVCION,CI62465/TESTSIGNING_OFF,same boot,relevant failure
events0. Host/final receiver/pattern exit0; clean encoder drain/shutdown.

Technical acceptance is complete. `OwnerVisibleObservationStillRequired` remains
true in the unchanged verifier output, so PHASE3D stays PARTIAL pending explicit
owner confirmation that normal motion was visible and rendering visibly resumed
after the receiver restart. Historical visual-flow1 +36/late overflow stays
UNKNOWN. No3E, phone, driver, certificate, deployment, security, commit or push.

## 2026-09-20 — `visual-flow-3` owner-observed final acceptance: VERIFIED

Owner requested a repeat solely to observe the windows. New identities
`visual-flow-3`, `visual-flow-3-normal` and `visual-flow-3-reconnect` were unused;
no previous evidence was overwritten. The unchanged async-v3 controller completed
`PASS_ARTIFACTS`, normal first and reconnect only after all normal gates passed.
Owner confirms real moving content in the normal window and moving content resumed
in the fresh receiver window after reconnect.

Normal35.0282436s:source1729/49.360168FPS;Host1280/36.541941FPS;encoder
1278/36.484844FPS. Source exact accounting:1729=1280Host+11producer+436stale+
2pending;contention0. Encoder1280=1278outputs+2pressure;rate0. Transport1278=
sent=ACKed=received. Receiver1278=1195admitted+1overflow+82resync;
1195=1192submitted+3reset drops;1192=1191decoded+1decoder-reset;1191Presented.
64/64 rendered samples match independent source nonce/counter/PTS. A1280,B/C/D/E0.

The normal overflow is fully classified. Windows message161 is
`WM_NCLBUTTONDOWN`, consistent with title-bar/window dragging. Its dispatch/pump
lasted1520.1875/1522.5653ms, filling the bounded three-frame queue.82dependent
frames were suppressed and the next IDR restored presentation after2112.2849ms.
Sparse poll max0.0208ms;decode-output max25.1916ms;render max29.217ms;Present
max10.1915ms. This is a bounded interactive-window continuity event, not silent
loss, unbounded growth, decode corruption or metadata failure. Post-startup
private-47,136,768bytes,handles+5; unchanged guards pass.

Reconnect45.0069927s:source2105/46.770510FPS;Host1569/34.861249FPS;encoded
1568/34.839031FPS. Sender1568=1281admitted+202disconnected+85resync;
1281=1280ACKed+1unconfirmed. Planned WM_CLOSE old receiver invalidated the old
session while Host remained active. Fresh process/session captured, independently
decoded and GPU decoded848 frames;847 accepted Presents+1explicit busy Present;
overflow/resync0,queues2/2/1,48/48 source matches,private-47,562,752bytes,
handles+1,clean final drain/exit0.

Owner perceived approximately15–20s between windows. QPC evidence measures
8660.8007ms from old shutdown to new first accepted Present:5621.373ms until new
process start (intentional three-second absence plus health-query/controller work),
105.6042ms renderer creation,489.5134ms handshake readiness,2432.6214ms wait for
the next valid SPS/PPS+IDR,117.2929ms IDR-to-Present. Last old Present to first new
Present is8764.1222ms. This is retained as a material reconnect-latency limitation;
no maximum recovery-delay criterion or test threshold was altered.

New `visual-flow-3-review-1` performs fresh independent decodes:normal Host/
receiver1278/1278,reconnect Host1568,fresh receiver848. Encoding, transport
sequence/ACK/CRC/byte identity, render/source association, stage and exact-accounting
reviews pass. SHA256 recheck finds zero changes among167 original run files; all
three retained simulator copies match the tested async-v3 binary. Before/after:
PnP0/0,SecureBootON,HVCION,CI62465/TESTSIGNING_OFF,same boot,failure events0.
Hardware AMD encode and Microsoft H264 MFT D3D11/NV12 decode remain active with
no software fallback or normal-path full-frame CPU readback.

PHASE3D VERIFIED. Historical visual-flow1 +36 handles/late overflow remain UNKNOWN;
visual-flow2 remains valid technical acceptance evidence without owner observation.
PHASE3B/3C stay VERIFIED. Hard stop honored: no3E/touch/USB/phone work, driver/
certificate/security change, commit or push.

Final `git diff --check` passes with only normal LF/CRLF notices. Public repository
hygiene passes for166 publishable working-tree files, the index and125 reachable
history blobs. Private raw video/evidence and build outputs remain ignored.

## 2026-09-21 — DEVICE PHASE 2C-T real touch transport: PARTIAL

Separately authorized device work used the existing ordinary receiver and
ephemeral ADB forward only. It added negotiated profile-1 touch messages, with
bounded normalized coordinates, contact state, pressure, active-mask and device
timestamp validation. The Windows host discovers its active SweetDisplay target
at session start and binds configuration to an opaque topology token; it does
not hard-code a monitor number or desktop coordinates.

The completed injection run had clean Host/pattern exits, one ready connection,
zero protocol errors, 103 real touch messages, clean video classification and
`mtp,adb` before/after. Independent target evidence records expected down/move/up
lifecycles for the tested controls and two separately released contacts. The
operator completed the requested live phone checks.

The controlled reconnect run stopped the receiver while one touch was active and
then started a fresh process. It recorded two ready connections, zero protocol
errors, clean video classification and unchanged `mtp,adb`. The independent
target recorded terminal UP, and the fresh session was established. The protocol
suite now explicitly rejects a validly formed retired-session touch message after
a new touch handshake. `Build-TouchTools.cmd` and `ProtocolTests.exe` pass
58,161 deterministic protocol/parser/queue/resync checks.

The target-side release is a safety-positive observation, but the host release
path logged timeout then invalid-parameter results while submitting the release
update. Local contact state was cleared; the API acknowledgement is still
ambiguous. Thus Phase 2C-T is PARTIAL: real single/two-contact mapping and
reconnect session invalidation are verified, while universal disconnect release
acknowledgement remains UNKNOWN. Failed and successful raw evidence remain
ignored/private. No USB gadget/HID, ConfigFS, root, remount, AVB, partition,
Fastboot, driver/security change, commit or push occurred. Hard stop after 2C-T.

## 2026-09-21 — DEVICE PHASE 2C-T1 disconnect release reliability: VERIFIED

Preserved `inject-reconnect-1` without modification. Its `INJECT_RETRY 1460`
was captured immediately from a failed `InjectTouchInput` call. The subsequent
`RELEASE_ERROR 87` was recorded by the old catch path only after exception
unwinding and therefore cannot be attributed to a second injection call. This
conclusively localizes the old invalid-parameter ambiguity to defective error
capture; the historical 2C-T result remains PARTIAL.

Audited ownership found initialization on the main thread but injection/teardown
on the transport worker, no stationary UPDATE cadence, conflated desired and
last-delivered positions, and local state clearing after attempted release. The
corrected production state machine makes the transport worker sole API/release
owner, separates desired/delivered state, emits 50ms keepalives, performs
all-active UPDATE then matching UP, captures each API result immediately, retries
only `ERROR_NOT_READY` twice at 2ms, and preserves/blocks on uncertain release.

`t1-local-4`: all nine required scenarios ran20 times.180/180 ended active0 and
release-clean; API ledger921 rows/0 failures/0 retries; independent target220
DOWN/220UP;120 teardown `RELEASE_ALL`;0 uncertain. Earlier setup failures remain
preserved and were not treated as touch results.

Real evidence combines accepted cycles1–7 from preserved `t1-phone-2` with the
bounded `t1-phone-3 -StartAt 8` continuation: three single DOWN, three MOVE and
three two-contact active disconnects, then normal Host shutdown under one active
contact. Every accepted reconnect created a fresh session and required a fresh
tap. Combined API ledgers917 rows/0 failures/0 retries; independent target reached
two contacts and both runs ended active0. `t1-phone-3` PASS:Host/pattern exit0,
connections3,protocol errors0,pending0,queue peak3; one bounded overflow,four
queue aborts and one unconfirmed frame around intentional reconnects were fully
accounted. Hardware-only encode, A-only classification, no software fallback,
clean drain. USB before/after `mtp,adb`.

`Build-TouchTools.cmd D:` PASS and `ProtocolTests.exe` PASS58,161. Before/after
health: same boot,SecureBootON,HVCION,display/monitor OK/problem0. Raw logs,
topology/session values and compressed evidence remain ignored/private. Public
documents contain no unique device identifier. No Android privilege/install,
Fastboot/root/remount/AVB/partition/SELinux,ConfigFS/NCM/HID/UDC,driver/cert/
Windows-security,commit or push action. DEVICE PHASE2C-T1 VERIFIED; hard stop
before2C-1/3E.

## 2026-09-22 — DEVICE PHASE 2C-PERF sustained cadence localization: VERIFIED

Added bounded one-second Windows/Android interval telemetry without changing
codec,bitrate,queue,ADB,touch,driver,display or USB policy. Android reports
process CPU/memory,public thermal status,battery temperature and refresh rate.
The controller aligns counters by observed session epochs and never subtracts
Windows QPC from Android time. Raw evidence remains ignored/private.

`perf-baseline-1` completed900.0397067s at2400x1080,nominal60FPS,30Mbps:
source49374/54.857580FPS versus Host25212/28.012097FPS. Encoder25212input,
25206accepted=output,6explicit pressure drops,28.005431FPS;hardware-only AMD,
conversion avg/peak3.222518/67.948700ms,encoder latency avg/p50/p95/peak
27.314055/28.4/34.1/49.6369ms. Transport25206admitted=wire=ACKed,queue peak1,
socket/protocol/overflow/expiry/pending0. Android Qualcomm hardware decoder
25206input=output,25046render callbacks,CRC/sequence/session/protocol/decoder/
queue-overflow errors0. USB before/after `mtp,adb`; clean drain/exits.

The observed transition starts at second7. The corrected adjacent-stage detector
uses a10s rolling downstream/upstream ratio below80% for15consecutive samples:
the first IddCx-source→Host-admission window ends at second11 and is confirmed at
second25. Seconds0–6 average source53.81/Host52.86/encoder-output52.57FPS and
7.08ms encoder latency;seconds7–14 source52.70/Host30.50/output30.25FPS and
25.89ms;seconds15–75 source54.93/all downstream approximately28.9FPS and27.47ms;
seconds840–899 source54.55/all downstream approximately24.83–24.91FPS.

Controls: `perf-source-control-1` source54.281420/Host54.123116FPS;
`perf-encode-control-1` source54.050957/Host50.611108/output50.381118FPS,
latency7.236316ms; `perf-local-transport-1` source54.923986/Host51.807377/
output51.515716/sender51.449944FPS,latency7.392955ms,6182sent=ACKed,queue1.
Local send-start→ACK mean/p950.521/0.776ms.

`perf-android-repeat-1` then reproduced the real-target result in120.0199471s:
source53.924370 versus Host/encoder26.228973FPS; observed onset second3,rolling
boundary second9,confirmation second23. Encoder latency27.154860ms;
3148encoded=sent=ACKed,queue2,errors0;Android3148decoder input=output and3111
final render callbacks. Real-target send-start→ACK mean/p95 is6.080/7.765ms.
USB again remained `mtp,adb` and thermal status stayed0.

Host private bytes38,883,328→83,480,576,peak86,306,816;handles1023→1269,
peak1279;threads73→39. Growth occurs during initialization and plateaus before
the sustained slow interval; no leak claim or slowdown correlation. Android PSS
57,921→68,454KiB,peak98,662KiB;battery33.3–34.2C;thermal status0. Late-window
HostCPU15.47% of one core,sampled GPU-engine sum10.62%,AndroidCPU25.31%;no
observed saturation or thermal-throttle evidence.

DEVICE PHASE2C-PERF VERIFIED: degradation is reproducibly localized to IddCx
source→Host admission when the real ADB/Android target is active. Every later
stage follows the reduced rate. Whether the mechanism is Windows ADB/socket
activity,Android receiver scheduling,Host transport-sink mutex/evidence work or
encoder-event pump delay remains UNKNOWN. Proposed next separately authorized
work is per-call Host micro-timing plus an ACK-only Android control, followed by
a bounded sink-worker correction only if proven. No fix,2C-1,3E,NCM,HID,
driver/security/boot/partition operation,commit or push. Touch was idle; its
lifecycle was not modified,2C-T stays PARTIAL and2C-T1 stays VERIFIED.

## 2026-09-22 — DEVICE PHASE 2C-PERF1 Host/ADB micro-timing: PARTIAL

Added opt-in bounded QPC instrumentation without changing normal defaults. The
750,000-entry preallocated buffer uses fixed 48-byte records and one shutdown
write. Accepted traces contain506,212–544,526 records with zero loss; measured
record-operation calibration mean20.060–20.745ns,p95100ns. Timed scopes cover
Host acquisition/admission/keyed mutex,conversion,encoder input/pump/output,
frame construction,transport queue/locks,socket I/O,evidence flush,ACK receive/
accounting and complete Host iterations. Analyzer output and raw evidence remain
ignored/private.

Three setup attempts are preserved rather than overwritten. `perf1-normal-full-1`
stopped before Host launch when an elevated controller could not use the existing
non-elevated ADB server. A bounded host ADB-server restart recovered that hang.
`perf1-normal-full-2` stopped before Host launch on PowerShell argument binding;
the controller now uses explicit argument arrays and a15s ADB deadline.
`perf1-normal-full-3` ran at source/Host56.299418FPS but a cold MediaCodec start
caused one queue overflow/reconnect and its then-500,000-record trace dropped
40,935 tail records, so it is excluded from acceptance.

Accepted `perf1-normal-full-4` ran120.001s: source56.298136FPS,Host/encoder
56.256470FPS,sender56.169946FPS,6,751 accepted frames,queue peaks shared3/
encoder1/transport2. Android added one clean drain and6,751 decoder outputs with
zero protocol,CRC,sequence,session,queue-overflow or decoder-error delta. Whole
Host iteration mean/p50/p95/p99/max4.8765/3.4191/10.2635/11.1135/34.0886ms;
frame-bearing iterations4.2654/4.0485/7.6559/11.0534/34.0886ms. No recurring
27–30ms Host-admission block appears.

The normal full-evidence run made6,873 nonblocking socket writes totalling
422,743,612bytes; every requested write completed, with zero partial writes,
would-block retries or write-select timeouts. Logical send p50/p95/p99/max was
0.1335/0.2147/0.2490/1.0433ms. Same-QPC send-to-ACK mean/p50/p95/p99/max was
5.1094/4.7020/7.5748/14.4585/34.5059ms. ACK depth peaked at one by worker policy.
Host Consume and sent-state mutexes had zero definitely contended acquisitions;
four ACK-state contentions totalled0.3802ms,max0.1173ms. Keyed-mutex max was
0.3956ms with no retry-marked contention.

`perf1-normal-reduced-1` buffered per-record transport/protocol flushes and ran
source56.365343FPS,Host56.290345FPS,sender56.203846FPS with zero trace/error loss.
The +0.033875Host-FPS difference versus full evidence is approximately0.060%,
so current evidence flushing is not the missing frame-period-scale loss.

`perf1-ack-only-1` used the real Android socket/parser/payload/CRC/session/
sequence/telemetry-ACK path without constructing MediaCodec or Surface. It ran
source56.323793FPS,Host56.290461FPS,sender56.215967FPS and validated/ACKed6,755
frames with decoder input/output0 and all error deltas0. Send-to-ACK mean/p95 was
3.9800/4.5458ms. Normal versus ACK-only Host differed by0.033991FPS; decode/
Surface is not a current limiter, though its role in the historical state remains
UNKNOWN because neither control reproduced that state.

An additional uninstrumented `perf1-legacy-host-control-1` used the earlier
`transport-host-content-v2` binary and normal receiver. It sustained source
56.391191FPS,Host/encoder56.382858FPS,sender56.301573FPS, proving PERF1 timing did
not mask the historical slowdown. No300s extension was run because this direct
old-binary120s control also remained fast.

DEVICE PHASE2C-PERF1 is PARTIAL. The historical26–28FPS degradation did not
reproduce after the host ADB-server restart, including with the old uninstrumented
Host. That restart is the only adjacent observed host-state discontinuity, but
without a degraded micro-trace it is correlation, not proof. In the current
state, ACK receive, socket send, transport/keyed locks and evidence flushes do
not block Host admission. The exact degraded-state first blocker remains UNKNOWN,
so no correction was implemented. The smallest future separately authorized step
is to arm this bounded trace before any ADB/app restart and freeze it only when a
rolling Host/source degradation trigger fires.

Android APK installation used ordinary authorized `adb install -r`; the APK has
only normal INTERNET permission. USB remained `mtp,adb`. No root/su,remount,
SELinux,Fastboot,flash,partition,AVB,ConfigFS,FunctionFS,NCM,HID,UDC,driver,
certificate,trust-store or Windows boot/security operation occurred. Touch paths
were not changed;2C-T stays PARTIAL,2C-T1 and2C-PERF stay VERIFIED. No commit or
push; hard stop after2C-PERF1.

## 2026-09-22 — DEVICE PHASE 2C-PERF2 triggered flight recorder: VERIFIED

Added an opt-in bounded flight recorder around the PERF1 QPC event model. It
uses fixed48-byte records, a preallocated200,000-record rolling pre-ring and a
350,000-record post area. No per-event disk write occurs. The detector samples
monotonic counters over approximately10s and triggers once only when source is
at least45FPS and Host/source is below0.75. `FlightRecorderTests.exe` passes74
synthetic checks covering healthy,matching slow source,transient,50%-Host,
single trigger,pre-ring,post bound and fixed capacity.

Setup attempts1–6 are privately preserved and excluded: they stopped on
controller/ADB-context,receiver-command or secure-desktop pattern setup faults.
Attempt7 stayed healthy for roughly10minutes but hit the initial fixed transport
evidence-row cap; it is also excluded. The flight-recorder-only row capacities
were raised to fixed400,000/250,000 values before the accepted run.

`perf2-observe-8` ran the normal2400x1080,nominal60FPS,30Mbps,AMD hardware AVC,
ADB-forwarded TCP,Qualcomm hardware decode/Surface path. It triggered and stopped
cleanly after1664.569967s Host observation. Whole-run source/Host was
55.595741/53.878180FPS,ratio0.969106249. The10.195552s detector window measured
source52.179618,Host37.761565FPS,ratio0.723684211. The60.070676s post interval
measured3122 source/1227 Host frames:51.972114/20.425940FPS,ratio0.393017297.
The trace retained61.360385s pre and60.070676s post,278,084 records from
6,342,368 observed events,zero loss. Calibration mean/p95/max was
21.385/100/6500ns; final write6.812ms.

Healthy-pre versus degraded-post p50/p95/p99/max milliseconds:

- frame Host iteration7.7342/33.3448/56.3944/185.0200 versus
  48.4691/69.4523/82.6407/102.8562;
- main-loop encoder pump0.0032/7.5853/8.2719/173.4104 versus
  27.1339/34.2986/35.2206/49.0152;
- MFT `HaveOutput` event service6.1213/7.4794/8.1968/172.8232 versus
  26.4200/33.7217/34.4915/48.2868;
- conversion wait1.1522/9.0519/24.2672/48.4805 versus
  9.8306/22.6629/31.9610/47.1359;
- output total0.4440/0.7538/0.9869/4.3286 versus
  0.6715/0.9100/1.0091/1.1691;
- socket write0.1429/0.2196/0.2631/0.3916 versus
  0.1417/0.2153/0.2627/0.4561;
- ACK receive5.4816/7.2254/9.4461/115.2421 versus
  5.7579/7.3930/9.7144/20.3989.

In the first slow one-second bin, main-loop pump median became20.7086ms and MFT
`HaveOutput` event service became26.6056ms while Submit-internal pump stayed
0.0025ms and conversion-wait median stayed1.2410ms. Frame iterations fell from
52 in the preceding bin to27, and the next Host counter interval fell from57 to
31 admissions while source stayed healthy. The event-service timer encloses
`GetEvent(MF_EVENT_FLAG_NO_WAIT)` plus event status/type inspection and ends
before output processing. This is the first sufficiently narrow changed wait;
the internal three-call split and reason for the AMD MFT transition remain
UNKNOWN. No fix was implemented.

Accepted final accounting: encoder89684 input,89675 accepted/output,nine explicit
pressure drops,hardware-only/GPU-surface/no fallback/clean drain. Transport89561
sent,89560 ACKed,zero socket/protocol errors,pending0. Android89558 decoder
input/output,zero protocol/CRC/sequence/session/queue-overflow/decoder errors,
thermal status0. A setup connection ended1604.21s before the trigger; the active
connection remained through degradation, so that reconnect is not the immediate
trigger. The existing ADB process/listener was unchanged at before/trigger/
capture-complete/after snapshots; it was not killed or restarted. USB stayed
`mtp,adb`.

Final protocol regression passes58,235 deterministic checks and the PERF2
synthetic suite passes74. Builds use `/W4 /WX`. Raw traces/video/logs/topology
and device evidence remain ignored/private; public files contain no unique
device identifier. No root/su/adb-root,remount,SELinux,Fastboot,flash,partition,
AVB,ConfigFS,FunctionFS,NCM,HID,UDC,driver/certificate/Windows-security,commit or
push action. DEVICE PHASE2C-PERF2 VERIFIED / TRIGGERED; hard stop before any
performance correction,2C-1,3E,NCM or HID work.

Final `git diff --check` passes with only normal LF/CRLF notices. Publication
hygiene passes for210 publishable working-tree files,the index and125 reachable
history blobs; all9 checker fixtures pass. The connected-device identifier has
zero matches in publishable paths,private PERF2 evidence is ignored and no
firmware/image/archive extension is tracked.

## 2026-09-22 — DEVICE PHASE 2C-PERF3 encoder isolation: VERIFIED

Microsoft asynchronous-MFT and media-event documentation was audited before
changing ownership. Exactly one MTA worker now owns MFT activation/config,
`GetEvent`,event status/type,`ProcessInput`,`ProcessOutput`,tokens,pending map,
drain,end-streaming and shutdown. Host admission retains shared-frame acquire,
bounded GPU synchronization,BGRA-to-NV12 conversion,GPU sample construction and
a fixed enqueue only; it no longer services `HaveOutput`. New input capacity is4
with reject-newest accounting. The existing sender capacity remains3. Four
private NV12 surfaces are released only by `IMFTrackedSample` callbacks; no CPU
copy,codec switch or software fallback was added.

`perf3-windows-local-1` passed120.001892s with a planned receiver disconnect/
restart. Source/Host was55.699122/54.865801FPS,ratio0.985038884. The worker was
slower at43.307650FPS and caused1387 explicit bounded input drops, but Host
admission stayed healthy; input/output queue bounds were4/3,all pending counts
ended0,and drain/shutdown/reconnect passed. This directly demonstrates that
worker slowness no longer forces Host admission to the same cadence.

The first real short attempt is retained and excluded: a cold Qualcomm
MediaCodec start filled its four-entry decoder queue, exactly matching the cold
start issue recorded in PERF1. A narrow first-frame session-ready backpressure
was added without increasing capacity or changing later asynchronous operation.
The rebuilt,signed development APK SHA-256 is
`63c90dd9388a5a96074162a7474342423aaae22f0e2bfdb35a045ee0be4744cd` and ordinary
authorized `adb install -r` succeeded. `perf3-phone-short-6` then passed120.000310s
at source/Host53.316529/51.483200FPS,ratio0.965614247,with6178/6178 encoder
input/output,zero Android protocol/queue/decoder error and one clean drain.
`perf3-phone-short-7` repeated the counter and visual gate at ratio0.968361405;
the owner confirmed the moving pink/black controlled test pattern.

`perf3-phone-sustained-1` passed1800.004732s. Source was53.358749FPS (96046),
Host51.527087FPS (92749),ratio0.965673. Upstream gaps account exactly as346
producer+2920 Host-queue+31 contention=3297. Encoder input92749 became92489
outputs+260 explicit capacity4 backpressure drops; final pending0. Transport
seen/admitted/wire/ACK was92489/92373/92370/92369 with110 resync,one overflow,
three shutdown aborts,one unconfirmed and pending0. Android received/decoded
92370,rendered78416 callbacks,with zero protocol/CRC/sequence/session/overflow/
decoder errors and one clean drain. PERF2 detector observed7,833,998 events,
lost0 and did not trigger.

Sustained MFT `HaveOutput` worker timing mean/p50/p95/p99/max was
4.505122/4.5/6.9/7.8/35.3408ms. Host iteration was
13.441489/11.8/28.9/38.8/2599.4449ms; the retained isolated maximum did not
produce a rolling trigger or backlog. Battery changed32.8C to35.2C,thermal
status stayed0,and measured resources remained bounded. USB remained `mtp,adb`.

`perf3-touch-regression-2` passed the bounded real-phone Host regression with
one single-finger press,drag and natural release. Android accepted/sent100 touch
messages,Host validated100 contact events,and the independent Windows target
observed118 pointer events. Android active mask,Host reconstructed active set and
target reconstructed active set all ended0; touch overflow and protocol-error
deltas were0. Concurrent source/Host was54.948757/54.215440FPS,ratio0.986654530,
with no degradation trigger and USB `mtp,adb`.

Rebuilds pass `/W4 /WX`; encoder-worker tests pass23,protocol regression58235
and flight-recorder regression74. DEVICE PHASE2C-PERF3 is VERIFIED. No2C-1,3E,
NCM,HID,camera,root,Fastboot,partition,driver,security,commit or push action
occurred.

Final `git diff --check` passes with only normal LF/CRLF notices. Publication
hygiene passes for214 publishable working-tree files,the index and125 reachable
history blobs; all9 checker fixtures pass. The connected-device identifier has
zero matches in publishable paths,private PERF3 evidence is ignored and no
firmware/image/archive extension is tracked.

## 2026-09-23 — DEVICE PHASE 2C-1 native USB architecture gate: VERIFIED

Performed repository-first and then read-only exact-device/Windows inventory.
No prior performance test was repeated. The phone remained stock Android13/MIUI
`V14.0.2.0.TKFTRXM`, kernel `4.14.190-perf-g6d6db67fd446`, SELinux Enforcing and
USB `mtp,adb` with running ADB before/after every query.

Runtime config confirms DWC3/MSM gadget, ConfigFS, FunctionFS, NCM and HID. ACM,
ECM, RNDIS, EEM and UVC ConfigFS functions are disabled. ConfigFS is mounted,
but shell reads of `/config/usb_gadget` and UDC speed/state are DENIED. FunctionFS
ADB and Qualcomm diagnostic mounts are active; no `/dev/hidg0` or USB network
interface exists. Framework reports MTP, configured/connected, Gadget HAL
unknown, USB HAL V1.0 and speed unknown. The only exposed HAL manifest is
`android.hardware.usb@1.0::IUsb/default`; vendor init owns composition through
`usb_control_prop` properties and ConfigFS.

Exact `init.qcom.usb.rc` evidence is decisive: it creates `functions/ncm.0` and
defines complete `ncm` and `ncm,adb` branches. The latter starts/retains ADB,
links NCM plus `ffs.adb`, binds `a600000.dwc3` and publishes `ncm,adb`. No stock
HID composition or SweetDisplay FunctionFS instance exists. Thus NCM is not only
compiled but stock-init-supported; its live enumeration remains UNKNOWN.

Windows11 Pro build26200 has inbox `usbncm.inf`/`usbncmum.inf`, matching CDC
subclass0Dh and `USB\MS_COMP_WINNCM`; inbox WinUSB and HID drivers are also
present. Current PnP shows ADB only and no NCM adapter. Microsoft/AOSP/Linux
primary documentation was cross-checked for NCM, WinUSB OS descriptors,
FunctionFS/ConfigFS and Windows Touchscreen HID requirements.

Architecture decision: first native prototype should use the exact stock
`ncm,adb` composition and carry the unchanged TCP SweetDisplay stream. Direct
FunctionFS vendor bulk remains a later fallback/optimization requiring a new
interface, OS descriptors, init/SELinux integration and one bulk pair. HID is a
later optional touch-only interrupt interface. Existing OEM diagnostic endpoints
are rejected. A custom kernel is not required; level2 system/init ownership is
the first practical boundary. Level3 temporary userspace is only a fallback.

Bandwidth analysis: raw2400x1080BGRA at56FPS is4.645Gbit/s and impossible on
USB2; encoded target is30Mbit/s. Current frame/ACK/touch application overhead is
about0.164Mbit/s at56FPS plus120 touch messages/s. A33.2Mbit/s NCM planning load
includes10% headroom. Use120Mbit/s as a conservative future High-Speed payload
budget rather than480Mbit/s signalling; current negotiated speed is UNKNOWN and
Full-Speed is an immediate stop result.

Selected next experiment, NOT EXECUTED: a level2 enumeration-only temporary
request for the stock `ncm,adb` branch, preserving an owner-present recovery path
and restoring exact `mtp,adb`. No IP, video, driver installation, firewall rule,
HID or direct ConfigFS action belongs in that experiment. Success requires inbox
NCM+ADB enumeration, ADB reconnect, no persistent property and verified rollback;
otherwise stop without bypass.

Created `docs/device/DEVICE_PHASE2C1_ARCHITECTURE.md`; updated STATUS, ROADMAP,
USB_GADGET_PLAN and this log. Public text intentionally contains no serial,
USB-instance path, session value or other unique device identifier. No raw device
output was added. Proprietary firmware/images and private evidence remain ignored.
No ConfigFS/property write, service start/stop, UDC action, ADB restart, NCM/HID
activation, driver install, root/su/adb-root, remount, SELinux, Fastboot,
partition/AVB, boot-image, commit or push action occurred. Nothing was written to
the phone. Final `git diff --check` and publication checker pass:215 publishable
paths,the index and125 reachable history blobs checked. Prohibited public paths,
tracked firmware/image/archive payloads,personal raw paths and public matches for
the privately read connected-device identifier are all zero. Only
`docs/evidence/README.md` is tracked under evidence. Final device state is
authorized,`mtp,adb`/`mtp,adb`,with ADB running. Hard stop after DEVICE PHASE2C-1.

## 2026-09-23 — DEVICE PHASE 2C-1E1 stock NCM+ADB enumeration-only: DENIED

Read the authoritative phase documents and captured a sanitized baseline before
mutation. The phone was awake and exactly `mtp,adb`/`mtp,adb`,ADB was `device`,
`adbd` was running,there was no ADB forward or active phone/USB test,and Windows
showed healthy problem-code-0 inbox `WINUSB` ADB plus `WUDFWpdMtp` MTP devices.
No NCM PnP device or network adapter was present. Primary same-path rollback and
the normal-stock-reboot fallback were established before the request.

At2026-09-23T00:17:27.560+03:00 the only authorized stock system-owned request,
`svc usb setFunctions ncm`,was attempted once. It returned255 and MIUI emitted
the already observed missing-theme compatibility diagnostic. During the bounded
observation,`sys.usb.config` and `sys.usb.state` remained `mtp,adb`,ADB/adbd stayed
available,no NCM network interface appeared on the phone,and Windows enumerated
no NCM PnP/network device. No `UsbNcm` service bound. USB speed remained `-1` /
UNKNOWN. Runtime Gadget HAL remains `unknown`,and the stock `svc` help advertises
NCM only when Gadget HAL1.2 is supported.

Because the legitimate framework path did not select the dormant vendor init
branch,the experiment is DENIED at its authorized privilege boundary. No direct
USB-property,ConfigFS,manual function,UDC,root or level3 bypass was attempted.
This does not prove that the ROM's NCM descriptors would fail Windows enumeration;
they were never presented to Windows.

Mandatory rollback began at2026-09-23T00:19:13.296+03:00 with
`svc usb setFunctions mtp` and succeeded. Final state was `mtp,adb`/`mtp,adb`,
`adbd` running and ADB `device`; ADB/MTP PnP health remained OK/problem0,active
NCM counts were zero,persistent system/vendor USB properties were unchanged,and
the third-party Windows network-driver inventory was byte-for-byte unchanged.
The reboot fallback was not used. No IP,DHCP,route,metric,profile,firewall,ping,
TCP,SweetDisplay,payload or throughput action occurred.

Created `docs/device/DEVICE_PHASE2C1E1_RESULTS.md`; updated STATUS,ROADMAP,
USB_GADGET_PLAN and this log. A sanitized transcript is retained only in the
ignored private evidence tree. Public text contains no device serial,USB instance
identity,MAC address or machine-specific unique identifier. No raw evidence or
firmware/image/archive was made publishable. No driver install/update,ADB-server
restart,root/su/adb-root,remount,SELinux,Fastboot,partition,AVB,bootloader,kernel,
FunctionFS,HID,camera,commit or push action occurred. Hard stop after2C-1E1.

## 2026-09-23 — DEVICE PHASE 2C-1E1A stock NCM control-path diagnosis: VERIFIED

Performed read-only analysis only. The phone began and ended `mtp,adb` / `mtp,adb`
with ADB `device`; no USB composition request or property write was issued.
Exact Xiaomi `svc.jar`,framework/services JARs and Settings APK were inspected
locally from the ignored private evidence tree. Pinned Android13 AOSP sources
were used only to explain matching framework and SELinux behavior.

The exact `svc` client accepts `ncm`,maps it to bit1024 and calls
`IUsbManager.setCurrentFunctions`. Runtime has no Gadget HAL and explicitly uses
`UsbHandlerLegacy`; HIDL `android.hardware.usb@1.0::IUsb/default` is the port HAL,
not a gadget HAL. The legacy handler adds ADB,sets transient `sys.usb.config`,and
the existing Qualcomm init branch links NCM first plus `ffs.adb` second before
UDC binding and state publication. Shell has MANAGE_USB for the Binder call;
`system_server` owns `usb_control_prop`,while init/vendor-init owns the predefined
property-triggered ConfigFS transition. Direct shell property writes were not
tested and remain prohibited.

Retained E1 runtime records show `Setting USB config to ncm,adb`,USB disconnect /
connect / configured events and authorized ADB reconnection,with no framework
failsafe record. About four seconds later a distinct `Setting USB config to
mtp,adb` request occurred. Xiaomi `UsbModeChooserReceiver` had opened
`UsbDetailsActivity`,and an input-interaction record immediately precedes the MTP
request. Exact Settings bytecode confirms this activity can call
`UsbManager.setCurrentFunctions`; the exact clicked row is UNKNOWN. Exit255 is
therefore the host adb-shell transport ending during USB re-enumeration,not proof
of NCM parsing,HAL,property or init rejection.

Classified 2C-1E1A VERIFIED because the control/ownership chain and overwrite
source are sufficiently localized. Preserved 2C-1 VERIFIED and historical
2C-1E1 DENIED. Existing legitimate stock Level2 control is accessible; custom
kernel remains unnecessary. Windows NCM enumeration is still UNKNOWN because no
Windows observation captured the brief interval. Created
`docs/device/DEVICE_PHASE2C1E1A_DIAGNOSIS.md`; updated STATUS,ROADMAP,
USB_GADGET_PLAN and this log. No root,ConfigFS/UDC write,service restart,SELinux,
Fastboot,driver,network,IP,TCP,SweetDisplay,HID,FunctionFS,camera,commit or push
operation occurred. Hard stop after2C-1E1A.

Final `git diff --check` passes with only normal LF/CRLF notices. Publication
hygiene passes for217 publishable working-tree files,the index and125 reachable
history blobs. The connected-device serial,personal path,MAC-address and ADB-key
checks each return zero in the updated public documents. Private framework
artifacts and the sanitized private summary are ignored. No commit or push.

## 2026-09-23 — DEVICE PHASE 2C-1E1B controlled hands-off enumeration: PARTIAL

Prepared independent Android and Windows observers before mutation. Baseline was
phone `mtp,adb`/`mtp,adb`,ADB `device`,adbd running,no phone USB-network
interface,healthy Windows ADB/MTP and no active NCM. No ADB forward/reverse or
established SweetDisplay TCP session existed. Primary stock rollback was ready.

Issued exactly one `adb shell svc usb setFunctions ncm` request. Its process
exited255 as USB interrupted its ADB transport. Independent Android observation
proved `ncm,adb` config/state and new `usb0`. Windows observed a healthy
problem-code-0 `UsbNcm Host Device`,service `UsbNcm`,provider Microsoft, inbox
`usbncm.inf` version10.0.26100.9444,plus healthy concurrent ADB; MTP was absent.
Framework speed remained-1/UNKNOWN. No driver package was installed or updated.

The result did not remain stable. Xiaomi Settings automatically opened
`Settings$UsbDetailsActivity`; retained input records show DOWN then UP,followed
about130ms later by `UsbDeviceManager: Setting USB config to mtp,adb` and an
app-requested activity finish. The owner had confirmed hands-off operation; the
input event's physical,unintended/ghost or other origin remains UNKNOWN. Android
and Windows observed NCM only for a few seconds,so the30-second stability and
no-second-actor acceptance criteria fail.

Issued mandatory `adb shell svc usb setFunctions mtp`; it returned0 and converged.
Final phone state was `mtp,adb`/`mtp,adb`,adbd running,ADB `device`; Windows NCM
was inactive and original ADB/MTP were healthy. Persistent system/vendor USB
properties,driver-package inventory,enabled-adapter count and active network-
profile state matched baseline. Emergency reboot was unnecessary.

Classified E1B PARTIAL: stock NCM+ADB enumeration and rollback are proven,stable
hands-off hold is not. Preserved 2C-1 VERIFIED,2C-1E1 DENIED and2C-1E1A VERIFIED.
No manual IP,DHCP,route,metric,firewall,profile,ping,TCP,iperf,SweetDisplay,
throughput,HID,FunctionFS,camera,root,SELinux,Fastboot,driver/security,commit or
push operation occurred. Raw evidence is ignored/private. Hard stop after E1B.

Final `git diff --check` passes with only normal LF/CRLF notices. Publication
hygiene passes for218 publishable working-tree files,the index and125 reachable
history blobs. Updated public documents have zero connected-device serial,
personal-path,MAC-address and ADB-key matches. Final phone remains
`mtp,adb`/`mtp,adb` with ADB `device`; Windows has healthy ADB/MTP and zero active
NCM device. No commit or push.

## 2026-09-23 — DEVICE PHASE 2C-1E1C input isolation / stable NCM hold: PARTIAL

Prepared private timestamped Android logcat,passive kernel input,phone USB/UI and
Windows PnP/network observers before mutation. Baseline was exact
`mtp,adb`/`mtp,adb`,ADB `device`,adbd running,MIUI Launcher foreground,no phone
USB-network interface,no ADB forward/reverse or active SweetDisplay transport,
healthy Windows ADB/MTP and no active NCM. Input inventory had eight devices:
one touchscreen,no mouse,two key/button class,one virtual/uinput class,and
accessibility disabled.

Issued exactly one `adb shell svc usb setFunctions ncm` request. Independent
phone evidence at2026-09-23T13:19:44.101+03:00 proved `ncm,adb`,running ADB and
new `usb0`. Windows at2026-09-23T13:19:45.305+03:00 showed healthy problem-code-0
Microsoft inbox `UsbNcm` plus ADB,with MTP absent. Xiaomi Settings automatically
opened and focused `Settings$UsbDetailsActivity` about0.80s after the NCM request.

The120s hold failed. Passive `getevent` recorded a103ms DOWN/UP on the physical
touchscreen kernel device; no button,mouse or observed virtual-input event
aligned with it. Android's input-interaction record targeted the focused USB
details activity,and `UsbDeviceManager` requested `mtp,adb` about99ms later while
the touch sequence was completing. The owner had confirmed hands-off operation.
Classification is A at the input-path level; whether the hardware event was real
conductive contact or a controller/electrical ghost remains UNKNOWN. Device logs
place NCM request-to-MTP request at about4.31s; the independently established
hold ran about3.05s before detected reversion.

The phone had already returned to `mtp,adb`,so no redundant stock rollback was
issued. Final config/state were `mtp,adb`/`mtp,adb`,adbd and ADB were healthy,
`usb0` and active Windows NCM were absent,and Windows ADB/MTP were healthy/problem0.
Driver-package inventory hashes matched byte-for-byte and active network-profile
state matched baseline. Emergency reboot was not used.

A local PowerShell expression fault occurred after final evidence capture while
building the private summary. Its safety trap confirmed the existing healthy MTP
state without issuing another USB command. The fault lost the volatile exact NCM
request exit and speed result,which remain UNKNOWN; the ignored controller was
corrected and reparsed. This does not alter the independently captured transition,
input or final-state evidence.

Classified E1C PARTIAL: useful source localization and healthy final state were
obtained,but the120s stable hold failed. Preserved2C-1 VERIFIED,2C-1E1 DENIED,
2C-1E1A VERIFIED and2C-1E1B PARTIAL. No IP,DHCP,route,profile,firewall,ping,TCP,
iperf,SweetDisplay,throughput,HID,FunctionFS,camera,root,property,ConfigFS/UDC,
SELinux,package/service,input-injection,Fastboot,driver/security,commit or push
operation occurred. Raw logs,input traces,device identities and MAC values remain
ignored/private. Hard stop after E1C.

Final `git diff --check` passes with only normal line-ending notices. Publication
hygiene passes for219 publishable working-tree files,the index and125 reachable
history blobs. Updated public documents have zero connected-device serial,
personal-path,MAC-address,USB-instance and ADB-key matches. The private controller
and complete raw run are ignored. No commit or push.

## 2026-09-23 — DEVICE PHASE 2C-1E1D screen-off NCM stability: VERIFIED

The owner placed the phone stationary on a dry nonconductive surface and used
the physical power button to enter the locked/noninteractive screen-off state.
Two read-only gates,including one after observer startup,confirmed phone
`mtp,adb`/`mtp,adb`,ADB `device`,adbd running,no `usb0`,healthy Windows ADB/MTP,
no active NCM,Wakefulness Dozing,built-in display OFF,policy screen OFF and
interactive state SLEEP. No POWER/keyevent was injected.

Armed private USB/ADB,input,UI/logcat and Windows PnP observers,then issued
exactly one `adb shell svc usb setFunctions ncm`. The host process exited255 on
expected USB re-enumeration. By2026-09-23T15:24:05.463+03:00 Android reported
`ncm,adb`,running ADB and `usb0`; healthy Windows Microsoft inbox `UsbNcm` plus
ADB established the hold at2026-09-23T15:24:06.744+03:00.

The hold completed121.364s. All phone samples remained `ncm,adb` with `usb0`;
57 consecutive Windows samples had healthy problem-code-0 `UsbNcm` plus ADB,
zero NCM problem sample and no concurrent MTP. No autonomous `mtp,adb` request
occurred. Both passive kernel-input observer segments had zero event lines:
no touchscreen DOWN/UP and no hardware key. Xiaomi's USB-state receiver ran,but
the bounded log has no USB-details activity launch/focus/resume.

Android remained noninteractive Dozing and never became Awake. The USB
notification activated AOD and literal panel DOZE for about9s,then returned to
OFF. Three readable samples were DOZE;55 following samples and the pre-rollback
sample were OFF. Focus stayed on AOD with no resumed activity. This is retained
as an exact distinction: the locked screen-off/dozing regime held,but the panel
was not literally OFF during the brief low-power AOD animation.

After PASS,issued the required stock `adb shell svc usb setFunctions mtp` rollback.
Its host process exited255 on expected re-enumeration; independent checks
converged to `mtp,adb`/`mtp,adb`,adbd running,ADB `device`,no `usb0`,no active
Windows NCM,and healthy problem-code-0 ADB/MTP. Persistent system/vendor USB
properties,Windows driver inventory and active network-profile state matched
baseline. Emergency reboot was not used. Framework speed remained-1/UNKNOWN.

Classified E1D VERIFIED and preserved2C-1 VERIFIED,E1 DENIED,E1A VERIFIED,E1B
PARTIAL and E1C PARTIAL. No IP,DHCP,route,profile,firewall,ping,TCP,iperf,
SweetDisplay,throughput,HID,FunctionFS,camera,root,property,ConfigFS/UDC,SELinux,
service/package,input-injection,accessibility,Fastboot,driver/security,commit or
push operation occurred. Next recommendation is separately authorized E2
ephemeral NCM IP+TCP proof; E2 was not begun. Raw evidence remains ignored/private.

Final `git diff --check` passes with only normal line-ending notices. Publication
hygiene passes for220 publishable working-tree files,the index and125 reachable
history blobs. Updated public documents have zero connected-device serial,
personal-path,MAC-address,USB-instance and ADB-key matches. The private controller
and complete raw run are ignored. No commit or push.

## 2026-09-23 — DEVICE PHASE 2C-1E2 ephemeral NCM IP + TCP data plane: BLOCKED

Prepared the smallest non-video proof path before live mutation. Android NCM
diagnostic mode binds ordinary userspace TCP `0.0.0.0:48231`,uses ACK-only mode
and does not request keep-screen-on. The Windows probe reuses `TcpStream`,the
48-byte SWDP header,HELLO/CAPABILITIES,eight deterministic HEARTBEAT echoes and
one DRAIN/ACK in each of two fresh sessions. APK build/signature verification,
EWDK `/W4 /WX` probe build and58,235 existing protocol regressions passed. CRC
remains FRAME/H.264-specific and was deliberately not included.

Installed the updated signed SweetDisplay development APK through the owner's
existing ordinary `adb install` authorization. It remains installed;the bounded
activity/process was force-stopped during cleanup. No persistent USB,network or
security configuration accompanied this authorized project APK update.

The accepted live run began `mtp,adb`/`mtp,adb`,ADB device,Dozing/OFF,no `usb0`,
no NCM and empty ADB forward/reverse lists. The selected non-conflicting design
was `10.77.77.0/30`,Windows `.1`,Android `.2`,with no gateway,DNS,DHCP,NAT,
bridge,ICS,persistent route/profile or firewall change.

One stock `svc usb setFunctions ncm` request established `ncm,adb`,running ADB,
phone `usb0`,and healthy Microsoft `UsbNcm Host Device`,service `UsbNcm`,status
OK/problem0. The unaddressed Windows adapter was present with a valid interface
index. No driver operation occurred.

The minimum stock Android runtime request `ndc interface setcfg usb0 10.77.77.2
30 up` returned exit1 with empty output. Independent `ip -o -4 addr show dev
usb0` remained empty. Classified E2 BLOCKED at the authorized address-ownership
boundary and stopped without privilege escalation. No Windows peer address,
ping,TCP,application payload,reconnect or performance operation ran;live payload
was0 bytes. ADB carried control/observation only and no forward/reverse existed.

Stopped the bounded APK,confirmed/cleared absent Android test address,and issued
stock MTP rollback. Final state was `mtp,adb`/`mtp,adb`,ADB device,no `usb0`,no
active NCM,healthy problem-code-0 Windows ADB/MTP,no temporary test address/route,
and empty forward/reverse lists. Default routes,DNS,active profiles,driver
inventory and persistent USB properties matched baseline. Cleanup errors0.

Preserved two earlier private controller attempts: one pre-mutation empty-array
fault,and one IP-before observer false negative caused by requiring `NetEnabled`
on an unaddressed adapter. Neither configured IP or ran TCP;both recovered
cleanly. The accepted run removed that assumption and reached the genuine stock
Android addressing blocker.

No root,su,adb root,remount,persistent property,ConfigFS/UDC,SELinux,Fastboot,
partition/AVB,kernel/boot,driver,firewall,NAT/bridge/ICS,default-route,DNS,
persistent network,video,H.264,HID,FunctionFS,camera,commit or push occurred.
Recommended next bounded phase is read-only E2A stock `usb0` address-ownership
diagnosis;E3 was not begun. Raw evidence and device identity remain ignored/private.

Final `git diff --check` passed with normal line-ending notices. Publication
hygiene passed for223 publishable working-tree files,the index and125 reachable
history blobs. Targeted E2 public-content scans found zero personal path,MAC,
ADB-key,PnP-instance or connected-device-serial matches. No commit or push.

## 2026-09-24 — DEVICE PHASE 2C-1E2A stock `usb0` ownership: VERIFIED

Completed the authorized diagnosis with read-only ADB/runtime inspection and
matching Android 13 source. The phone had been rebooted by the owner and began
healthy `mtp,adb`/`mtp,adb`;the SweetDisplay process did not need to run. No NCM,
RNDIS or tethering state was activated.

Android 13 `ndc interface setcfg` parses an `InterfaceConfigurationParcel` and
calls `INetd.interfaceSetCfg`. `NetdNativeService` enforces `NETWORK_STACK` or
`MAINLINE_NETWORK_STACK` before applying the configuration. Exact runtime state
showed the UID-2000 `u:r:shell:s0` caller has zero effective capabilities and
neither required permission. NetworkStack's UID1073 holds the mainline
permission and runs with network capabilities;`netd` runs in its dedicated
domain with the capabilities that apply interface state. E2 exit1 alone did not
name the layer,and no exact Binder error text was retained,but source plus the
runtime grant set localize the effective failure to direct `netd` authorization,
before successful kernel configuration. The write command was not repeated.

Registered stock services include connectivity,ethernet,network management,
NetworkStack,Tethering,USB and `netd`. Runtime Tethering configuration reported
USB regexes `[usb\d,rndis\d]`,empty NCM regexes,USB function RNDIS,no requested
upstream and no active downstream. Historical service records observed NCM USB
broadcasts,but E2's composition-only `usb0` remained unaddressed. Thus the
dormant `ncm,adb` branch creates the gadget interface without starting IpServer.
The stock RNDIS flow would let NetworkStack/Tethering select a downstream prefix,
ask `netd` to configure it,add a connected route and manage DHCP/NAT as needed;
this exact ROM exposes no equivalent NCM-specific request.

The installed SweetDisplay APK requests only `INTERNET`. Connectivity/network
request/socket binding APIs cannot assign an address;TetheringManager,
EthernetManager and IpClient paths are system/module/internal and either require
signature privilege or do not apply to gadget `usb0`;VPN APIs configure TUN,
not `usb0`. Shell holds some unrelated privileged connectivity permissions,but
no stock CLI targets NCM addressing and those permissions do not satisfy direct
`netd`. No matching Xiaomi/vendor Binder service was found.

Classified E2A VERIFIED with architectural conclusion: stock owner exists but no
accessible path exists in the current ordinary-APK/bounded-shell boundary. Root
is not inherently required,the stock privileged components can do the work,and
a custom kernel is not required. E2 remains BLOCKED. The E2 TCP/protocol work,
APK/probe builds and58,235 regressions remain OFFLINE VERIFIED / LIVE NCM
UNVERIFIED. The one recommended next gate is offline 2C-1L3G ephemeral Level-3
authorization and recovery;E2B and E3 were not begun.

No USB composition,address,route,DHCP,ping,TCP,ADB tunnel,tethering,service,
package,security,system/vendor,boot or partition state changed. No commit or push.

Final `git diff --check` passed with only normal line-ending notices. Publication
hygiene passed for224 publishable working-tree files,the index and125 reachable
history blobs. Targeted changed-document scans found zero connected-device
serial,personal path,MAC address,USB instance or ADB-key marker matches. Raw
runtime output remains ignored/private.

## 2026-09-24 — SWEETDISPLAY FINAL-ARCH PHASE 1: VERIFIED

Completed an offline architecture/migration gate. No phone,ADB,Fastboot,USB,
network,build or boot-image operation occurred. Preserved every historical
classification,including 2C-1E2 BLOCKED in the ordinary APK/shell environment.

Established the authoritative product correction:MIUI plus the receiver APK is
the development/validation platform,not the final product. Inventoried Windows
and device work separately as direct implementation reuse,adaptation,
hardware/validation evidence,likely obsolete final plumbing or custom-boot
unknown. The Windows IddCx/shared-GPU/BGRA-NV12/AMD H.264/worker/queue/SWDP/
ACK/reconnect/TcpStream/touch-target stack remains directly reusable. The APK's
parser,decoder lifecycle and queue/reconnect rules are adaptable;its Activity,
MotionEvent and ADB packaging are validation-only. NCM/UsbNcm,Goodix,DRM,SDE,
VIDC,KGSL,DMA-BUF/ION/SMMU and stock-boot findings remain valuable exact-device
evidence.

Selected one architecture:exact stock kernel,embedded DTB and unchanged DTBO
plus a custom Android-derived minimal userspace. Retain only required native
init/early-mount,Binder,SurfaceFlinger,HWC/gralloc/EGL,media codec and minimum
power/thermal/health components with exact private vendor dependencies. Do not
start the MIUI launcher,SystemUI,MIUI apps or the full Java product shell.
SweetDisplay becomes a supervised native core with its own EGL/GLES UI.

Selected initial native transport NCM/TCP and staged touch migration:direct
Goodix evdev into existing SWDP Touch Profile1 and verified Windows injection;
HID later. The E2 privilege block is deferred rather than solved in MIUI because
custom userspace will own ConfigFS and address `usb0`. NcmProtocolProbe remains
OFFLINE VERIFIED / LIVE NCM UNVERIFIED until FINAL-USB.

Stock-kernel reuse is preferred;the known non-matching public source/toolchain
gate makes a custom kernel unnecessary and higher risk. Hardware decode should
first retain Android native media/vendor OMX services because direct downstream
VIDC V4L2 behavior is unproven. Vendor graphics,codec firmware/services and
power components are expected privately for the owner's exact device;no
redistribution conclusion was made.

Temporary boot is technically plausible but live-unverified. The future design
preserves boot-header-v2,page/address/command-line/DTB requirements,mounts only
needed stock logical partitions read-only,uses tmpfs and avoids userdata/FBE.
No partition write is allowed for first development boot;unsupported RAM boot
means stop. Defined the smallest live milestone as a RAM-only custom environment
showing a SweetDisplay diagnostic UI,touch coordinates and DWC3 presence,then
rebooting to untouched MIUI without video,NCM throughput,HID,Spotify or camera.

Created `docs/final-architecture/SWEETDISPLAY_FINAL_ARCH_PHASE1.md`;updated
STATUS,ROADMAP,USB_GADGET_PLAN and this log. The one next phase is host-only
FINAL-BOOT PREP1 offline artifact/dependency/recovery validation. It was not
started. No commit or push.

Final `git diff --check` passed with only normal line-ending notices. Publication
hygiene passed for225 publishable working-tree files,the index and125 reachable
history blobs. The new/updated public architecture text contains no device
serial,personal path,MAC address,unique USB/PnP instance or ADB-key material.
Proprietary images,blobs and raw evidence remain ignored/private.

## 2026-09-24 — SWEETDISPLAY FINAL-BOOT PREP 1: VERIFIED

Completed the authorized host-only boot-artifact and recovery gate. No ADB or
Fastboot device command,temporary boot,flash,partition write,phone reboot,USB or
network mutation occurred.

Used only the exact preserved Turkey `V14.0.2.0.TKFTRXM` stock artifacts. Pinned
AOSP Android13-r36 `unpack_bootimg`/`mkbootimg` unpacked and repacked the stock
boot body. The 22,257,664-byte `STOCK-ROUNDTRIP` was byte-identical to the
original image prefix through its AVB original-image boundary. Its SHA-256 is
`c810a47ed05a06e0c06792e65184a453c402f6ff52c71816d73e92bb7f050e12`.
The only original-file tail omitted from the round-trip is the known unsigned
AVB descriptor/footer placement and partition padding.

Implemented a native first-boot diagnostic and built it with Android NDK r28c,
Clang19.0.1,`-Wall -Wextra -Werror`. The stripped static ELF64/AArch64 binary is
433,096 bytes,SHA-256
`dc0832544cf5108e50cae7df69cb090b94a14ebe2807224c4e280c3bbc188ddc`,
and has no dynamic dependency. It requests DRM master,discovers a connected
connector/CRTC,draws one dumb-buffer SweetDisplay UI,discovers Goodix by evdev
name plus MT capabilities,scales coordinates,observes DWC3 read-only,shows the
SELinux enforcing flag and requests normal reboot after180 seconds.

Selected direct DRM/KMS for this disposable first diagnostic because exact
runtime evidence has DRM/SDE DSI and no `/dev/fb*`;SurfaceFlinger/HWC remains the
selected final accelerated UI but has a larger unresolved first-boot closure.
Built the private ramdisk deterministically from exact stock recovery init,
ueventd,its recursive33-library closure and compiled enforcing policy. Replaced
the recovery UI with the static diagnostic and used a reviewed rc graph with
only tmpfs `/tmp`. Audit passes for66 allowlisted entries,exactly three regular
executables,no device node,no fstab,no `/data`/`/metadata`/`/cache`,no block or
partition utility,no ADB/fastbootd/vold/shell/network/USB gadget,and no private
key,ADB key or personal path. The bounded recovery-domain reuse is a first-test
exception,not final product policy.

Created and twice reproduced the private `SWEETDISPLAY-TEMP-BOOT-v1.img` at
25,673,728 bytes,SHA-256
`af7ff18d9fa87929b6eff1949180f9ea5788fa0d079897222d218c356852af9e`.
Its gzip ramdisk SHA-256 is
`d8e3af020ecba694cc9bb1ab99a463de9e6ab5f82a09f7954b5f82ed3c985ef5`.
Re-unpack verifies boot-header-v2,4KiB pages,all original load addresses,complete
command line,exact stock kernel hash
`764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc`
and embedded DTB hash
`8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10`.
The candidate is unsigned,has no AVB footer and is never to be flashed.

No dynamic partition is needed for the diagnostic;system/vendor/product/odm/
system_ext remain unmounted. No userdata,FBE,metadata or persistent setting is
involved. Exact-bootloader downloaded-boot acceptance,AVB treatment,early
policy load,Qualcomm downstream KMS behavior,Goodix raw orientation and direct
reboot permission remain live unknowns.

Prepared a no-flash owner recovery matrix and future command sequence,marking
every command FUTURE / NOT EXECUTED. The decision is **GO FOR SEPARATELY
AUTHORIZED FINAL-BOOT 1**. GO does not assert `fastboot boot` works. Image
rejection,write/flash prompt,identity/hash ambiguity,non-enforcing state,missing
or corrupt UI,timeout or uncertain physical recovery is a hard stop.

Created `docs/final-architecture/SWEETDISPLAY_FINAL_BOOT_PREP1.md`,the public
diagnostic/build sources and scripts;updated STATUS,ROADMAP,ARCHITECTURE and this
log. Generated ELF,round-trip,ramdisk,candidate,compiled policy and stock
extractions remain ignored/private. No commit or push.

Final `git diff --check` passed with only normal line-ending notices.
Publication hygiene passed for233 publishable working-tree files,the index and
125 reachable history blobs. The targeted public scan contains no personal
path,unique device identity,MAC address,ADB key or embedded private artifact.

## 2026-09-24 — SWEETDISPLAY FINAL-BOOT 1: FAILED / stock recovered

The owner explicitly passed the physical safety/recovery gate and entered the
visible Fastboot screen with hardware buttons. Host preflight rechecked the
private candidate at25,673,728 bytes and SHA-256
`af7ff18d9fa87929b6eff1949180f9ea5788fa0d079897222d218c356852af9e`.
Header-v2,4KiB page,OS/patch,exact stock kernel/DTB hashes,custom ramdisk hash
and absent AVB footer matched PREP1. Android SDK Fastboot36.0.2-14143358 was used.

The sanitized identity gate observed exactly one device,product `sweet`,
`unlocked:yes` and max download805,306,368 bytes. The one reviewed semantic
operation was TEMPORARY DOWNLOAD + BOOT ONLY. Exactly one `fastboot boot` command
ran;Fastboot reported `Sending boot.img OKAY`,then `Booting OKAY`,exit0. No
second attempt,alternate image,rebuild,flash,erase,format,slot,lock,OEM,vbmeta or
verification operation occurred.

The owner observed normal Android instead of the SweetDisplay diagnostic. No
SweetDisplay pixels,SELinux state,Goodix coordinate or DWC3 status was available,
so display failed the custom-UI gate and the remaining diagnostic fields are
UNKNOWN/NOT TESTED. Physical recovery was not used;the phone returned
automatically to stock Android13 `V14.0.2.0.TKFTRXM`. Bounded read-only ADB
verified one authorized `device`,boot completion1 and the expected build.

Post-return `sys.usb.config`,state and persistent config were `adb`,not the
historical `mtp,adb` development baseline. Pre-Fastboot values were not captured,
no USB property was written,and causation remains UNKNOWN. A read-only framework
function query failed in Xiaomi resource initialization and changed nothing.
Stock OS/build/ADB recovery passed;exact MTP-baseline equivalence remains UNKNOWN.

Classified FINAL-BOOT1 FAILED because the boot command was accepted but no useful
custom-environment milestone appeared and stock recovery succeeded. **NO
PARTITION-WRITE COMMAND WAS ISSUED.** No root,SELinux,ConfigFS/UDC,network,media,
HID,camera,Windows driver/security,commit or push operation occurred. FINAL-INPUT1
and FINAL-USB1 were not begun. The sole recommendation is a separately authorized
host-only FINAL-BOOT1A early-boot failure diagnosis and non-persistent evidence
design;it was not started.

Final `git diff --check` passed with only normal line-ending notices. Publication
hygiene passed for234 publishable working-tree files,the index and125 reachable
history blobs. Public content contains no unique Fastboot identity,personal
path,MAC address,ADB key,raw private evidence or firmware/image payload. The
candidate and related raw evidence remain ignored. No commit or push.

## 2026-09-24 — SWEETDISPLAY FINAL-BOOT 1A: VERIFIED offline

Performed only host-side/offline work. No ADB or Fastboot command was sent to the
phone and FINAL-BOOT 1B was not executed. FINAL-BOOT 1 remains FAILED with stock
recovered.

Revalidated the exact private v1 image and ramdisk hashes and reproduced the
original static diagnostic ELF from the non-v2 source path. Static review found
that v1 retries DRM for30 seconds,then waits10 seconds and requests a normal
reboot without a durable checkpoint. This previously unreported approximately
40-second path is consistent with the initial bounded stock observation,but is
not proven to have executed. The visible180-second timer is not a credible
primary explanation for stock Android being observed during the initial60-second
gate. Exact wall-clock command/report points and all v1 postmortem data remain
NOT CAPTURED.

Compared the868-path stock recovery ramdisk with the66-path minimal candidate.
The candidate removed802 stock-only paths,added no new pathname and changed only
the diagnostic,init rc and ueventd rc. The boot-critical manifest preserves
stock modes,owners,init/linker/library closure,compiled enforcing policy and
contexts. No persistent mount path,block device or unexpected executable exists.

Implemented a conditional v2 static diagnostic plus direct-`late-init` rc. V2
retains the exact stock recovery ueventd rules,requires SELinux enforcing,writes
bounded `SDV2` checkpoints to pmsg/kmsg/stderr,signals diagnostic/SELinux/DRM
states with distinct backlight pulse counts,records exact DRM stage and errno,
and tags each requested reboot. It introduces no USB gadget,ADB,NCM,HID,network,
partition mount or persistent filesystem.

Built and twice reproduced the private v2 candidate. Static diagnostic:
435,848 bytes,SHA-256
`3f6d08f45de5d3878738cfbec012f3b19ae1025672bd1ecbdcf3bdf662bec174`.
Ramdisk:4,230,458 bytes,SHA-256
`1cdd0b95f9f2bfbb1f0791783a4c1d824c4b9a03442a40d394b1280a4be0959f`.
Image:25,673,728 bytes,SHA-256
`764c8886547dd2f3010f643ecb5789c57af3694aa4dadff91093476facc9cbd1`.
Re-unpack preserves the exact stock kernel and DTB,header v2,4KiB page,original
command line and load addresses,and confirms no AVB footer.

Exact kernel config enables pstore console,pmsg and RAM backends and extends
`ramoops_memreserve=4M`. The Sweet-correlated base DTB did not expose the ramoops
node found in another bundled tree,so runtime backend creation,persistence and
ordinary-shell readability remain UNKNOWN. Pmsg is credible but not treated as
guaranteed; kmsg,stderr,pulse codes and visible pixels provide bounded redundancy.

Classified FINAL-BOOT 1A VERIFIED for its offline scope and selected **GO FOR
SEPARATELY AUTHORIZED FINAL-BOOT 1B**. This is not live authorization. 1B may use
only the exact v2 hash for one RAM-only attempt with precise timestamps and
bounded read-only post-return pstore/boot-reason capture. FINAL-INPUT and
FINAL-USB remain unstarted. No commit or push.

Final `git diff --check` passed with only normal line-ending notices. Publication
hygiene passed for238 publishable working-tree files,the index and125 reachable
history blobs. Generated ELF,manifest,ramdisk,unpacked validation and image files
remain ignored/private. Public 1A changes contain no unique device identity,USB
instance identity,MAC,ADB key,personal path,private log,firmware/image payload or
raw evidence.

## 2026-09-24 — SWEETDISPLAY FINAL-BOOT 1B: VERIFIED observability

The owner explicitly passed the physical safety gate and entered Fastboot with
hardware controls. Read-only preflight recorded healthy Android13,
`V14.0.2.0.TKFTRXM`,boot completion and `mtp,adb`. Pstore existed but enforcing
shell access was denied;last-kmsg was inaccessible. No baseline state changed.

Immediately before boot,the private v2 image revalidated at25,673,728 bytes and
SHA-256
`764c8886547dd2f3010f643ecb5789c57af3694aa4dadff91093476facc9cbd1`.
The read-only gate observed exactly one `sweet` device,unlocked state and an
805,306,368-byte download bound. Exactly one RAM-only boot ran. Fastboot reported
Sending OKAY in0.605s,Booting OKAY in0.146s,total0.763s and exit0. About4.75s
after command return,Fastboot transport was present again.

The owner was not watching the initial screen,so pulse/UI observations are NOT
OBSERVED and no count is inferred. The owner later confirmed Fastboot. A repeat
request was declined because the authorized1-of-1 attempt and evidence boundary
were consumed. The owner used the prepared physical forced reboot;healthy stock
Android,boot completion,ADB and exact pre-boot `mtp,adb` returned.

Direct pstore remained SELinux-inaccessible,but stock BootReceiver copied
`console-ramoops-0` to read-only `SYSTEM_LAST_KMSG`. Its tail proves the exact
kernel and v2 ramdisk/PID1 path ran. At about1.72s Android first-stage init failed
to mount tmpfs at missing `/mnt`,could not create `/mnt/vendor` or `/mnt/product`,
failed to mount tmpfs at missing `/debug_ramdisk`,reported first-stage errors and
`InitFatalReboot: signal6`,then explicitly restarted with command `bootloader`.
No `SDV2`,panic or watchdog marker was recovered.

The private66-entry manifest confirms `/mnt` and `/debug_ramdisk` are absent;
the exact stock868-entry recovery ramdisk contains both as root-owned mode0755
empty directories. The builder incorrectly treated `/mnt` itself as a forbidden
persistent path,although here it is only a RAM-backed mountpoint. Highest proven
checkpoint is C3 entry;C3 completion failed and C4 SELinux is the first unproven
checkpoint. Diagnostic,DRM,Goodix,DWC3 and tagged reboot paths were not reached.

Classified FINAL-BOOT1B VERIFIED for its observability objective with successful
stock recovery. No partition-write command,second boot,alternate image,live
rebuild,root,SELinux weakening,USB/network/media/input expansion,commit or push
occurred. The sole recommendation is host-only FINAL-BOOT1C:restore/audit the
first-stage ramdisk directory closure and validate a deterministic v3 without a
live boot.

Created `docs/final-architecture/SWEETDISPLAY_FINAL_BOOT1B_RESULTS.md` and
updated STATUS,ROADMAP,ARCHITECTURE and this log. No source or image was changed.
Final `git diff --check` passed with only normal line-ending notices.Publication
hygiene passed for239 publishable working-tree files,the index and125 reachable
history blobs.Targeted review found no unique device identity,USB instance,
boot serial/PARTUUID,MAC,ADB key,personal path,private key,raw pstore or image
payload in public changes.Raw evidence and generated artifacts remain private/
ignored.No commit or push.

## 2026-09-25 — FINAL-BOOT BRINGUP V10 physical display/touch evidence

Resumed the existing V10 attempt after an interrupted observation; did not boot
again. Exactly one owner-authorized RAM-only V10 command had passed the identity
and SHA-256 gate at 17:07:09 +03:00 (Sending 0.613 s, Booting 0.146 s, total
0.767 s). Image hash:
`aefc8d233309bb0ba1f07497b634026ca1d164ed2f44057eb8ca0123e273892d`.

Owner confirmed the SweetDisplay screen and physical drag coordinate updates
(including 489 / 1810). Owner photo independently shows DISPLAY OK, TOUCH
WAITING at that captured instant, USB CONTROLLER DETECTED, SELINUX ENFORCING,
V10 build label and AUTO REBOOT 170. WAITING does not contradict the separately
reported subsequent touch. A still image is not temporal-stability evidence.

Latest V10 SYSTEM_LAST_KMSG: enforcing at 3.763366 s, V10_COLD_E0 at 4.665404 s,
V10_PIXELS_E0 at 4.688743 s, normal restart at 184.428210 s. Goodix lifecycle
activity is present; alternate FTS probe failed. Stock dumpsys input names
goodix_ts; V10 did not emit its exact selected evdev name. Kernel evidence and
physical observations are distinguished in the concise bringup result.

At resumed check, the same device was boot-complete in V14.0.2.0.TKFTRXM with
authorized ADB, mtp,adb config/state, Enforcing, good battery health, 100% charge
and 28.3 C. The first resumed observation time is not the actual stock-return
time. Raw dropbox logs and original photo remain ignored/private. No second boot,
flash, partition write, security change, USB/network/media operation, commit or
push. Full result stays PARTIAL pending owner clarification of temporal visual
stability and hands-off return; no new experiment is scheduled.

### V10 owner clarification and offline V11 correction

Owner now confirms automatic MIUI return at timer expiry and describes visible
wave/fade-like redraw with briefly missing text during held/dragged touch.
Stable-display acceptance therefore FAILS; the milestone remains PARTIAL, not
pending clarification and not VERIFIED. Source confirms Render clears the
actively scanned-out framebuffer before repainting on each input update.
This strongly explains the observed symptom; exact panel timing is unmeasured.

Built V11 offline with two dumb buffers, normal PAGE_FLIP_EVENT presentation,
cookie-matched completion before old-buffer reuse, nonblocking event reads and
a two-second monotonic event-wait deadline. No async/single-buffer fallback.
Failure exits through the existing bounded init-owned reboot. No change to
stock kernel/DTB, enforcing, coldboot, volatile backlight or 180/240-second timers.
Actual stock recovery drmModePageFlip disassembly confirms 24-byte request,
cookie offset16 and ioctl0xc01864b0; actual V11 driver support is NOT YET TESTED.

V10 diagnostic hash regression PASS; 18 compile-time event parser cases PASS;
two V11 builds byte-identical; static AArch64 ELF and direct image/73-entry
manifest audit PASS. V11 image25,677,824 bytes, SHA-256
`25c31a22fd5645c4604bc01b9f179a7ae8dc16e7f8d3f3f93dfe7112321bfbe4`.
No phone command ran during this correction. V11 live validation requires a
fresh owner safety/Fastboot gate; neither V10 replay nor FINAL-USB is started.

### V11 live stable-presentation closeout

Owner reconfirmed the V11 safety gate and physical Fastboot. Exact preflight
verified one device, product sweet, unlocked yes, bootloader Fastboot and image
SHA-256 `25c31a22fd5645c4604bc01b9f179a7ae8dc16e7f8d3f3f93dfe7112321bfbe4`.
Exactly one RAM-only boot ran at 19:27:23 +03:00: Sending OKAY 0.632s, Booting
OKAY 0.146s, total 0.789s. No flash/partition-write command or retry occurred.

Owner confirms V11 is stable/readable while holding and dragging: coordinates
change and only their numbers update; V10's wave/fade and transient text loss are
completely gone. SELINUX ENFORCING and USB CONTROLLER DETECTED are visible.
Kernel record independently contains enforcing at3.685606s, V11_COLD_E0 at
4.527686s, V11_PIXELS_E0 at4.562788s and normal reboot at184.522019s. Normal
PAGE_FLIP_EVENT plus completion-gated old-buffer reuse is therefore live-proven
for this diagnostic; no atomic-path claim is needed or made.

Host observed ADB return at19:30:55 and boot-complete at19:31:12. Same-device
stock state is sweet / V14.0.2.0.TKFTRXM, mtp,adb config/state, Enforcing, good
battery health,98%,31.9C. Raw normalized latest V11 kernel record is66,148 chars,
SHA-256 `7d509bffc289e122af40312fa9168fa8cfa491f214a5877db37a5a0bed5b5384`;
it remains ignored/private. Full partition byte-equivalence was not re-read.

FINAL-BOOT BRINGUP = VERIFIED. Historical FINAL-BOOT1/1A/1B and V10 PARTIAL
evidence remain unchanged. No persistent SweetDisplay environment, root,
SELinux weakening, USB/network/media/HID/camera expansion, commit or push.
HARD STOP before separately authorized FINAL-USB.

## 2026-09-25 — SWEETDISPLAY FINAL-USB candidate 1 offline gate

Cleaned the stale final-bringup sentence that simultaneously described V11 as
untested and verified; historical V10 failure and V11 progression remain.
Reused the exact preserved Xiaomi NCM branch and earlier Windows UsbNcm/SWDP
work instead of repeating architecture discovery.

Implemented an NCM-only ConfigFS graph using the stock `05C6:A4A1` identity,
one `ncm.0` function and `a600000.dwc3`, with no serial string, ADB, MTP,
FunctionFS, HID or media functions. Added a static native endpoint that assigns
only 10.77.77.2/30 to `usb0`, listens on TCP/48231 and answers two sequential
SWDP HELLO/CAPABILITIES/eight-HEARTBEAT/DRAIN sessions. The V11 page-flip UI
now reads a volatile `/tmp` status record and displays USB/IP/TCP/SWDP progress.
SELinux remains enforcing with the exact stock compiled recovery policy;
`CAP_NET_ADMIN` is the sole explicit service capability.

The Windows controller checks exactly one `sweet`/unlocked Fastboot target and
the exact image hash, issues at most one RAM-only `fastboot boot`, accepts only
a healthy inbox UsbNcm adapter, configures 10.77.77.1/30 in ActiveStore, runs
the existing protocol probe, removes the address, and requires healthy stock
`mtp,adb`/Enforcing return with unchanged driver/default-route/DNS state. It
does not install a driver, change firewall/profile state, ping, use an ADB
tunnel or retry a failed image.

Both device ELFs are static; 18 page-flip cases, PowerShell parsing, 75-entry
ramdisk audit, boot-v2 re-unpack, exact stock kernel/DTB, no-AVB-footer,
forbidden-path and sensitive-marker checks pass. The pinned EWDK rebuild of the
existing Windows NCM protocol probe passes. Two clean candidate builds are
byte-identical. Final image is 25,862,144 bytes, SHA-256
`d88e37141185d4c1d9942b59087b47d3f4f36987b3207f37289952a5c594caac`.

No phone command, Fastboot operation, USB/IP mutation, flash, partition write,
SELinux weakening, commit or push occurred. FINAL-USB remains NOT YET TESTED
until a fresh owner physical safety/Fastboot gate authorizes one live attempt.
`git diff --check` passed with normal line-ending notices. Publication hygiene
passed for 276 publishable working-tree files, the index and 125 reachable
history blobs. Public changes contain no unique device/USB identity, MAC,
personal path or raw private evidence; images, binaries and the iteration ledger
remain ignored/private.

### Candidate 1 live failure and Candidate 2 correction

A fresh owner gate authorized one exact Candidate 1 RAM-only attempt. Read-only
preflight passed one `sweet`, unlocked device and exact image SHA-256. Fastboot
reported Sending OKAY 0.627 s, Booting OKAY 0.147 s and total 0.786 s. Owner
observed the SweetDisplay UI with `USB: ERROR`.

The host controller independently failed immediately after boot on malformed
PowerShell property-filter shorthand, before retained NCM observation or any
`New-NetIPAddress` call. That observer failure neither caused nor explains the
on-device error. No project IP/route was present afterward, no NCM device
remained and the installed-driver inventory matched baseline. Ordinary host
adapters had disconnected by the post-check, so global default-route/DNS/profile
snapshots differed; their exact equality is not claimed or attributed.

Read-only post-return `SYSTEM_LAST_KMSG` contains the decisive device evidence.
At 4.584 s the kernel created the NCM function and generated volatile self/host
Ethernet addresses. The endpoint then received only a new enforcing
`recovery -> sysfs_net:dir search` denial for its `/sys/class/net/usb0` access
check. Its code never reached address ioctls. Normal restart occurred at
184.582 s. Stock ADB, boot completion, V14.0.2.0.TKFTRXM, `mtp,adb` and Enforcing
all passed. No repeat, flash, write, persistent state or policy weakening.

Candidate 2 removes only that unnecessary sysfs read and retries the actual
`usb0` ioctls for the same bounded window, with stage-specific thread names for
future AVC localization. The Windows controller now uses explicit script-block
filters; all corrected filters execute in the safe no-NCM/no-project-route
state. Candidate 1 source-regression hashes remain exact. Candidate 2 builds
twice identically: 25,862,144 bytes, SHA-256
`57b6e8e3f3577297563874b7ef9540fcedb67633bd39b230a9044b59bf0e5308`.
Stock kernel/DTB, ConfigFS graph, enforcing policy, capability, IP/protocol,
timers and prohibitions are unchanged. Candidate 2 needs a fresh owner gate;
it has not run live.

### Candidate 2 live result and Candidate 3 offline correction

A fresh owner gate authorized exactly one Candidate 2 RAM-only boot. Fastboot
reported Sending OKAY 0.623 s, Booting OKAY 0.147 s and total 0.779 s. The
custom environment enumerated a healthy Microsoft inbox `UsbNcm` adapter with
problem code 0. The owner observed `USB: ACTIVE` followed by `USB: ERROR`.

The host controller temporarily added its ActiveStore /30 address, then stopped
before TCP because its 500 ms snapshot required exactly one immediately visible
project route. Its `finally` removed the address. Retained kernel evidence is
decisive on the device boundary: `ncm.0` was created at 4.724 s, then enforcing
SELinux denied `recovery -> self:udp_socket create` at `FU2_SOCKET`, before any
address ioctl. Normal reboot occurred at 184.672 s. The controller's first stock
sample caught transient `adb`; a later bounded read-only check proved final ADB
device, boot completion 1, `mtp,adb`, Enforcing and V14.0.2.0.TKFTRXM. Windows
had no NCM device, project IP/route, driver change, default-route/DNS/profile
change or problem device. Candidate 2 is a safe PARTIAL result, not VERIFIED.

Candidate 3 avoids adding UDP access: interface ioctls use the stock-allowed TCP
socket class. The stock-derived policy remains enforcing and adds only recovery
TCP read/write/getattr/bind/listen/accept/setopt/shutdown plus node/port bind.
Compiled version-30 disassembly shows the exact expected allow delta, zero other
runtime-rule delta and no recovery UDP permission. The host now waits boundedly
for address/route and complete stock USB convergence while retaining strict
default-route/DNS/cleanup rejection.

Candidate 3 reproduces at 25,858,048 bytes and SHA-256
`62dade6576d8c2338fb080b94d3a3ee6c43f1973e56950522bee6719b0310a41`.
Ramdisk SHA-256 is
`54ae6081b399f65f85951b900b057e9fd0d1c46bf3128331d911ee1b10dca4c9`;
policy SHA-256 is
`320e3cfc471a214e33964529206812371df1cdeaeeb975dc9af2319525303cb8`.
Static ELF, 18 event cases, boot-v2 re-unpack, exact stock kernel/DTB, 75-entry
closure, no-AVB-footer and sensitive/forbidden-path checks pass. Candidate 3 has
not run live; a fresh owner physical/Fastboot gate is required. No flash,
partition write, persistent device/network change, commit or push occurred.

Final `git diff --check` passed with normal line-ending notices. Publication
hygiene passed for 278 publishable working-tree files, the index and 125
reachable history blobs. Public content contains no unique device/USB identity,
MAC, personal path or raw private evidence; generated images, policies,
binaries and raw logs remain ignored/private.

### Candidate 3 live failure and Candidate 4 exact-xperm correction

A fresh owner gate authorized exactly one Candidate 3 RAM-only boot. Fastboot
reported Sending OKAY 0.606 s, Booting OKAY 0.146 s and total 0.765 s. Windows
enumerated healthy Microsoft inbox `UsbNcm` with problem code 0. The owner photo
shows DISPLAY OK, USB CONTROLLER DETECTED, SELINUX ENFORCING, FINAL-USB-3,
USB NCM READY, IP ERROR, TCP WAITING and all SWDP counters zero.

The latest read-only `SYSTEM_LAST_KMSG` segment proves the root cause. At
4.556 s, `FU3_ADDR` reached TCP-socket `SIOCSIFADDR`; enforcing SELinux denied
ioctl command `0x8916`. Sixty-three bounded retries continued through 31.696 s.
The ordinary `tcp_socket ioctl` allow rule was present, but Android's separate
ioctl extended-permission filter allowed recovery only stock commands
`0x8913`/`0x8914`. Netmask, link-up, listener and SWDP were not reached. Normal
reboot occurred at 184.423 s. The 66,163-character latest kernel segment has
SHA-256 `9ba5f178b046726c45c585ac3e981589e83858209699b93f4ae0cdda0f08cb45`.

Because device `usb0` never came up, the healthy Windows NCM adapter did not
produce the expected connected route. The controller removed its temporary
ActiveStore address and stopped before TCP. Final outcome is FAILED with clean
rollback: stock ADB, boot completion, `mtp,adb`, Enforcing and expected build
returned; no NCM/project IP/route or problem device remained; default route,
DNS, profiles and driver inventory matched. No retry or persistent change.

Candidate 4 adds only xperm commands `0x8916` (`SIOCSIFADDR`) and `0x891c`
(`SIOCSIFNETMASK`), verified from the pinned Android NDK headers. It retains the
Candidate 3 TCP-server allow delta and stays enforcing, with no UDP permission
or permissive domain. Compiled policy comparison proves the exact allow/xperm
delta and zero other runtime-rule delta.

Two Candidate 4 builds are byte-identical. Image: 25,858,048 bytes, SHA-256
`cf0158c10fc083c2fc4e71e75a0fa828fb3b17e0b4d5b7277ce9dcd389fb5938`.
Ramdisk SHA-256:
`b6b27cee1c0e724af4ac87513d927ea4c1f6787699e08345ea1d192c0d153757`;
policy SHA-256:
`83a09282dcfc1aac1424d7413fe02e07429031d49e72597d0d70eaefefd60079`.
Static ELF, 18 event cases, boot-v2 re-unpack, exact stock kernel/DTB, 75-entry
closure, no-AVB-footer and sensitive/forbidden-path checks pass. Candidate 3
policy/ELF regression hashes remain exact. Candidate 4 has not run live; a
fresh owner physical/Fastboot gate is required.

Final `git diff --check` passed with normal line-ending notices. Publication
hygiene passed for 279 publishable working-tree files, the index and 125
reachable history blobs. The owner photo and raw kernel evidence were not
published; generated images, policies, binaries and all private evidence remain
ignored. No commit or push.
