package com.sweetdisplay.receiver;

import android.os.Debug;
import android.os.Handler;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;

import java.io.EOFException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

final class Receiver {
    private static final String TAG = "SWDPRX";
    static final int PORT = 48231;

    final AtomicLong connections = new AtomicLong();
    final AtomicLong cleanDrains = new AtomicLong();
    final AtomicLong disconnects = new AtomicLong();
    final AtomicLong protocolErrors = new AtomicLong();
    final AtomicLong crcErrors = new AtomicLong();
    final AtomicLong sequenceErrors = new AtomicLong();
    final AtomicLong sessionErrors = new AtomicLong();
    final AtomicLong frames = new AtomicLong();
    final AtomicLong frameBytes = new AtomicLong();
    final AtomicLong wireBytes = new AtomicLong();
    final AtomicLong queueOverflows = new AtomicLong();

    private final AtomicBoolean stopping = new AtomicBoolean(false);
    private final DecoderWorker decoder;
    private final boolean ackOnly;
    private final String bindAddress;
    private final TouchCapture touch = new TouchCapture();
    private final Thread listenerThread;
    private volatile Socket activeSocket;
    private volatile ServerSocket server;
    private volatile String lastError = "none";

    Receiver(Surface surface, Handler callbackHandler, boolean diagnosticAckOnly,
             String diagnosticBindAddress) {
        ackOnly = diagnosticAckOnly;
        bindAddress = diagnosticBindAddress;
        decoder = ackOnly ? null : new DecoderWorker(surface, callbackHandler);
        listenerThread = new Thread(this::listen, "swdp-listener");
        listenerThread.start();
    }

    void stop() {
        stopping.set(true);
        close(activeSocket);
        close(server);
        try { listenerThread.join(2500); } catch (InterruptedException ignored) {
            Thread.currentThread().interrupt();
        }
        if (decoder != null) decoder.stop();
        logSnapshot("STOPPED");
    }

    String statusLine() {
        return "mode=" + (ackOnly ? "ACK_ONLY" : "NORMAL")
                + " connections=" + connections.get() + " frames=" + frames.get()
                + " decode=" + decoderValue(0) + "/" + decoderValue(1)
                + " rendered=" + decoderValue(2) + " queue=" + decoderValue(3)
                + " touch=" + touch.sent.get() + "/" + touch.queued()
                + " errors=" + protocolErrors.get() + "/" + decoderValue(4);
    }

    void updateSurface(int width, int height, int rotation) {
        touch.updateSurface(width, height, rotation);
    }

    boolean captureTouch(android.view.MotionEvent event) { return touch.capture(event); }

    void logSnapshot(String event) {
        logSnapshot(event, -1, -1, -1, -1.0f);
    }

    void logSnapshot(String event, int thermalStatus, int batteryTemperatureDeciC,
                     int batteryStatus, float refreshRate) {
        Debug.MemoryInfo memory = new Debug.MemoryInfo();
        Debug.getMemoryInfo(memory);
        Log.i(TAG, "METRICS event=" + event
                + " elapsedRealtimeNs=" + SystemClock.elapsedRealtimeNanos()
                + " processCpuMs=" + android.os.Process.getElapsedCpuTime()
                + " connections=" + connections.get()
                + " cleanDrains=" + cleanDrains.get()
                + " disconnects=" + disconnects.get()
                + " frames=" + frames.get()
                + " frameBytes=" + frameBytes.get()
                + " wireBytes=" + wireBytes.get()
                + " protocolErrors=" + protocolErrors.get()
                + " crcErrors=" + crcErrors.get()
                + " sequenceErrors=" + sequenceErrors.get()
                + " sessionErrors=" + sessionErrors.get()
                 + " queueOverflows=" + queueOverflows.get()
                + " ackOnly=" + ackOnly
                + " queueDepth=" + decoderValue(3)
                + " queuePeak=" + decoderValue(5)
                + " queueAborted=" + decoderValue(6)
                + " decoderInputs=" + decoderValue(0)
                + " decoderOutputs=" + decoderValue(1)
                + " renderCallbacks=" + decoderValue(2)
                + " decoderResets=" + decoderValue(7)
                + " decoderErrors=" + decoderValue(4)
                + " touchAccepted=" + touch.accepted.get()
                + " touchSent=" + touch.sent.get()
                + " touchQueued=" + touch.queued()
                + " touchActiveMask=" + touch.activeMask()
                + " touchOutside=" + touch.rejectedOutside.get()
                + " touchMovesCoalesced=" + touch.coalescedMoves.get()
                + " touchQueueOverflows=" + touch.queueOverflows.get()
                + " pssKb=" + memory.getTotalPss()
                + " privateDirtyKb=" + memory.getTotalPrivateDirty()
                + " thermalStatus=" + thermalStatus
                + " batteryTemperatureDeciC=" + batteryTemperatureDeciC
                + " batteryStatus=" + batteryStatus
                + " refreshHz=" + refreshRate
                + " lastError=" + lastError.replace(' ', '_'));
    }

