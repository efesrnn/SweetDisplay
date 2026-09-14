# SweetDisplay AMOLED driver prototype

Derived only after the unchanged Microsoft IndirectDisplay baseline passed on
2026-09-13. Debug x64 builds with EWDK 26100.6584, including normal code analysis
and catalog generation. No installation or live display enumeration yet.

`SweetDisplayDriver` advertises one indirect wired virtual monitor. The shared
mode list prefers 2400x1080@60 and includes 1280x576@30, 1280x576@60 and
1920x864@30. EDID uses private prototype identity SWT0001, zero serial, unspecified
physical dimensions and the short name SweetDisplay. This is synthetic Windows
metadata, not the phone panel's measured EDID/timing/color calibration.

`SweetDisplayMonitor` is a separate monitor-class INF mapping MONITOR\SWT0001 to
the full name SweetDisplay AMOLED and preferred resolution. Actual Settings name
and mode selection require runtime verification with both signed packages.

`SweetDisplayDevice` is a development software-device enumerator. Run only during
approved deployment after package signing/installation. It stays open until X,
then closes the software device. Device creation is not proof of driver startup.
This tool is not the future transport/encoder host service.

The sample's D3D swap-chain loop is retained. Frames are released without encoding
or USB transport. No image has been delivered to the phone. HDR, touch, camera,
multi-instance support and recovery from every runtime failure are not implemented.

From the repository root, build without installing:

```bat
scripts\windows\Build-SweetDisplay.cmd D:\
```

Replace D:\ with the mounted EWDK root. Build outputs use ignored out/windows;
logs use ignored docs/evidence/private. Obsolete exploratory build outputs were archived under ignored out/legacy-builds.
No installer, certificate generation, trust import or boot-policy change runs.

Attribution: driver, enumerator and project scaffolding derive from Microsoft's
[IndirectDisplay sample](https://github.com/microsoft/Windows-driver-samples/tree/67d81f217bc01edf7a4320e4911c11065635acfa/video/IndirectDisplay),
commit 67d81f217bc01edf7a4320e4911c11065635acfa. Microsoft copyright notices are
retained. The complete upstream MS-PL is in LICENSE-MS-PL.txt and applies to the
sample-derived files and their modifications. This is not a license selection for
unrelated repository components. The ignored upstream checkout remains unchanged.

Monitor INF structure follows Microsoft's
[monitor INF guidance](https://learn.microsoft.com/en-us/previous-versions/windows/drivers/display/monitor-inf-file-sections).
See ../../docs/BUILD_WINDOWS.md and ../../docs/DEPLOYMENT_WINDOWS.md for validation
and the outstanding deployment gate.

