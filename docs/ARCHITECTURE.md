# Architecture — implemented baseline and future design

## Current implementation

PHASE 2 is VERIFIED: SWT0001 desktop -> IddCx -> one GPU copy -> three
same-adapter shared D3D11 textures -> SweetDisplayHost. The restricted documented
IddCx IOCTL interface carries metadata/control. Keyed mutexes protect GPU ownership;
the bounded newest-frame policy replaces ready frames and never overwrites a held
texture. Host termination/cleanup and reconnect have been verified. Full frames
are not downloaded to CPU memory. See FRAME_HANDOFF.md and DISPLAY_PIPELINE.md.

PHASE 3A measures capacity and long-run stability without replacing this path.
Hardware encoding, localhost protocol and the simulator are subsequent gates,
not implemented capabilities. Future device-specific behavior is UNVERIFIED.

## Future device path (not implemented)

Windows desktop → IddCx UMDF driver → bounded driver/host frame interface → SweetDisplayHost → transport adapter → device displayd → discovered Xiaomi display pipeline → AMOLED.

Planned touchd will consume device input and send negotiated TOUCH messages through the same logical protocol as the simulator. Windows-side injection and any later physical HID integration remain unimplemented/unverified. A future camera service is outside the current scope. Services are intended to start from appliance init, with no manually opened phone app; phone-specific startup and hardware behavior remain UNVERIFIED.

Driver owns monitor lifecycle, modes, D3D render-adapter selection, swapchain lifetime and fast bounded frame copies. Host owns USB discovery, codec selection, connection state, frame pacing, telemetry and logging. Host starts as a console development tool; service packaging follows working frame proof. Driver runs in Session 0; do not assume shared user desktop or arbitrary process handles.

Planned transport states: disconnected → connecting → negotiating → streaming → draining/disconnected. Transport reconnect will create a new logical session and invalidate queued encoded frames. The implemented GPU handoff has three total slots and one outstanding Host lease; the driver's source epoch remains stable across ordinary Host reconnects. Future encoder/transport queue capacities are not yet implemented. Codec reference dependencies constrain encoded-frame dropping.

No requirement depends on Android mirroring, HDMI adapters, replacement SoCs, or modifying Qualcomm firmware. Reusing published downstream drivers with minimal userspace is a strategy, not proof that Android vendor services can be removed without replacement. Camera is especially uncertain.

See DISPLAY_PIPELINE, USB_PROTOCOL, TOUCH, CAMERA and BUILD_DEVICE. Original battery/charging hardware stays intact. Batteryless operation is outside V1.
