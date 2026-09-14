# Milestones and gates

| Order | Deliverable | Acceptance / gate |
|---|---|---|
| 0 | Foundation / inventory / recovery plan | Source-attributed facts; no invented device values; repository and evidence saved |
| 1 | Original Microsoft sample | VERIFIED unchanged Debug x64 build with EWDK 26100.6584 |
| 2 | SweetDisplay driver | One valid monitor, preferred 2400x1080@60 and fallback modes; build then explicit signing/configuration review |
| 3 | Windows extension | Settings + QueryDisplayConfig verify active extended display; not yet achieved |
| 4 | Driver-to-host proof | Moving actual monitor content produces timestamped frames in host; bounded queue, reset/cancel tests |
| 5 | Encoder / transport foundations | Enumerate and actually test MFTs; protocol parser conformance; no device required |
| 6 | Manual unlock + recovery ready | Verify unlock and exact stock image set before any custom boot |
| 7 | Temporary minimal boot | Verify support, reliable debug shell, no persistent writes; unsupported temporary boot → stop |
| 8 | Local graphics / input | Color patterns without SurfaceFlinger; reliable local touch |
| 9 | Raw end-to-end | Moving Windows window appears on phone at 800x360@10, measured latency |
| 10 | Compression / scale | Hardware decode capability measured; 1280x576@30 → 2400x1080@30 → 60 |
| 11 | Native touch | Absolute touchscreen mapped to the correct display; Spotify control test |
| 12 | Camera / UVC | Local capture then Windows Camera 720p30; simultaneous bandwidth/thermal tests |

120 Hz and batteryless operation are separate future projects. Maintain STATUS and TEST_LOG at every meaningful attempt; never turn an architectural target into a success claim.

