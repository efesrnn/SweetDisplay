package com.sweetdisplay.receiver;

import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.zip.CRC32;

final class Protocol {
    static final int HEADER_BYTES = 48;
    static final int FRAME_BYTES = 64;
    static final int MAX_AU = 4 * 1024 * 1024;
    static final int MAX_PAYLOAD = MAX_AU + FRAME_BYTES;
    static final int HELLO = 1;
    static final int CAPABILITIES = 2;
    static final int FRAME = 3;
    static final int TOUCH = 4;
    static final int CONTROL = 5;
    static final int TELEMETRY = 6;
    static final int HEARTBEAT = 7;
    static final int CAMERA_CONTROL = 8;
    static final int TOUCH_PROFILE_MINOR = 1;
    static final int FEATURE_VIDEO = 1;
    static final int FEATURE_TOUCH = 2;
    static final long TOUCH_DESCRIPTOR = 1L | (2L << 8) | (1L << 16)
            | (1L << 17) | (1L << 18) | (1L << 19);
    static final int TOUCH_CONFIGURE = 1;
    static final int TOUCH_READY = 2;
    static final int TOUCH_CONTACT = 3;
    static final int TOUCH_COORDINATE_MAXIMUM = 65535;
    static final int TOUCH_MAX_CONTACTS = 2;
    static final int TOUCH_CONFIG_FLAGS = 15;

    static final class Violation extends IOException {
        Violation(String message) { super(message); }
    }

    static final class Header {
        int type;
        int payloadBytes;
        long session;
        long sequence;
        long timestampNs;
    }

    static final class Message {
        final Header header;
        final byte[] payload;
        Message(Header header, byte[] payload) { this.header = header; this.payload = payload; }
    }

    static final class Frame {
        long id;
        long sourceNs;
        long pts100ns;
        int width;
        int height;
        int bytes;
        int flags;
        int crc;
        long sourceTicks;
        long sourceFrequency;
        byte[] accessUnit;
    }

    static final class TouchConfiguration {
        int coordinateMaximum;
        int targetWidth;
        int targetHeight;
        int maxContacts;
        int flags;
        long targetToken;
    }

    private Protocol() { }

    static Message read(InputStream input) throws IOException {
        byte[] wireHeader = new byte[HEADER_BYTES];
        readFully(input, wireHeader, 0, wireHeader.length);
        ByteBuffer b = ByteBuffer.wrap(wireHeader).order(ByteOrder.LITTLE_ENDIAN);
        require(b.get() == 'S' && b.get() == 'W' && b.get() == 'D' && b.get() == 'P', "magic");
        require(u16(b.getShort()) == 1, "major");
        require(u16(b.getShort()) == 0, "minor");
        Header h = new Header();
        h.type = u16(b.getShort());
        require(u16(b.getShort()) == HEADER_BYTES, "header size");
        h.payloadBytes = b.getInt();
        require(h.payloadBytes >= 0 && h.payloadBytes <= MAX_PAYLOAD, "payload limit");
        require(b.getInt() == 0 && b.getInt() == 0, "header flags/reserved");
        h.session = b.getLong();
        h.sequence = b.getLong();
        h.timestampNs = b.getLong();
        require(h.session != 0 && h.sequence != 0 && h.timestampNs != 0, "zero identity/time");
        validatePayloadSize(h.type, h.payloadBytes);
        byte[] payload = new byte[h.payloadBytes];
        readFully(input, payload, 0, payload.length);
        return new Message(h, payload);
    }

    static void write(OutputStream output, int type, long session, long sequence,
                      long timestampNs, byte[] payload) throws IOException {
        validatePayloadSize(type, payload.length);
        ByteBuffer h = ByteBuffer.allocate(HEADER_BYTES).order(ByteOrder.LITTLE_ENDIAN);
        h.put((byte)'S').put((byte)'W').put((byte)'D').put((byte)'P');
        h.putShort((short)1).putShort((short)0).putShort((short)type)
                .putShort((short)HEADER_BYTES).putInt(payload.length)
                .putInt(0).putInt(0).putLong(session).putLong(sequence).putLong(timestampNs);
        output.write(h.array());
        output.write(payload);
        output.flush();
    }

    static byte[] helloDevice(int negotiatedMinor) {
        ByteBuffer b = little(24);
        b.putInt(2).putShort((short)0).putShort((short)negotiatedMinor)
                .putInt(0).putInt(0).putLong(1);
        return b.array();
    }

    static byte[] capabilitiesDevice(int negotiatedMinor) {
        ByteBuffer b = little(32);
        b.putInt(1).putInt(MAX_AU).putInt(4096).putInt(2160)
                .putInt(negotiatedMinor >= TOUCH_PROFILE_MINOR
                        ? FEATURE_VIDEO | FEATURE_TOUCH : FEATURE_VIDEO)
                .putInt(0).putLong(negotiatedMinor >= TOUCH_PROFILE_MINOR
                        ? TOUCH_DESCRIPTOR : 0);
        return b.array();
    }

