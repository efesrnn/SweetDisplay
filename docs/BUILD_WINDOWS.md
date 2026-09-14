# Verified result — 2026-09-13

Unchanged Microsoft sample: PASS, 0 warnings/errors, EWDK 26100.6584, SignMode=Off. SweetDisplay derivative: PASS, plus EDID/mode tests and monitor INF/catalog validation. Run scripts/windows/Build-SweetDisplay.cmd with the mounted EWDK root. Outputs: out/windows; details in windows/driver/README.md. No driver installed. See DEPLOYMENT_WINDOWS.md for the signing gate and runtime acceptance.

# Windows baseline build

See HOST_ENVIRONMENT.md for installed tools and official installation options. No Windows security settings or certificates were changed.

Current installation checklist: [WDK_SETUP.md](WDK_SETUP.md). SDK is 26100.7705; WDK 26100.6584 requires the documented VS 2022 pairing. Recommended EWDK supplies that pairing independently of installed VS 2026. Setup confirmed and EWDK baseline passed on 2026-09-13; no further installation needed for builds.

## Historical baseline attempt before EWDK

Official repository commit: `67d81f217bc01edf7a4320e4911c11065635acfa`. Original solution: `third_party/upstream/Windows-driver-samples/video/IndirectDisplay/IddSampleDriver.sln`. Original sources/projects unchanged.

```powershell
MSBuild.exe `
  third_party/upstream/Windows-driver-samples/video/IndirectDisplay/IddSampleDriver.sln `
  /t:Build /p:Configuration=Debug /p:Platform=x64 /nologo /v:minimal
```

**Actual:** exit 1, MSB8020, missing `WindowsUserModeDriver10.0` and `WindowsApplicationForDrivers10.0`. Raw log is retained in private evidence. No successful driver binary or catalog. Do not retarget the driver as an ordinary DLL to hide this failure.

After approved toolchain setup, run `./scripts/windows/Build-IddBaseline.ps1 -MSBuild <full-path-to-approved-msbuild.exe>` from any directory. It checks the source pin and tracked-source cleanliness and writes a unique build log. For EWDK, run it inside that kit's initialized build environment. It never installs the output.

## Baseline-to-product requirements — implemented, runtime pending

Only after baseline build success: copy the small sample into windows/driver, preserve license/attribution, replace sample identity/INF GUIDs and DeviceGroupId, enumerate exactly one SweetDisplay monitor, reconcile monitor/target mode callbacks with validated EDID data. Preferred 2400x1080@60; fallback 1280x576@30/60 and 1920x864@30. Verify both size-query and undersized-buffer paths in mode callbacks.

The requested full name `SweetDisplay AMOLED` exceeds the 13-byte EDID monitor-name text field. Do not truncate a byte array unsafely or fake a 19-character EDID descriptor. Use a valid short EDID name and investigate a documented monitor INF/device friendly-name mapping for the full display label; verify what Settings actually displays. Implemented valid short EDID name plus companion MONITOR\SWT0001 INF; runtime display-name verification remains pending.

## Deployment gate — build passed, signing/runtime pending

Build success is not driver installation or enumeration. Before loading a development driver, inspect signing requirements and current machine policy read-only. Present the exact certificate trust/test-signing changes and reboot implications if required. Stop for permission before TESTSIGNING, Secure Boot changes, debugging configuration or certificate trust changes. Do not disable protections speculatively. Driver sample app creates a software device; running it is a separate deployment step.

Acceptance: Settings shows an additional SweetDisplay monitor and extends the desktop; QueryDisplayConfig confirms active mode/refresh; disconnect/removal is clean. Display number 3 is assigned by Windows and is not a guaranteed persistent identity.

Official references: [IddCx model](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview), [sample](https://github.com/microsoft/Windows-driver-samples/tree/67d81f217bc01edf7a4320e4911c11065635acfa/video/IndirectDisplay), [WDK setup](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk).


