# Windows deployment gate — 2026-09-13

Status: packages built and validated; installation, driver startup, Windows
Settings name, desktop extension and active 2400x1080@60 mode NOT YET TESTED.
No software device has been created by the new enumerator.

## Prepared artifacts

- Driver package: out/windows/x64/Debug/SweetDisplayDriver/bin/SweetDisplayDriver/
  (SweetDisplayDriver.inf, SweetDisplayDriver.dll, sweetdisplaydriver.cat).
- Companion monitor package: out/windows/monitor/
  (SweetDisplayMonitor.inf, sweetdisplaymonitor.cat).
- Development enumerator: out/windows/x64/Debug/SweetDisplayDevice/bin/SweetDisplayDevice.exe.

Both packages are unsigned. Inf2Cat success validates catalog generation and
signability; it does not sign a catalog. The monitor INF passed InfVerif /w for
this kit. The driver passed its normal WDK build checks and code analysis.

## Local policy observations

Machine-specific policy readings are retained only in private evidence. Recheck
the agreed test computer before deployment; do not assume its security state.
No certificate store or Windows security policy was modified.

## Signing path under the owner's current constraints

The owner forbids Windows security-setting changes. Do not enable test signing,
disable Secure Boot/HVCI, change debugging settings, or import a test certificate.
The ISO approval covered only its Zone.Identifier and has been completed.

Microsoft supports dashboard/attestation signing for Windows desktop user-mode
driver packages. Attestation requires a Hardware Dev Center submission and an EV
certificate. This is a documented path toward trusted packages while retaining
the current host protections; it is not an existing certificate/account or a
completed submission. No upload or purchase is authorized or performed.

The IDD is UMDF: do not apply kernel-driver test-signing advice as proof that
this specific driver requires a BCD change. PnP package trust and runtime code
integrity are separate gates. No unsigned installation has been attempted.

An isolated development machine with explicitly approved test certificate/policy
configuration is another test option. It is not authorization to change this PC.

## Runtime verification after trusted packages are available

1. Verify returned signatures, package contents and exact hardware IDs. Install
   only the two SweetDisplay packages on the agreed test PC; retain assigned
   oemNN.inf names for removal. Never target generic monitors by class alone.
2. Start SweetDisplayDevice; verify device problem code, UMDF startup and one
   monitor arrival. Its process must remain open during the test.
3. Verify the monitor package matches MONITOR\SWT0001. Confirm the full
   SweetDisplay AMOLED name in Windows Settings; an adapter name alone is
   insufficient. The EDID has only the valid short name SweetDisplay.
4. Select Extend while preserving the existing primary monitor. QueryDisplayConfig
   must confirm an active 2400x1080 target at 60/1 Hz on the new indirect display.
   Record the real result, including whether Windows selected a different mode.
5. Exit the enumerator with X; verify disappearance and restoration of the original
   desktop topology. Test reconnection before claiming stable enumeration.

No phone display/transport success follows from this Windows milestone. The
current driver acquires/releases surfaces without encoding or sending them.

References: [Microsoft signing options](https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/driver-signing-offerings),
[user-mode signing distinctions](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/windows-driver-signing-tutorial),
[monitor INF naming](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/overriding-monitor-edids),
[SwDeviceClose callback/lifetime contract](https://learn.microsoft.com/en-us/windows/win32/api/swdevice/nf-swdevice-swdeviceclose).