    static byte[] telemetry(long frames, long frameSequence, long frameId, long bytes) {
        return little(32).putLong(frames).putLong(frameSequence).putLong(frameId).putLong(bytes).array();
    }

    static byte[] drainAck() {
        return little(8).putInt(2).putInt(0).array();
    }

    static int validateHostHello(Message message) throws IOException {
        require(message.header.type == HELLO && message.payload.length == 24, "HELLO required");
        ByteBuffer b = little(message.payload);
        require(b.getInt() == 1, "host role");
        int minimum = u16(b.getShort());
        int maximum = u16(b.getShort());
        require(minimum <= 0 && minimum <= maximum, "minor negotiation");
        int clock = b.getInt();
        require(clock >= 0 && clock <= 1, "host clock");
        require(b.getInt() == 0 && b.getLong() == 1, "HELLO reserved/features");
        return Math.min(maximum, TOUCH_PROFILE_MINOR);
    }

    static void validateHostCapabilities(Message message, int expectedClock,
                                         int negotiatedMinor) throws IOException {
        require(message.header.type == CAPABILITIES && message.payload.length == 32,
                "CAPABILITIES required");
        ByteBuffer b = little(message.payload);
        int codecs = b.getInt();
        int maxAu = b.getInt();
        int width = b.getInt();
        int height = b.getInt();
        int features = b.getInt();
        int clock = b.getInt();
        long reserved = b.getLong();
        require(codecs == 1 && maxAu > 0 && maxAu <= MAX_AU, "codec/AU capabilities");
        require(width > 0 && width <= 4096 && height > 0 && height <= 2160,
                "geometry capabilities");
        if (negotiatedMinor >= TOUCH_PROFILE_MINOR) {
            require(features == (FEATURE_VIDEO | FEATURE_TOUCH)
                            && clock == expectedClock && reserved == TOUCH_DESCRIPTOR,
                    "touch capability profile");
        } else {
            require(features == FEATURE_VIDEO && clock == expectedClock && reserved == 0,
                    "legacy capability profile");
        }
    }

    static TouchConfiguration parseTouchConfiguration(Message message, int operation)
            throws IOException {
        require(message.header.type == TOUCH && message.payload.length == 32,
                "TOUCH configuration required");
        ByteBuffer b = little(message.payload);
        require(u16(b.getShort()) == 1 && u16(b.getShort()) == operation,
                "TOUCH profile/operation");
        TouchConfiguration c = new TouchConfiguration();
        c.coordinateMaximum = b.getInt();
        c.targetWidth = b.getInt();
        c.targetHeight = b.getInt();
        c.maxContacts = b.getInt();
        c.flags = b.getInt();
        c.targetToken = b.getLong();
        require(c.coordinateMaximum == TOUCH_COORDINATE_MAXIMUM
                        && c.targetWidth > 0 && c.targetWidth <= 4096
                        && c.targetHeight > 0 && c.targetHeight <= 2160
                        && c.maxContacts == TOUCH_MAX_CONTACTS
                        && c.flags == TOUCH_CONFIG_FLAGS && c.targetToken != 0,
                "TOUCH configuration");
        return c;
    }

    static byte[] touchConfiguration(TouchConfiguration c, int operation) throws IOException {
        require(operation == TOUCH_CONFIGURE || operation == TOUCH_READY,
                "TOUCH configuration operation");
        ByteBuffer b = little(32);
        b.putShort((short)1).putShort((short)operation)
                .putInt(c.coordinateMaximum).putInt(c.targetWidth).putInt(c.targetHeight)
                .putInt(c.maxContacts).putInt(c.flags).putLong(c.targetToken);
        return b.array();
    }

    static byte[] touchContact(int contact, int action, int x, int y, int pressure,
                               int flags, int activeMask, long eventTimeNs) throws IOException {
        require(contact >= 0 && contact < TOUCH_MAX_CONTACTS
                        && action >= 1 && action <= 4
                        && x >= 0 && x <= TOUCH_COORDINATE_MAXIMUM
                        && y >= 0 && y <= TOUCH_COORDINATE_MAXIMUM
                        && pressure >= 0 && pressure <= 1024
                        && (flags & ~0x0303) == 0
                        && activeMask >= 0 && activeMask < (1 << TOUCH_MAX_CONTACTS)
                        && eventTimeNs > 0, "TOUCH event");
        ByteBuffer b = little(32);
        b.putShort((short)1).putShort((short)TOUCH_CONTACT)
                .putShort((short)contact).putShort((short)action)
                .putInt(x).putInt(y).putShort((short)pressure).putShort((short)flags)
                .putInt(activeMask).putLong(eventTimeNs);
        return b.array();
    }

    static int hostHelloClock(Message message) {
        return little(message.payload).getInt(8);
    }