    private void listen() {
        try {
            server = new ServerSocket();
            server.setReuseAddress(true);
            server.bind(new InetSocketAddress(InetAddress.getByName(bindAddress), PORT), 1);
            server.setSoTimeout(500);
            Log.i(TAG, "LISTENING address=" + bindAddress + " port=" + PORT
                    + " queueCapacity=4 mode="
                    + (ackOnly ? "ACK_ONLY" : "NORMAL"));
            while (!stopping.get()) {
                try {
                    Socket socket = server.accept();
                    activeSocket = socket;
                    socket.setTcpNoDelay(true);
                    socket.setReceiveBufferSize(64 * 1024);
                    socket.setSendBufferSize(64 * 1024);
                    socket.setSoTimeout(2500);
                    connections.incrementAndGet();
                    Log.i(TAG, "SESSION_ACCEPT local="
                            + socket.getLocalAddress().getHostAddress() + ":"
                            + socket.getLocalPort() + " peer="
                            + socket.getInetAddress().getHostAddress() + ":"
                            + socket.getPort());
                    handle(socket);
                } catch (SocketTimeoutException ignored) {
                    // Periodically observe stop without a permanent blocking accept.
                } catch (EOFException | SocketException disconnected) {
                    if (!stopping.get()) {
                        disconnects.incrementAndGet();
                        lastError = disconnected.getClass().getSimpleName();
                        Log.i(TAG, "SESSION_DISCONNECTED type=" + lastError);
                    }
                } catch (Protocol.Violation violation) {
                    protocolErrors.incrementAndGet();
                    classifyViolation(violation.getMessage());
                    lastError = violation.getMessage();
                    Log.e(TAG, "PROTOCOL_ERROR reason=" + violation.getMessage());
                } catch (Exception error) {
                    if (!stopping.get()) {
                        protocolErrors.incrementAndGet();
                        lastError = error.getClass().getSimpleName() + ":" + error.getMessage();
                        Log.e(TAG, "SESSION_ERROR", error);
                    }
                } finally {
                    touch.end();
                    close(activeSocket);
                    activeSocket = null;
                    if (decoder != null) decoder.discardQueued();
                }
            }
        } catch (Exception error) {
            if (!stopping.get()) {
                protocolErrors.incrementAndGet();
                lastError = error.getClass().getSimpleName() + ":" + error.getMessage();
                Log.e(TAG, "LISTENER_FATAL", error);
            }
        } finally {
            close(server);
            server = null;
        }
    }

