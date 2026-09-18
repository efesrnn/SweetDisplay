# Milestones and gates

## Active PC-only sequence (2026-09-16)

PHASE 1 frame proof and PHASE 2 shared-GPU handoff are VERIFIED. The owner's
current sequence supersedes the older milestone numbering below:

1. 3A: establish actual new-frame capacity and >=30-minute soak, bounded resources,
   queue and healthy reconnect. VERIFIED by the fresh 1810.0007551-second soak
   and reconnect; Host 55.662408 FPS. Historical soak UNKNOWN; 60 FPS not proven.
   See PHASE3A_RESULTS.md. Its stop/report gate was completed.
2. 3B: real GPU-native hardware H.264, progressive modes, decode/content proof.
   Latest owner request and explicit in-chat UAC authorization cover this phase only.
   VERIFIED under latest bounded-admission acceptance: recovered1810.0008772s
   sustained run,93069 admitted=encoded=decoded frames,source56.241409FPS versus
   output51.419312FPS.99 transient admission drops remain explicit; strict all-Host
   assertion fails. Stable observed resources/latency,exact accounting,clean and
   forced-Host reconnect/content verification pass; latter has1 transient drop.
   Early upstream slowdown and underlying token timing UNKNOWN. Previous short
   results/evidence unchanged. Actual60FPS and lossless-all-Host capacity unproven.
   No driver/deployment/security changes. Stop after this phase; do not start3C.
   See PHASE3B_RESULTS.md. Stop/report before 3C.
3. 3C: actual versioned bounded protocol over localhost.
4. 3D: live Display 3 -> encoder -> protocol -> decoder -> simulator window.
5. 3E: same-protocol absolute touch loop, coordinate and disconnect cleanup tests.
6. 3F: codec/protocol/reconnect/input tests and logical future daemon contracts.

The current authorization ends after PHASE 3B; stop before PHASE 3C.
Report a sustained-rate shortfall before major architecture changes.
Phone work, real USB gadget and
camera work remain outside this run. Preserve all Windows security settings.
Current measurements and limits: PHASE3_VALIDATION.md and STATUS.md.

## Broader project roadmap

| Order | Deliverable | Acceptance / gate |
|---|---|---|
| 0 | Foundation / inventory / recovery plan | Source-attributed facts; no invented device values; repository and evidence saved |
| 1 | Original Microsoft sample | VERIFIED unchanged Debug x64 build with EWDK 26100.6584 |
| 2 | SweetDisplay driver | One valid monitor, preferred 2400x1080@60 and fallback modes; build then explicit signing/configuration review |
| 3 | Windows extension | VERIFIED: owner confirmed Settings Display 3; active SWT0001 2400x1080@60 extended desktop |
| 4 | Driver-to-host proof | VERIFIED PHASE 2: actual changing content, bounded shared GPU queue, crash cleanup and reconnect |
| 5 | Encoder / transport foundations | Enumerate and actually test MFTs; protocol parser conformance; no device required |
| 6 | Manual unlock + recovery ready | Verify unlock and exact stock image set before any custom boot |
| 7 | Temporary minimal boot | Verify support, reliable debug shell, no persistent writes; unsupported temporary boot → stop |
| 8 | Local graphics / input | Color patterns without SurfaceFlinger; reliable local touch |
| 9 | Raw end-to-end | Moving Windows window appears on phone at 800x360@10, measured latency |
| 10 | Compression / scale | Hardware decode capability measured; 1280x576@30 → 2400x1080@30 → 60 |
| 11 | Native touch | Absolute touchscreen mapped to the correct display; Spotify control test |
| 12 | Camera / UVC | Local capture then Windows Camera 720p30; simultaneous bandwidth/thermal tests |

120 Hz and batteryless operation are separate future projects. Maintain STATUS and TEST_LOG at every meaningful attempt; never turn an architectural target into a success claim.

