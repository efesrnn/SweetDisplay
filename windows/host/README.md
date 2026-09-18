# SweetDisplayHost — PHASE 2

Console consumer for the real SweetDisplay IddCx source. Requires an elevated
interactive user on the development PC and the frame-handoff driver interface.
It does not encode, transmit, install a driver, or alter security settings.

Build using `scripts\windows\Build-FrameHandoff.cmd D:` with the actual EWDK drive.
Outputs are ignored under out/host/, out/host-tests/ and out/windows/. The shared
metadata/queue tests run as part of the build. Signing/updating are separate steps.
For Host-only iteration use `Build-FrameHandoff.cmd D: host-only`; this does not
rebuild or update the driver package.

Create an ignored evidence directory, then run elevated:

```powershell
out\host\SweetDisplayHost.exe --output docs\evidence\private\phase2\manual --seconds 35
```

Host discovers the private device interface, queries the actual render LUID,
creates three GPU textures and connects through the IddCx IOCTL callback.
Once per second it reports delivered/source FPS, drops, queue depth/high-water,
last ID and age. Metadata CSV and a final JSON report go to the requested
evidence directory. Ctrl+C requests clean shutdown. The default run is 35 seconds;
--seconds supports up to 600 seconds. --sessions 2 closes and reconnects with
fresh allocations, keeping the driver running between sessions.

--sample --nonce HEX validates the existing FrameProbe pattern using only two
640-pixel rows (5,120 bytes per frame). This optional diagnostic readback is not
a full-frame download. --slow-ms 100 slows consumption; --hold-ms 1000 holds
a GPU lease for crash testing. --inspect-seconds 4 checks that source frames
advance while no streaming Host is connected.

Automated real-device integration, from elevated Windows PowerShell:

```powershell
scripts\windows\Test-FrameHandoff.ps1 -RunName my-acceptance-run
scripts\windows\Verify-FrameHandoffEvidence.ps1 -EvidenceDirectory docs\evidence\private\phase2\my-acceptance-run
```

The integration test creates a changing window on SWT0001, exercises two
35-second sessions, a slow consumer and termination of its own test Host while
holding a GPU lease. It verifies continued source activity and reconnection,
reads PnP/security state and checks for new driver/system failure events.
It does not stop the device helper or install/sign packages. Do not run
concurrent copies against the one supported Host connection.

See [FRAME_HANDOFF.md](../../docs/FRAME_HANDOFF.md) for the supported API
references, synchronization, measured results and development security boundary.
