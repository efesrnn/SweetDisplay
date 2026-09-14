# Architecture — design, not implementation

Windows desktop → IddCx UMDF driver → bounded driver/host frame interface → SweetDisplayHost → transport adapter → device displayd → discovered Xiaomi display pipeline → AMOLED.

Phone touchd later consumes evdev and submits USB HID absolute digitizer reports. Camera capture later feeds UVC gadget; it is not on the display critical path. Services start from appliance init, with no manually opened phone app.

Driver owns monitor lifecycle, modes, D3D render-adapter selection, swapchain lifetime and fast bounded frame copies. Host owns USB discovery, codec selection, connection state, frame pacing, telemetry and logging. Host starts as a console development tool; service packaging follows working frame proof. Driver runs in Session 0; do not assume shared user desktop or arbitrary process handles.

Connection states: disconnected → connecting → negotiating → streaming → draining/disconnected. Reconnect creates a new session/generation and invalidates queued frames. Initial host queues hold at most two frames and favor freshness; codec reference dependencies constrain encoded-frame dropping.

No requirement depends on Android mirroring, HDMI adapters, replacement SoCs, or modifying Qualcomm firmware. Reusing published downstream drivers with minimal userspace is a strategy, not proof that Android vendor services can be removed without replacement. Camera is especially uncertain.

See DISPLAY_PIPELINE, USB_PROTOCOL, TOUCH, CAMERA and BUILD_DEVICE. Original battery/charging hardware stays intact. Batteryless operation is outside V1.
