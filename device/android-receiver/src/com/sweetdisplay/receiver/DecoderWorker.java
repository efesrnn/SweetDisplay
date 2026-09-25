package com.sweetdisplay.receiver;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.SystemClock;
import android.util.Log;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

final class DecoderWorker {
    private static final String TAG = "SWDPRX";
    private static final String MIME = "video/avc";
    private static final int CAPACITY = 4;

    private static final class Item {
        final long session;
        final Protocol.Frame frame;
        final CountDownLatch drain;
        final CountDownLatch sessionReady;
        Item(long session, Protocol.Frame frame, CountDownLatch drain,
                CountDownLatch sessionReady) {
            this.session = session;
            this.frame = frame;
            this.drain = drain;
            this.sessionReady = sessionReady;
        }
    }

    final AtomicLong inputs = new AtomicLong();
    final AtomicLong outputs = new AtomicLong();
    final AtomicLong rendered = new AtomicLong();
    final AtomicLong resets = new AtomicLong();
    final AtomicLong errors = new AtomicLong();
    final AtomicLong queueAborted = new AtomicLong();
    final AtomicLong queuePeak = new AtomicLong();
    final AtomicLong firstRenderedNs = new AtomicLong();
    final AtomicLong lastRenderedNs = new AtomicLong();
    final AtomicReference<String> outputFormat = new AtomicReference<>("unknown");

    private final ArrayBlockingQueue<Item> queue = new ArrayBlockingQueue<>(CAPACITY);
    private final AtomicBoolean stopping = new AtomicBoolean(false);
    private final AtomicReference<Throwable> failure = new AtomicReference<>();
    private final Surface surface;
    private final Handler callbackHandler;
    private final Thread thread;
    private volatile CountDownLatch sessionReady = new CountDownLatch(0);
    private MediaCodec codec;
    private long codecSession;

    DecoderWorker(Surface surface, Handler callbackHandler) {
        this.surface = surface;
        this.callbackHandler = callbackHandler;
        thread = new Thread(this::run, "swdp-decoder");
        thread.start();
    }

    void beginSession(long session) throws Exception {
        checkFailure();
        int removed = queue.size();
        queue.clear();
        if (removed != 0) queueAborted.addAndGet(removed);
        sessionReady = new CountDownLatch(1);
        Log.i(TAG, "DECODER_SESSION_BEGIN session=" + Long.toUnsignedString(session)
                + " queueCleared=" + removed);
    }

    boolean submit(long session, Protocol.Frame frame) throws Exception {
        checkFailure();
        CountDownLatch ready = sessionReady;
        boolean accepted = queue.offer(new Item(session, frame, null, ready),
                100, TimeUnit.MILLISECONDS);
        updatePeak();
        if (accepted && ready.getCount() != 0) {
            Protocol.require(ready.await(2500, TimeUnit.MILLISECONDS),
                    "decoder session-ready deadline");
            checkFailure();
        }
        return accepted;
    }

    boolean drain(long session) throws Exception {
        checkFailure();
        CountDownLatch done = new CountDownLatch(1);
        if (!queue.offer(new Item(session, null, done, null), 100, TimeUnit.MILLISECONDS)) return false;
        updatePeak();
        boolean result = done.await(2500, TimeUnit.MILLISECONDS);
        checkFailure();
        return result;
    }

    int queueDepth() { return queue.size(); }

    void discardQueued() {
        int discarded = queue.size();
        queue.clear();
        if (discarded > 0) queueAborted.addAndGet(discarded);
    }

    void stop() {
        stopping.set(true);
        thread.interrupt();
        try { thread.join(2500); } catch (InterruptedException ignored) { Thread.currentThread().interrupt(); }
    }

    private void run() {
        try {
            while (!stopping.get()) {
                Item item = queue.poll(50, TimeUnit.MILLISECONDS);
                if (item == null) {
                    if (codec != null) drainAvailable(false, 0);
                    continue;
                }
                if (item.frame == null) {
                    drainCodec(item.session);
                    item.drain.countDown();
                    continue;
                }
                if (codec == null || codecSession != item.session) {
                    releaseCodec();
                    configure(item.session, item.frame);
                }
                try {
                    submitFrame(item.frame);
                } catch (Throwable error) {
                    failure.compareAndSet(null, error);
                    throw error;
                } finally {
                    if (item.sessionReady != null) item.sessionReady.countDown();
                }
            }
        } catch (InterruptedException ignored) {
            Thread.currentThread().interrupt();
        } catch (Throwable error) {
            errors.incrementAndGet();
            failure.compareAndSet(null, error);
            Log.e(TAG, "DECODER_FATAL", error);
        } finally {
            releaseCodec();
            for (Item item : queue) {
                if (item.drain != null) item.drain.countDown();
                if (item.sessionReady != null) item.sessionReady.countDown();
            }
            queue.clear();
        }
    }

