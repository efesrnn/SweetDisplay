# SWEETDISPLAY FINAL-BOOT BRINGUP — VERIFIED THROUGH V11

Current result: **VERIFIED.** V11 produced stable SweetDisplay-owned pixels,
enforcing SELinux, physical-touch coordinate updates, read-only DWC3 discovery
and a bounded normal reboot to healthy unchanged stock MIUI. HARD STOP: no
FINAL-USB, media, HID or camera work was started.
Historical FINAL-BOOT 1 FAILED, 1A VERIFIED and 1B VERIFIED are unchanged.

## Proven result

One owner-authorized V10 RAM-only boot passed the one-device, product `sweet`,
unlocked and bootloader-Fastboot checks. Sending and Booting returned OKAY;
host dispatch was 2026-09-25 17:07:09 +03:00, command duration 0.767 seconds.
V9 was reviewed offline and never live-booted.

| Acceptance item | Evidence / status |
|---|---|
| Stock kernel and DTB | Exact pinned hashes preserved; no kernel build |
| First-stage / enforcing diagnostic | V10 kernel log: enforcing at 3.763366 s; `V10_COLD_E0` at 4.665404 s |
| DRM query, framebuffer and modeset | V10 code reaches its render path only after successful connector/CRTC/dumb-buffer/ADDFB/mmap/legacy SETCRTC; `V10_PIXELS_E0` at 4.688743 s |
| Physical AMOLED pixels | Owner confirms SweetDisplay; private owner photo shows V10, DISPLAY OK and readable diagnostic text |
| SELinux | Photo shows ENFORCING; kernel marker AVCs have permissive=0 |
| Physical touch | Owner confirms changing coordinates while dragging, including 489 / 1810 |
| Goodix attribution | V10 kernel contains Goodix lifecycle activity and failed alternate FTS probe; unchanged-stock input inventory names goodix_ts. Diagnostic accepts Goodix/touch-labelled multitouch devices. Exact selected evdev name was not independently emitted by V10 |
| DWC3 read-only discovery | Photo shows USB CONTROLLER DETECTED; code checks existing DWC3/UDC paths without gadget configuration |
| Bounded reboot | Owner confirms automatic MIUI return at timer expiry without intervention; V10 kernel records normal restart at 184.428210 s. No second host boot/reboot was sent |
| Stock return | Same-device authorized ADB, boot complete, V14.0.2.0.TKFTRXM, mtp,adb config/state, Enforcing; battery health good, 100%, 28.3 C at resumed check |
| Stable readable motion | V10 FAIL retained. V11 PASS: owner confirms only coordinate numbers change while holding/dragging; no wave/fade, transient text loss or instability |
| V11 completion evidence | Enforcing 3.685606 s; `V11_COLD_E0` 4.527686 s; `V11_PIXELS_E0` 4.562788 s; normal restart 184.522019 s |
| Final stock health | Same device boot-complete on V14.0.2.0.TKFTRXM, mtp,adb config/state, Enforcing; battery health good, 98%, 31.9 C |

Observation was interrupted before transport return. The first resumed host
stock confirmation was 18:27:03 +03:00; this is NOT the return time or an
80-minute boot duration. Kernel-relative reboot timing is retained separately.
Dropbox entry wall-clock labels are stale and must not be used as host timing.
Owner has now explicitly confirmed hands-off automatic return; no physical
fallback was needed.

## Revision progression and exact candidate

V3/V4 closed missing RAM mountpoint directories; V5 reached DRM; V6 moved to
stock coldboot completion and init-owned rollback; V7 added the exact volatile
stock backlight action; V8 proved connected DSI/modes and second-query failure.
V9 supplied missing connector property/value arrays. Exact stock recovery
disassembly then revealed a second-pass count-growth guard missing in V9.
V10 adds that guard with bounded retry and failure cleanup, preserving V9.
Count growth was an offline code finding, not a measured V8 failure cause.

- Image SHA-256: `aefc8d233309bb0ba1f07497b634026ca1d164ed2f44057eb8ca0123e273892d`.
- Diagnostic SHA-256: `ac60147129d1c6668dbfcc0d84301dc8a43d5b878092648409fed84d1de7058f`.
- Kernel SHA-256: `764b10ccaf74d2fb0a4569d1254bfc21251a3351afe296d149f639c3d65c7bfc`.
- DTB SHA-256: `8f1b614d87eb9c6835ecf56cfe6f7a338855a5317dad1eab6391222e04804d10`.

V10 is a reproducible 25,673,728-byte v2/4096-page image with a 73-entry
minimal ramdisk. Runtime pixels prove the existing legacy modeset path works
for this bounded diagnostic; an atomic rewrite is not justified by this result.
Exact running 4.14.190 source remains unavailable; the local 4.14.180 source
was corroborating material, not claimed as an exact downstream source match.

## Boundary and remaining work

No flash/erase/format/partition-write command, AVB/bootloader mutation, persistent
installation, SELinux weakening, USB gadget/IP/network configuration, commit or
push occurred. Stock-return health is verified; full partition byte-equivalence
was not re-read. Raw logs, owner photo, identifiers and all images remain private.

Source review finds V10 clears and redraws its currently scanned-out framebuffer
on every input SYN_REPORT batch. There is no off-screen completed frame or
flip-completion ownership boundary; MS_ASYNC is not a presentation fence. This
is a concrete redraw defect and a strong explanation for the symptom, not a
measured proof of the panel's exact scan timing or a hardware-damage diagnosis.

V11 changes only frame presentation and its bounded error handling: render into
a second dumb buffer, queue normal DRM page flip with an event cookie, and wait
for matching completion before reusing the old front buffer. No async-flip or
single-buffer fallback; malformed events, ioctl failure or a two-second event
wait timeout terminate through existing init-owned rollback. Stock kernel/DTB,
enforcing, backlight action, coldboot gate and 180/240-second timers remain.
Exact stock recovery's drmModePageFlip wrapper confirms the 24-byte structure
and ioctl 0xc01864b0. At the time this motivated V11 it did not by itself prove
runtime panel support; the subsequent V11 live run did prove page-flip event
delivery, stable presentation and bounded healthy recovery for this diagnostic.

V11 image SHA-256:
`25c31a22fd5645c4604bc01b9f179a7ae8dc16e7f8d3f3f93dfe7112321bfbe4`.
Two offline builds match; 18 compile-time event-parser cases, static ELF,
stock-payload/manifest audit and unchanged V10 diagnostic hash pass.

One fresh owner-authorized V11 RAM-only attempt passed the exact-device/hash
preflight. Fastboot reported Sending OKAY 0.632 s and Booting OKAY 0.146 s;
total host command duration was 0.789 s. Owner then confirmed a stable readable
V11 screen, changing physical-touch coordinates, no V10 wave/fade or text loss,
SELINUX ENFORCING and USB CONTROLLER DETECTED. Kernel evidence independently
proves normal timer-path restart at 184.522019 s. Host observed ADB return and
stock boot completion. No second V11 attempt occurred.

Do not repeat V10 or V11. FINAL-BOOT BRINGUP is complete. FINAL-USB is a separate
future authorization, then FINAL-MEDIA; neither streaming, HID nor camera is
part of this result. No public unique device identity, owner photo or raw kernel
record is included; those remain ignored/private.
