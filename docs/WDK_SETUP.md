# WDK setup decision — verified 2026-09-13

## Son durum — ISO sağlandı

Kullanıcı EWDK_ge_release_svc_prod1_26100_250904-1728.iso dosyasını ekledi. ISO salt okunur D: olarak bağlandı; Version.txt 26100.6584, bundled VS version.txt 17.14.5 gösteriyor. Artık eksik WDK indirmesi beklenmiyor. Derleme DLL/EXE ve Inf2Cat aşamalarını geçti; tam build .NET araç yüklemesi/kod analizi nedeniyle başarısız. ISO ZoneId=3 ve doğrudan .NET yüklemesinde 0x80131515 doğrulandı. Yalnızca bu dosya için Unblock-File onayı bekleniyor; uygulanmadı. Ayrıntılar TEST_LOG ve STATUS içinde. Aşağıdaki kurulum seçimi notları önceki değerlendirmeyi belgeliyor.

Current task: Windows build only. The owner independently verified Fastboot successfully; it is not a prerequisite or blocker for this milestone. No phone operations, installers, security changes or new build attempts were run during this assessment. Rebuild only after the owner's installation-complete confirmation.

## Exact installed environment

Evidence: locally retained private component inventory, read-only registry, vswhere and filesystem checks.

- Visual Studio **2026 Build Tools 18.4.11605.240**, MSBuild 18.4, MSVC 14.50 (compiler 19.50).
- SDK product name: **Windows Software Development Kit - Windows 10.0.26100.7705**. Its MSI DisplayVersion is **10.1.26100.7705**. Include/Lib directory is **10.0.26100.0**, which does not expose the installed servicing revision.
- .NET Framework v4 Full is installed (Release registry value 533509).
- Core x64/x86 C++ tools are registered. No VS 2022 installation is registered.

## Missing build components

| Component | Evidence / relevance |
|---|---|
| WindowsUserModeDriver10.0 MSBuild platform toolset | Absent in VS VC toolsets; original driver project fails MSB8020 |
| WindowsApplicationForDrivers10.0 platform toolset | Absent; original sample application fails MSB8020 |
| WDK headers/libraries and MSBuild integration | No Windows Kits/10/build or WDF include/lib trees; no iddcx.h, IddCxStub.lib, WdfDriverStubUm.lib or wdf.h found in the kit |
| Inf2Cat | Not found in the installed kit; required for driver catalog generation |
| C++ x64/x86 Spectre-mitigated libraries | Component not registered; sample explicitly enables Driver_SpectreMitigation=Spectre |
| WDK Visual Studio integration component | Not registered in the existing Build Tools instance |

SignTool and TraceWPP already exist in SDK 26100; do not list them as missing. ATL/MFC Spectre components are also absent, but this pinned native sample uses WRL, not ATL/MFC, so they are not demonstrated dependencies for this x64-only baseline. ARM64 build components are not needed for the Windows x64 milestone. IddCx/UMDF development files come with the WDK, not a separate phone driver or runtime download.

## Version compatibility

Microsoft's [supported-kit table](https://learn.microsoft.com/en-us/windows-hardware/drivers/other-wdk-downloads) pairs **WDK 10.0.26100.6584 with VS 2022**. It pairs **WDK 10.0.28000.2526 with VS 2026**. Thus, installing WDK 26100 alone into the existing VS 2026 Build Tools instance is not the documented supported pairing.

The existing SDK **26100.7705** and WDK **26100.6584** have the same build family. Their different QFE revisions do not, by themselves, require reinstalling the SDK; Microsoft requires matching build numbers, not equal QFEs. The compiler/integration pairing still needs resolving. [Kit versioning](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#kit-versioning).

## Minimum setup recommended for this milestone: EWDK 26100

This minimizes installation/integration steps, not necessarily download size. The self-contained EWDK includes its own compiler, SDK, WDK and driver build environment. It does not use or repair the separately installed VS 2026/SDK combination. [Official EWDK usage](https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/using-the-enterprise-wdk).

1. Open [Microsoft's EWDK download/license page for VS 2022](https://learn.microsoft.com/en-us/legal/windows/hardware/enterprise-wdk-license-2022). At the bottom choose **Windows 11, version 25H2 EWDK (released September, 2025) with Visual Studio Buildtools 17.14.0**. This is the 26100.6584 row's EWDK link from the supported-kit table. Choose 25H2, not the neighboring 26H1 download.
2. Download the ISO to a local drive and mount it in Windows. No separate Visual Studio IDE, SDK installer, WDK VSIX or global PATH change is needed for the EWDK command-line route.
3. Report that setup is complete and provide the ISO path or mounted drive letter. After that confirmation, we will initialize its **LaunchBuildEnv.cmd**, verify actual bundled versions/tools, and build the unchanged sample within that environment. Update 2026-09-13: EWDK 26100.6584 mounted read-only at D:, VS 2022 17.14.5 verified; baseline build passed after owner-approved ISO-only unblock.

Do not run a driver installer or change TESTSIGNING, Secure Boot, debugging, Memory Integrity, execution policy or certificate trust. No security-setting change is required to acquire the build tools; runtime deployment will be assessed separately.

## Conventional installer alternative, retaining installed SDK 26100

If a persistent IDE installation is preferred, install supported **VS 2022** alongside 2026 using Microsoft's [VS 2022 link in the supported-kit table](https://learn.microsoft.com/en-us/windows-hardware/drivers/other-wdk-downloads). Select Desktop development with C++, x64/x86 MSVC v143 Spectre libraries, and the Windows Driver Kit integration component (VS 17.11+). Then install **WDK 10.0.26100.6584** using the **WDK link in the 25H2 / 26100.6584 row**. Use the existing SDK 26100.7705. After confirmation, verify integration and build with VS 2022 MSBuild. This has more installation steps than EWDK; no need to perform both routes.

Keeping VS 2026 as the driver toolchain would instead require the supported 28000 SDK/WDK combination and appropriate integration (or 2026 EWDK). That is a different kit-family choice, not the minimum 26100 baseline setup requested here.

## Rebuild and adaptation sequence after confirmation

1. Verify actual selected MSBuild/compiler/SDK/WDK paths and version family; no automatic fallback to the existing VS 2026 MSBuild.
2. Verify upstream HEAD remains 67d81f217bc01edf7a4320e4911c11065635acfa and tracked sources are clean.
3. Run scripts/windows/Build-IddBaseline.ps1 with the selected MSBuild path from the initialized environment. Build Debug x64 only. Save full log and actual exit code; update STATUS.
4. If successful, immediately derive the small licensed sample into windows/driver. Implement one monitor with requested name **SweetDisplay AMOLED**, preferred **2400x1080 at 60 Hz**, coherent monitor/target modes and valid identity data. Update STATUS after verified build results.
5. Determine deployment requirements without changing Windows security. Success is only recorded after Windows Settings shows SweetDisplay AMOLED as an extendable display, with actual active mode verified. Compilation alone does not meet that criterion.


## Verified result — 2026-09-13

Owner-supplied EWDK 26100.6584 is sufficient for the current Debug x64 milestone;
no further WDK/VS installer is needed. The unchanged Microsoft sample built with
0 warnings/errors and SignMode=Off after the approved ISO file unblock. The
SweetDisplay derivative then built and passed mode/EDID and monitor INF/catalog
checks. Standalone VS 2026 still has no verified 26100 WDK integration; all these
successful builds use the self-contained EWDK. See STATUS and TEST_LOG.