    private void configure(long session, Protocol.Frame frame) throws Exception {
        MediaFormat format = MediaFormat.createVideoFormat(MIME, frame.width, frame.height);
        format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, Protocol.MAX_AU);
        format.setFloat(MediaFormat.KEY_FRAME_RATE, 60.0f);
        String decoder = chooseDecoder(frame.width, frame.height);
        Protocol.require(decoder != null, "no hardware AVC decoder");
        codec = MediaCodec.createByCodecName(decoder);
        MediaCodecInfo info = codec.getCodecInfo();
        Protocol.require(info.isHardwareAccelerated() && !info.isSoftwareOnly(),
                "selected decoder is not hardware backed");
        codec.setOnFrameRenderedListener((ignored, ptsUs, nanoTime) -> {
            rendered.incrementAndGet();
            firstRenderedNs.compareAndSet(0, nanoTime);
            lastRenderedNs.set(nanoTime);
        }, callbackHandler);
        codec.configure(format, surface, null, 0);
        codec.start();
        codec.setVideoScalingMode(MediaCodec.VIDEO_SCALING_MODE_SCALE_TO_FIT);
        codecSession = session;
        resets.incrementAndGet();
        Log.i(TAG, "DECODER_CONFIG session=" + Long.toUnsignedString(session)
                + " name=" + decoder + " hardware=" + info.isHardwareAccelerated()
                + " width=" + frame.width + " height=" + frame.height
                + " scaling=FIT");
    }

    private String chooseDecoder(int width, int height) {
        MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);
        for (MediaCodecInfo info : list.getCodecInfos()) {
            if (info.isEncoder() || !info.isHardwareAccelerated() || info.isSoftwareOnly()
                    || info.getName().toLowerCase(Locale.ROOT).contains("secure")) continue;
            try {
                if (info.getCapabilitiesForType(MIME).getVideoCapabilities()
                        .isSizeSupported(width, height)) return info.getName();
            } catch (IllegalArgumentException ignored) { }
        }
        return null;
    }

    private void submitFrame(Protocol.Frame frame) throws Exception {
        long deadline = SystemClock.elapsedRealtime() + 250;
        for (;;) {
            int index = codec.dequeueInputBuffer(10_000);
            if (index >= 0) {
                ByteBuffer input = codec.getInputBuffer(index);
                Protocol.require(input != null && input.capacity() >= frame.bytes,
                        "decoder input capacity");
                input.clear();
                input.put(frame.accessUnit);
                codec.queueInputBuffer(index, 0, frame.bytes, frame.pts100ns / 10, 0);
                inputs.incrementAndGet();
                drainAvailable(false, 0);
                return;
            }
            drainAvailable(false, 0);
            Protocol.require(SystemClock.elapsedRealtime() < deadline, "decoder input deadline");
        }
    }

    private void drainCodec(long session) throws Exception {
        if (codec == null || codecSession != session) return;
        long deadline = SystemClock.elapsedRealtime() + 2000;
        boolean eosQueued = false;
        boolean eosOutput = false;
        while (!eosOutput && SystemClock.elapsedRealtime() < deadline) {
            if (!eosQueued) {
                int input = codec.dequeueInputBuffer(10_000);
                if (input >= 0) {
                    codec.queueInputBuffer(input, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                    eosQueued = true;
                }
            }
            eosOutput = drainAvailable(true, deadline);
        }
        Protocol.require(eosQueued && eosOutput, "decoder drain deadline");
        Log.i(TAG, "DECODER_DRAIN session=" + Long.toUnsignedString(session)
                + " inputs=" + inputs.get() + " outputs=" + outputs.get()
                + " rendered=" + rendered.get());
    }

    private boolean drainAvailable(boolean wait, long deadline) throws Exception {
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        for (;;) {
            long timeoutUs = wait ? 10_000 : 0;
            int output = codec.dequeueOutputBuffer(info, timeoutUs);
            if (output >= 0) {
                boolean hasImage = info.size > 0;
                codec.releaseOutputBuffer(output, hasImage);
                if (hasImage) outputs.incrementAndGet();
                if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) return true;
                continue;
            }
            if (output == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                String value = codec.getOutputFormat().toString();
                outputFormat.set(value);
                Log.i(TAG, "DECODER_FORMAT " + value);
                continue;
            }
            if (!wait || SystemClock.elapsedRealtime() >= deadline) return false;
        }
    }

    private void releaseCodec() {
        if (codec == null) return;
        try { codec.stop(); } catch (RuntimeException ignored) { }
        codec.release();
        codec = null;
        codecSession = 0;
    }

    private void updatePeak() {
        long depth = queue.size();
        for (;;) {
            long old = queuePeak.get();
            if (depth <= old || queuePeak.compareAndSet(old, depth)) return;
        }
    }

    private void checkFailure() throws Exception {
        Throwable error = failure.get();
        if (error == null) return;
        if (error instanceof Exception) throw (Exception)error;
        throw new Exception(error);
    }
}
