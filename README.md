# SweetDisplay

Reuse a Redmi Note 10 Pro motherboard, AMOLED panel, touchscreen and camera as a
dedicated USB companion for Windows. This is an experimental hardware/software project.

The unchanged Microsoft IndirectDisplay sample and the SweetDisplay driver derivative
have built successfully with EWDK 26100.6584. The unsigned prototype advertises one
virtual monitor, preferring 2400x1080 at 60 Hz. Driver installation, Windows desktop
extension and actual phone video output have **not** been verified.

Start with [status](STATUS.md), [Windows build](docs/BUILD_WINDOWS.md),
[architecture](docs/ARCHITECTURE.md) and [project scope](docs/PROJECT_BRIEF.txt).
The device services and kernel patch directories are placeholders.

Before committing or pushing, run:
```powershell
pwsh -NoProfile -File scripts/windows/Test-PublicRepository.ps1
```

See [publication policy and audit](docs/REPOSITORY_HYGIENE.md) for excluded data and
known limits. ISO media, firmware, full upstream trees, outputs, raw inventories and
signing material belong in ignored local directories. Never use git add -f to
publish these files. No project-wide open-source license has been selected; consult
[licensing scope](LICENSE.md) and [third-party notices](NOTICE.md).
