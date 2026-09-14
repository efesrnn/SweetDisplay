# Camera — later milestone, NOT YET TESTED

Camera must not delay display/touch. Sensor identity, camera driver/ISP interface and vendor service requirements are unknown. Inventory permitted camera configuration/HAL/module filenames now; after unlock correlate sensors, CSI/ISP, media graph and V4L2 capabilities with exact stock vendor components.

Preference order: native V4L2 capture, minimal Qualcomm/vendor components, compatibility layer only if needed. A /dev/video node does not establish usable camera capture: it may be a decoder, metadata or proprietary ISP endpoint. Camera HAL dependency on Android services is a material possibility, not yet demonstrated. Do not redistribute proprietary blobs; local files from the owner's stock firmware remain ignored.

Prove capture to a local diagnostic buffer first, including exposure/color/stride/timestamps. Then configure UVC gadget formats/endpoints matching actual producer capability and feed buffers through its userspace streaming interface. Declaring descriptors alone does not produce video. Target name SweetDisplay Camera; initial 1280x720@30, then 1920x1080@30.

Test Windows Camera, OBS and optionally browser/WebRTC. Record negotiated UVC format (e.g. MJPEG versus raw), frame rate, drops and actual USB speed. Uncompressed YUY2 720p30 is 55.296 MB/s payload alone, so format and shared display bandwidth require measurement. Keep logging capture/decode/encode and thermal load.

References: [V4L2](https://docs.kernel.org/userspace-api/media/v4l/v4l2.html), [Linux UVC gadget](https://docs.kernel.org/usb/gadget_uvc.html).
