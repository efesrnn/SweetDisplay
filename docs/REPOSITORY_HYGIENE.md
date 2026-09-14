# Public repository audit — 2026-09-14

The approved cleanup preserves local evidence and build reproducibility. No project
commit, staging, remote push, history rewrite or external upload was performed.
The main repository had no tracked files, commits, remote or Git objects to clean.
Synthetic checker tests create their own separate ignored fixture repositories.

| Audit category | Finding and applied action |
|---|---|
| Files not intended for publication | Raw operator brief, inventories, local host facts and original document snapshots are private; public documents contain engineering summaries |
| Build artifacts | Active outputs stay in out; three obsolete x64 output trees moved to out/legacy-builds; generated binary/cache extensions and directories ignored |
| Large generated/installation files | Approximately 20 GB EWDK ISO remains at its existing mounted-media location, ignored; no file over 1 MiB is allowed through the publication checker without review |
| Firmware/proprietary blobs | No stock Android/Xiaomi firmware or proprietary phone payload found in the inspected repository; future media/blob extensions and firmware/downloads directories excluded |
| Upstream trees | Microsoft checkout remains ignored and unchanged; public URL/commit pins retained; full Xiaomi tree absent; downloaded Xiaomi Makefile moved into private evidence |
| Credentials and signing material | No high-confidence private-key/token signature or credential/certificate filename found outside synthetic tests; signing/key/certificate formats excluded |
| Personal information | Hard-coded personal Fastboot path removed; raw conversation, workstation fingerprints, account/wait details and host security readings retained locally; no known personal path/identifier remains in the publishable text |
| Logs | All raw evidence files moved to ignored private storage; public TEST_LOG contains reviewed results; log/dump/inventory formats excluded |
| Attribution | Microsoft MS-PL text preserved; root NOTICE and scoped LICENSE added; no new project-wide license chosen; Xiaomi reference contains no redistributed upstream code |
| Ignore coverage | .gitignore covers upstream, outputs, local state, evidence, media, firmware, binaries, editor caches, environment secrets and signing material |

Original changed documents were copied into an ignored local backup and their
SHA-256 values compared before edits. Nothing was discarded. The local operator
notes preserve hardware restrictions that are inappropriate for a public README.

Git author name/email are configured; the configured email uses GitHub noreply.
The actual identity is not reproduced here. Commit authorship is separate from file
content and will still be visible under Git's usual rules.

## Publishable layout

```text
SweetDisplay/
  .gitignore
  AGENTS.md
  CMakeLists.txt
  README.md
  STATUS.md
  LICENSE.md
  NOTICE.md
  docs/                    # public design/build/recovery/validation summaries
    evidence/README.md     # policy only; all raw evidence ignored
  windows/
    driver/
      SweetDisplay.sln
      Directory.Build.props
      LICENSE-MS-PL.txt
      README.md
      SweetDisplayDriver/  # C++/headers/INF/project sources
      SweetDisplayDevice/  # enumerator sources/project
      SweetDisplayMonitor/ # companion INF source
      tests/               # EDID/mode test source/project
    tools/                 # encoder probe source
    host/                  # planned host component
    common/                # placeholder
  device/                  # planned device-service placeholders
  kernel/configs/          # project configurations only
  kernel/patches/          # project patches only
  scripts/windows/        # build, inventory and publication checks
  scripts/linux/          # placeholder
  third_party/
    README.md
    SOURCES.json           # official URLs and commit pins
```

Local-only directories/files include .local, out, third_party/upstream,
docs/evidence/private, downloads, firmware and the existing EWDK ISO. They remain
on disk but are absent from Git's normal publishable file list.

## Verification and future use

Run `pwsh -NoProfile -File scripts/windows/Test-PublicRepository.ps1` before every
commit or push. It reads publishable working-tree files, actual index blobs and
blobs reachable from all local refs. It checks excluded paths even when tracked,
common credential patterns, home paths, email addresses, local username occurrences,
unexpected binary text and size limits. It emits categories/paths, never matched values.

The checker has nine passing regression scenarios: clean source, index-only secret,
history-only secret, ignored media, forced tracked media, UTF-16 secret, personal
home path, unknown binary and large payload. Run its `.Tests.ps1` companion to repeat.
Fixtures contain synthetic data only and are retained under ignored out.

This is a manual pre-publication command, not an automatically installed Git hook.
Pattern scanning cannot prove the absence of every possible confidential value,
especially custom/encoded credentials, commit-message content or future changes.
Review the staged diff and file list; never force-add private content. A future
tracked secret requires removal from published history and credential rotation;
adding an ignore rule alone does not remove committed data.

Before an actual public push, the project owner should select the license for
independently authored components; current LICENSE.md explicitly leaves that choice open.

Verified final public set: 63 files, approximately 147 KiB in total; largest file
approximately 26 KiB. Nine representative private/generated paths are ignored.
Local Markdown links and PowerShell syntax checks passed. The MS-PL license matches
the pinned upstream file byte-for-byte and the upstream source diff is empty.
