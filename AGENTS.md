# SweetDisplay operating constraints

Read STATUS.md, docs/DEVICE_FACTS.md and docs/RECOVERY.md before hardware work.
If present, read ignored .local/OPERATOR_NOTES.md for device-specific operator state.
Public engineering scope is in docs/PROJECT_BRIEF.txt; raw conversations are private.

- Respect the owner's device unlock/wait state. Never bypass waiting, rebind an account,
  reset, sign out, relock or modify partitions without explicit authorization.
- Keep read-only inventory bounded; private raw identifiers and host details remain ignored.
  Do not treat an empty device list as proof that previously working Fastboot is broken.
- Destructive hardware work requires a concrete proposal, matching recovery images and
  explicit approval. Never alter EFS/modem/persist/fsg or use unofficial unlock/EDL bypasses.
- Build the pinned Microsoft sample unchanged before deriving the driver; this gate passed.
  No silent installs, TESTSIGNING, Secure Boot, HVCI, debugging or certificate-trust changes.
- Keep sources pinned; upstream trees, proprietary payloads and signing material stay local.
  Preserve license notices. Do not infer runtime compatibility from source branch names.
- Update STATUS and TEST_LOG after verified milestones; distinguish plans from measured results.
- Run scripts/windows/Test-PublicRepository.ps1 before a commit/push. Report findings by
  path/category without printing secret values. Do not force-add ignored files or publish
  raw logs, local operator notes, personal paths, device serials or account identifiers.