    private void handle(Socket socket) throws Exception {
        InputStream input = socket.getInputStream();
        OutputStream output = socket.getOutputStream();
        long expectedRx = 1;
        long tx = 0;
        long lastTimestamp = 0;
        long session;

        Protocol.Message hello = Protocol.read(input);
        wireBytes.addAndGet(Protocol.HEADER_BYTES + hello.payload.length);
        requireSequence(hello, expectedRx++);
        int negotiatedMinor = Protocol.validateHostHello(hello);
        session = hello.header.session;
        int hostClock = Protocol.hostHelloClock(hello);
        lastTimestamp = hello.header.timestampNs;

        Protocol.write(output, Protocol.HELLO, session, ++tx, System.nanoTime(),
                Protocol.helloDevice(negotiatedMinor));
        Protocol.write(output, Protocol.CAPABILITIES, session, ++tx, System.nanoTime(),
                Protocol.capabilitiesDevice(negotiatedMinor));

        Protocol.Message capabilities = Protocol.read(input);
        wireBytes.addAndGet(Protocol.HEADER_BYTES + capabilities.payload.length);
        requireSession(capabilities, session);
        requireSequence(capabilities, expectedRx++);
        requireTimestamp(capabilities, lastTimestamp);
        Protocol.validateHostCapabilities(capabilities, hostClock, negotiatedMinor);
        lastTimestamp = capabilities.header.timestampNs;
        if (negotiatedMinor >= Protocol.TOUCH_PROFILE_MINOR) {
            Protocol.Message configure = Protocol.read(input);
            wireBytes.addAndGet(Protocol.HEADER_BYTES + configure.payload.length);
            requireSession(configure, session);
            requireSequence(configure, expectedRx++);
            requireTimestamp(configure, lastTimestamp);
            Protocol.TouchConfiguration touchConfiguration = Protocol.parseTouchConfiguration(
                    configure, Protocol.TOUCH_CONFIGURE);
            lastTimestamp = configure.header.timestampNs;
            touch.begin(touchConfiguration);
            Protocol.write(output, Protocol.TOUCH, session, ++tx, System.nanoTime(),
                    Protocol.touchConfiguration(touchConfiguration, Protocol.TOUCH_READY));
        }
        if (decoder != null) decoder.beginSession(session);
        Log.i(TAG, "SESSION_READY ordinal=" + connections.get()
                + " hostClock=" + hostClock + " deviceClock=0"
                 + " minor=" + negotiatedMinor + " touch="
                + (negotiatedMinor >= Protocol.TOUCH_PROFILE_MINOR)
                + " mode=" + (ackOnly ? "ACK_ONLY" : "NORMAL"));

        boolean first = true;
        long localFrames = 0;
        long localBytes = 0;
        long lastId = 0;
        long lastSourceNs = 0;
        long lastPts = 0;
        for (;;) {
            Protocol.Message message = Protocol.read(input);
            wireBytes.addAndGet(Protocol.HEADER_BYTES + message.payload.length);
            requireSession(message, session);
            requireSequence(message, expectedRx++);
            requireTimestamp(message, lastTimestamp);
            lastTimestamp = message.header.timestampNs;

            if (message.header.type == Protocol.FRAME) {
                Protocol.Frame frame = Protocol.parseFrame(
                        message, first, lastId, lastSourceNs, lastPts);
                if (decoder != null && !decoder.submit(session, frame)) {
                    queueOverflows.incrementAndGet();
                    throw new Protocol.Violation("decoder queue overflow");
                }
                first = false;
                lastId = frame.id;
                lastSourceNs = frame.sourceNs;
                lastPts = frame.pts100ns;
                localFrames++;
                localBytes += frame.bytes;
                frames.incrementAndGet();
                frameBytes.addAndGet(frame.bytes);
                Protocol.write(output, Protocol.TELEMETRY, session, ++tx, System.nanoTime(),
                        Protocol.telemetry(localFrames, message.header.sequence, frame.id, localBytes));
                tx = drainTouch(output, session, tx);
            } else if (message.header.type == Protocol.HEARTBEAT) {
                Protocol.write(output, Protocol.HEARTBEAT, session, ++tx, System.nanoTime(),
                        message.payload);
                tx = drainTouch(output, session, tx);
            } else if (message.header.type == Protocol.CONTROL) {
                Protocol.require(Protocol.controlOperation(message) == 1, "DRAIN request");
                Protocol.require(decoder == null || decoder.drain(session), "decoder drain failed");
                Protocol.write(output, Protocol.CONTROL, session, ++tx, System.nanoTime(),
                        Protocol.drainAck());
                cleanDrains.incrementAndGet();
                Log.i(TAG, "SESSION_DRAIN_ACK frames=" + localFrames + " bytes=" + localBytes);
                logSnapshot("DRAIN");
                return;
            } else {
                throw new Protocol.Violation("unnegotiated inbound message");
            }
        }
    }

    private long drainTouch(OutputStream output, long session, long tx) throws Exception {
        for (TouchCapture.Item item : touch.drain(16)) {
            Protocol.write(output, Protocol.TOUCH, session, ++tx, System.nanoTime(), item.payload);
        }
        return tx;
    }

    private long decoderValue(int value) {
        if (decoder == null) return 0;
        switch (value) {
            case 0: return decoder.inputs.get();
            case 1: return decoder.outputs.get();
            case 2: return decoder.rendered.get();
            case 3: return decoder.queueDepth();
            case 4: return decoder.errors.get();
            case 5: return decoder.queuePeak.get();
            case 6: return decoder.queueAborted.get();
            case 7: return decoder.resets.get();
            default: throw new IllegalArgumentException("decoder counter");
        }
    }

    private void requireSequence(Protocol.Message message, long expected) throws Exception {
        if (message.header.sequence != expected) {
            throw new Protocol.Violation("sequence integrity");
        }
    }

    private void requireSession(Protocol.Message message, long session) throws Exception {
        if (message.header.session != session) {
            throw new Protocol.Violation("session mismatch");
        }
    }

    private static void requireTimestamp(Protocol.Message message, long last) throws Exception {
        Protocol.require(message.header.timestampNs >= last, "timestamp regression");
    }

    private void classifyViolation(String reason) {
        if (reason.contains("sequence")) sequenceErrors.incrementAndGet();
        if (reason.contains("session")) sessionErrors.incrementAndGet();
        if (reason.contains("CRC")) crcErrors.incrementAndGet();
    }

    private static void close(Socket socket) {
        if (socket == null) return;
        try { socket.close(); } catch (Exception ignored) { }
    }

    private static void close(ServerSocket socket) {
        if (socket == null) return;
        try { socket.close(); } catch (Exception ignored) { }
    }
}
