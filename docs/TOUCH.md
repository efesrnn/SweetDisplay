# Touch — design, NOT YET TESTED

Inventory /proc/bus/input/devices first; after minimal boot, enumerate evdev capability bits and ABS ranges, never hardcode event node numbers. Log EV_ABS multitouch slots/tracking IDs/positions/pressure when exposed, BTN_TOUCH and SYN_REPORT; handle SYN_DROPPED by resynchronizing device state. Touch vendor, coordinate limits and orientation remain unknown.

Establish local down/up/contact continuity. Normalize observed axis ranges, apply explicit rotation/mirroring, then scale and clamp to advertised logical coordinates. Test each corner and moving contacts for 0/90/180/270 degree orientation. Record touch timestamps and clock domains.

Then use HID gadget with Digitizers usage page and Touch Screen top-level collection, initially one contact. Descriptor must carry correct absolute X/Y, tip state, contact ID and required contact-count/capability reports; validate against Microsoft's touchscreen requirements, not a mouse or keyboard gadget example. Multi-contact framing follows a verified single-touch descriptor.

Windows touch-to-monitor association is a separate milestone. A generic digitizer may map to the primary monitor until configured; USB enumeration alone does not prove targeting of the indirect display. Investigate documented association/calibration and validate all monitor arrangements and rotations.

Acceptance: move Spotify onto the SweetDisplay monitor, touch Next on the phone, and observe that exact control activate. No Spotify account automation or messaging is involved. Buttons may later use a separate consumer-control collection.

References: [Linux HID gadget](https://docs.kernel.org/usb/gadget_hid.html), [Windows touchscreen collections](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchscreen-required-hid-top-level-collections).
