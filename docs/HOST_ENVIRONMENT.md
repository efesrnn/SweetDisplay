# Build environment

The validated driver toolchain is the self-contained Microsoft EWDK 26100.6584
with bundled Visual Studio 2022 Build Tools 17.14.5. It supplies SDK/WDK headers,
libraries, driver toolsets, catalog tools and analysis components.

See [WDK setup](WDK_SETUP.md) and [Windows build](BUILD_WINDOWS.md). Use the mounted
EWDK path supplied locally; no developer username or absolute installation path is
required in the sources. Other installed SDK/Visual Studio combinations have not
been validated for this driver.

The encoder probe additionally needs CMake, MSVC and a Windows SDK. Phone inventory
needs an existing adb/fastboot executable selected through PATH or an explicit local
parameter. Do not install or replace phone tooling as part of Windows driver builds.

Raw workstation inventory, GPU/OS fingerprints, executable paths and security-policy
observations are kept under ignored docs/evidence/private.