    static Frame parseFrame(Message message, boolean first, long lastId,
                            long lastSourceNs, long lastPts) throws IOException {
        require(message.header.type == FRAME && message.payload.length > FRAME_BYTES,
                "FRAME required");
        ByteBuffer b = little(message.payload);
        Frame f = new Frame();
        f.id = b.getLong();
        f.sourceNs = b.getLong();
        f.pts100ns = b.getLong();
        require(b.getInt() == 1, "frame codec");
        f.width = b.getInt();
        f.height = b.getInt();
        f.bytes = b.getInt();
        f.flags = b.getInt();
        f.crc = b.getInt();
        f.sourceTicks = b.getLong();
        f.sourceFrequency = b.getLong();
        require(f.id > 0 && f.sourceNs > 0 && f.sourceTicks > 0, "frame identity");
        require(f.width > 0 && f.width <= 4096 && (f.width & 1) == 0
                        && f.height > 0 && f.height <= 2160 && (f.height & 1) == 0,
                "frame geometry");
        require(f.bytes > 0 && f.bytes <= MAX_AU
                        && message.payload.length == FRAME_BYTES + f.bytes, "AU length");
        require((f.flags & ~15) == 0, "frame flags");
        require(f.sourceFrequency > 0 && f.sourceFrequency <= 1_000_000_000L,
                "source frequency");
        long calculatedNs = (f.sourceTicks / f.sourceFrequency) * 1_000_000_000L
                + (f.sourceTicks % f.sourceFrequency) * 1_000_000_000L / f.sourceFrequency;
        require(calculatedNs == f.sourceNs, "source clock association");
        if (!first) {
            require(f.id > lastId && f.sourceNs > lastSourceNs && f.pts100ns > lastPts,
                    "frame/source/PTS ordering");
        }
        f.accessUnit = Arrays.copyOfRange(message.payload, FRAME_BYTES, message.payload.length);
        int actualFlags = nalFlags(f.accessUnit);
        require((f.flags & 7) == actualFlags, "NAL/metadata association");
        require(!first || actualFlags == 7, "session needs SPS/PPS/IDR");
        CRC32 crc = new CRC32();
        crc.update(f.accessUnit);
        require((int)crc.getValue() == f.crc, "AU CRC");
        return f;
    }

    static int nalFlags(byte[] bytes) throws IOException {
        int flags = 0;
        int starts = 0;
        boolean vcl = false;
        for (int i = 0; i + 3 < bytes.length; i++) {
            int offset = 0;
            if (bytes[i] == 0 && bytes[i + 1] == 0 && bytes[i + 2] == 1) offset = 3;
            else if (i + 4 < bytes.length && bytes[i] == 0 && bytes[i + 1] == 0
                    && bytes[i + 2] == 0 && bytes[i + 3] == 1) offset = 4;
            if (offset != 0) {
                require(starts != 0 || i == 0, "Annex B prefix");
                starts++;
                int header = bytes[i + offset] & 0xff;
                require((header & 0x80) == 0, "NAL forbidden bit");
                int type = header & 31;
                require(type > 0 && type < 24, "NAL type");
                if (type == 1 || type == 5) vcl = true;
                if (type == 5) flags |= 1;
                if (type == 7) flags |= 2;
                if (type == 8) flags |= 4;
                i += offset - 1;
            }
        }
        require(starts > 0 && vcl, "missing VCL");
        return flags;
    }

    static int controlOperation(Message message) throws IOException {
        require(message.header.type == CONTROL && message.payload.length == 8, "CONTROL required");
        ByteBuffer b = little(message.payload);
        int operation = b.getInt();
        require((operation == 1 || operation == 2) && b.getInt() == 0, "control operation");
        return operation;
    }

    private static void validatePayloadSize(int type, int size) throws IOException {
        if (type == HELLO) require(size == 24, "hello size");
        else if (type == CAPABILITIES || type == TELEMETRY || type == TOUCH)
            require(size == 32, "fixed payload size");
        else if (type == FRAME) require(size > FRAME_BYTES && size <= MAX_PAYLOAD,
                    "frame payload size");
        else if (type == CONTROL || type == HEARTBEAT || type == CAMERA_CONTROL)
            require(size == 8, "control payload size");
        else throw new Violation("unknown message type");
    }

    private static void readFully(InputStream input, byte[] bytes, int offset, int count)
            throws IOException {
        while (count > 0) {
            int result = input.read(bytes, offset, count);
            if (result < 0) throw new EOFException("truncated byte stream");
            if (result == 0) continue;
            offset += result;
            count -= result;
        }
    }

    private static ByteBuffer little(int bytes) {
        return ByteBuffer.allocate(bytes).order(ByteOrder.LITTLE_ENDIAN);
    }

    private static ByteBuffer little(byte[] bytes) {
        return ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
    }

    private static int u16(short value) { return value & 0xffff; }

    static void require(boolean condition, String message) throws Violation {
        if (!condition) throw new Violation(message);
    }
}
