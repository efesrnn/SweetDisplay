package com.sweetdisplay.userspaceprobe;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Color;
import android.hardware.display.DisplayManager;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public final class MainActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = "SWDP2B";
    private static final String MIME = "video/avc";
    private int videoWidth = 1280;
    private int videoHeight = 576;
    private float videoFrameRate = 30.0f;

    private final AtomicBoolean decoding = new AtomicBoolean(false);
    private final AtomicBoolean stop = new AtomicBoolean(false);
    private final AtomicInteger rendered = new AtomicInteger(0);
    private final List<Long> renderTimesNs = new ArrayList<>();
    private final Handler main = new Handler(Looper.getMainLooper());
    private SurfaceView surfaceView;
    private TextView status;
    private ProbeLayout root;
    private int touchDown;
    private int touchMove;
    private int touchUp;
    private float touchMinX = Float.POSITIVE_INFINITY;
    private float touchMaxX = Float.NEGATIVE_INFINITY;
    private float touchMinY = Float.POSITIVE_INFINITY;
    private float touchMaxY = Float.NEGATIVE_INFINITY;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().setStatusBarColor(Color.TRANSPARENT);
        getWindow().setNavigationBarColor(Color.BLACK);
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);

        root = new ProbeLayout(this);
        root.setBackgroundColor(Color.BLACK);
        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        root.addView(surfaceView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT));

        status = new TextView(this);
        status.setTextColor(Color.WHITE);
        status.setBackgroundColor(0xB0000000);
        status.setTextSize(14.0f);
        status.setPadding(24, 18, 24, 18);
        status.setText("SweetDisplay Phase 2B\nSurface bekleniyor...");
        FrameLayout.LayoutParams textParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.TOP);
        root.addView(status, textParams);
        setContentView(root);

        readFixtureConfiguration();
        logDisplay("CREATE");
        enumerateAvcDecoders();
    }

    @Override
    protected void onDestroy() {
        stop.set(true);
        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration configuration) {
        super.onConfigurationChanged(configuration);
        logDisplay("CONFIG_CHANGED");
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(TAG, "SURFACE_CREATED width=" + surfaceView.getWidth()
                + " height=" + surfaceView.getHeight());
        if (decoding.compareAndSet(false, true)) {
            new Thread(() -> decode(holder.getSurface()), "swdp2b-decoder").start();
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i(TAG, "SURFACE_CHANGED width=" + width + " height=" + height
                + " format=" + format + " rotation=" + currentRotation());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "SURFACE_DESTROYED");
        stop.set(true);
    }

    private void enumerateAvcDecoders() {
        MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);
        int count = 0;
        for (MediaCodecInfo info : list.getCodecInfos()) {
            if (info.isEncoder()) continue;
            boolean hasAvc = false;
            for (String type : info.getSupportedTypes()) {
                if (MIME.equalsIgnoreCase(type)) hasAvc = true;
            }
            if (!hasAvc) continue;
            count++;
            try {
                MediaCodecInfo.CodecCapabilities caps = info.getCapabilitiesForType(MIME);
                MediaCodecInfo.VideoCapabilities video = caps.getVideoCapabilities();
                StringBuilder profiles = new StringBuilder();
                for (MediaCodecInfo.CodecProfileLevel level : caps.profileLevels) {
                    if (profiles.length() != 0) profiles.append(',');
                    profiles.append(level.profile).append(':').append(level.level);
                }
                boolean size = video.isSizeSupported(videoWidth, videoHeight);
                boolean rate = video.areSizeAndRateSupported(
                        videoWidth, videoHeight, videoFrameRate);
                StringBuilder colors = new StringBuilder();
                for (int color : caps.colorFormats) {
                    if (colors.length() != 0) colors.append(',');
                    colors.append(color);
                }
                Log.i(TAG, "CODEC name=" + info.getName()
                        + " canonical=" + info.getCanonicalName()
                        + " hardware=" + info.isHardwareAccelerated()
                        + " software=" + info.isSoftwareOnly()
                        + " vendor=" + info.isVendor()
                        + " alias=" + info.isAlias()
                        + " testSize=" + videoWidth + "x" + videoHeight
                        + " sizeSupported=" + size
                        + " testRate=" + format(videoFrameRate)
                        + " rateSupported=" + rate
                        + " maxInstances=" + caps.getMaxSupportedInstances()
                        + " adaptive=" + caps.isFeatureSupported(
                                MediaCodecInfo.CodecCapabilities.FEATURE_AdaptivePlayback)
                        + " secure=" + caps.isFeatureSupported(
                                MediaCodecInfo.CodecCapabilities.FEATURE_SecurePlayback)
                        + " tunneled=" + caps.isFeatureSupported(
                                MediaCodecInfo.CodecCapabilities.FEATURE_TunneledPlayback)
                        + " lowLatency=" + caps.isFeatureSupported(
                                MediaCodecInfo.CodecCapabilities.FEATURE_LowLatency)
                        + " colors=" + colors
                        + " profiles=" + profiles);
            } catch (RuntimeException error) {
                Log.e(TAG, "CODEC_QUERY_ERROR name=" + info.getName(), error);
            }
        }
        Log.i(TAG, "CODEC_ENUM_DONE avcDecoders=" + count);
    }

    private String chooseHardwareDecoder(MediaFormat format) {
        MediaCodecList list = new MediaCodecList(MediaCodecList.ALL_CODECS);
        for (MediaCodecInfo info : list.getCodecInfos()) {
            if (info.isEncoder() || !info.isHardwareAccelerated()
                    || info.isSoftwareOnly() || info.getName().toLowerCase(Locale.ROOT).contains("secure")) {
                continue;
            }
            try {
                MediaCodecInfo.CodecCapabilities caps = info.getCapabilitiesForType(MIME);
                if (caps.getVideoCapabilities().areSizeAndRateSupported(
                        videoWidth, videoHeight, videoFrameRate)) {
                    return info.getName();
                }
            } catch (IllegalArgumentException ignored) {
                // Not an AVC decoder.
            }
        }
        return list.findDecoderForFormat(format);
    }

    private void decode(Surface surface) {
        MediaCodec codec = null;
        int queued = 0;
        int output = 0;
        boolean eosQueued = false;
        boolean eosOutput = false;
        String decoderName = "none";
        long startedNs = System.nanoTime();
        try {
            byte[] stream = readAsset("test_stream.h264");
            List<AccessUnit> units = readIndex();
            int expectedBytes = 0;
            for (AccessUnit unit : units) expectedBytes += unit.bytes;
            if (stream.length != expectedBytes || units.isEmpty()) {
                throw new IllegalStateException("fixture length mismatch stream="
                        + stream.length + " index=" + expectedBytes);
            }

            MediaFormat format = MediaFormat.createVideoFormat(MIME, videoWidth, videoHeight);
            format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 256 * 1024);
            format.setFloat(MediaFormat.KEY_FRAME_RATE, videoFrameRate);
            decoderName = chooseHardwareDecoder(format);
            if (decoderName == null) throw new IllegalStateException("no AVC decoder");
            codec = MediaCodec.createByCodecName(decoderName);
            final MediaCodec callbackCodec = codec;
            codec.setOnFrameRenderedListener((ignored, presentationTimeUs, nanoTime) -> {
                rendered.incrementAndGet();
                synchronized (renderTimesNs) {
                    renderTimesNs.add(nanoTime);
                }
                if (rendered.get() == 1) {
                    Log.i(TAG, "FIRST_FRAME_RENDERED ptsUs=" + presentationTimeUs
                            + " callbackNs=" + nanoTime);
                }
            }, main);
            codec.configure(format, surface, null, 0);
            codec.start();
            MediaCodecInfo selectedInfo = codec.getCodecInfo();
            Log.i(TAG, "DECODE_START decoder=" + decoderName
                    + " canonical=" + selectedInfo.getCanonicalName()
                    + " hardware=" + selectedInfo.isHardwareAccelerated()
                    + " software=" + selectedInfo.isSoftwareOnly()
                    + " vendor=" + selectedInfo.isVendor()
                    + " accessUnits=" + units.size()
                    + " bytes=" + stream.length
                    + " testSize=" + videoWidth + "x" + videoHeight
                    + " testRate=" + format(videoFrameRate));
            setStatus("Donanım AVC decode başladı\n" + decoderName
                    + "\nEkrana dokunup sürükle; sonuçlar loglanıyor.");

            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
            int byteOffset = 0;
            long renderBaseNs = System.nanoTime() + 250_000_000L;
            long deadline = SystemClock.elapsedRealtime() + 20_000L;
            while (!eosOutput && !stop.get() && SystemClock.elapsedRealtime() < deadline) {
                if (!eosQueued) {
                    int inputIndex = codec.dequeueInputBuffer(10_000L);
                    if (inputIndex >= 0) {
                        ByteBuffer buffer = codec.getInputBuffer(inputIndex);
                        if (buffer == null) throw new IllegalStateException("null input buffer");
                        buffer.clear();
                        if (queued < units.size()) {
                            AccessUnit unit = units.get(queued);
                            buffer.put(stream, byteOffset, unit.bytes);
                            codec.queueInputBuffer(inputIndex, 0, unit.bytes, unit.ptsUs, 0);
                            byteOffset += unit.bytes;
                            queued++;
                        } else {
                            long lastPts = units.get(units.size() - 1).ptsUs;
                            codec.queueInputBuffer(inputIndex, 0, 0, lastPts + 33_333L,
                                    MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                            eosQueued = true;
                        }
                    }
                }

                int outputIndex = codec.dequeueOutputBuffer(info, 10_000L);
                if (outputIndex >= 0) {
                    if (info.size > 0) {
                        long targetNs = renderBaseNs + info.presentationTimeUs * 1_000L;
                        codec.releaseOutputBuffer(outputIndex, targetNs);
                        output++;
                    } else {
                        codec.releaseOutputBuffer(outputIndex, false);
                    }
                    if ((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) eosOutput = true;
                } else if (outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                    Log.i(TAG, "OUTPUT_FORMAT " + codec.getOutputFormat());
                }
            }
            SystemClock.sleep(1200L);
            RenderStats stats = renderStats();
            long elapsedMs = (System.nanoTime() - startedNs) / 1_000_000L;
            Log.i(TAG, "DECODE_RESULT outcome=" + (eosOutput ? "PASS" : "INCOMPLETE")
                    + " decoder=" + decoderName
                    + " queued=" + queued
                    + " output=" + output
                    + " renderedCallbacks=" + rendered.get()
                    + " elapsedMs=" + elapsedMs
                    + " cadenceCount=" + stats.intervals
                    + " cadenceAvgMs=" + format(stats.averageMs)
                    + " cadenceMinMs=" + format(stats.minimumMs)
                    + " cadenceMaxMs=" + format(stats.maximumMs)
                    + " surface=" + surfaceView.getWidth() + "x" + surfaceView.getHeight()
                    + " rotation=" + currentRotation());
            final int finalOutput = output;
            final boolean finalEos = eosOutput;
            setStatus((finalEos ? "DECODE PASS" : "DECODE TAMAMLANMADI")
                    + "\nDecoder: " + decoderName
                    + "\nOutput: " + finalOutput + ", rendered callback: " + rendered.get()
                    + "\nTouch: DOWN " + touchDown + " / MOVE " + touchMove + " / UP " + touchUp
                    + "\nLütfen ekrana dokunup sürükle.");
        } catch (Throwable error) {
            Log.e(TAG, "DECODE_RESULT outcome=FAIL decoder=" + decoderName
                    + " queued=" + queued + " output=" + output, error);
            setStatus("DECODE FAIL\n" + error.getClass().getSimpleName()
                    + ": " + error.getMessage());
        } finally {
            if (codec != null) {
                try { codec.stop(); } catch (RuntimeException ignored) { }
                codec.release();
            }
        }
    }

    private byte[] readAsset(String name) throws Exception {
        try (InputStream input = getAssets().open(name);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[64 * 1024];
            int count;
            while ((count = input.read(buffer)) >= 0) output.write(buffer, 0, count);
            return output.toByteArray();
        }
    }

    private List<AccessUnit> readIndex() throws Exception {
        List<AccessUnit> result = new ArrayList<>();
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                getAssets().open("test_stream.index"), StandardCharsets.US_ASCII))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isEmpty() || line.startsWith("#")) continue;
                String[] fields = line.split(",");
                result.add(new AccessUnit(Integer.parseInt(fields[0]), Long.parseLong(fields[1])));
            }
        }
        return result;
    }

    private void readFixtureConfiguration() {
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                getAssets().open("test_stream.config"), StandardCharsets.US_ASCII))) {
            String line = reader.readLine();
            String[] fields = line.split(",");
            videoWidth = Integer.parseInt(fields[0]);
            videoHeight = Integer.parseInt(fields[1]);
            videoFrameRate = Float.parseFloat(fields[2]);
            Log.i(TAG, "FIXTURE_CONFIG width=" + videoWidth + " height=" + videoHeight
                    + " frameRate=" + format(videoFrameRate));
        } catch (Exception error) {
            throw new IllegalStateException("invalid fixture configuration", error);
        }
    }

    private void logDisplay(String event) {
        DisplayManager manager = (DisplayManager) getSystemService(DISPLAY_SERVICE);
        Display display = manager.getDisplay(Display.DEFAULT_DISPLAY);
        Display.Mode mode = display.getMode();
        Log.i(TAG, "DISPLAY event=" + event
                + " mode=" + mode.getPhysicalWidth() + "x" + mode.getPhysicalHeight()
                + " refresh=" + format(mode.getRefreshRate())
                + " rotation=" + display.getRotation());
    }

    private int currentRotation() {
        DisplayManager manager = (DisplayManager) getSystemService(DISPLAY_SERVICE);
        return manager.getDisplay(Display.DEFAULT_DISPLAY).getRotation();
    }

    private void recordTouch(MotionEvent event) {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) touchDown++;
        if (action == MotionEvent.ACTION_MOVE) touchMove++;
        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) touchUp++;
        float x = event.getX();
        float y = event.getY();
        touchMinX = Math.min(touchMinX, x);
        touchMaxX = Math.max(touchMaxX, x);
        touchMinY = Math.min(touchMinY, y);
        touchMaxY = Math.max(touchMaxY, y);
        InputDevice device = event.getDevice();
        float rangeXMin = Float.NaN;
        float rangeXMax = Float.NaN;
        float rangeYMin = Float.NaN;
        float rangeYMax = Float.NaN;
        if (device != null) {
            InputDevice.MotionRange rangeX = device.getMotionRange(MotionEvent.AXIS_X,
                    event.getSource());
            InputDevice.MotionRange rangeY = device.getMotionRange(MotionEvent.AXIS_Y,
                    event.getSource());
            if (rangeX != null) { rangeXMin = rangeX.getMin(); rangeXMax = rangeX.getMax(); }
            if (rangeY != null) { rangeYMin = rangeY.getMin(); rangeYMax = rangeY.getMax(); }
        }
        Log.i(TAG, "TOUCH action=" + MotionEvent.actionToString(action)
                + " pointers=" + event.getPointerCount()
                + " pointerId=" + event.getPointerId(0)
                + " x=" + format(x) + " y=" + format(y)
                + " pressure=" + format(event.getPressure())
                + " eventTimeMs=" + event.getEventTime()
                + " rangeX=" + format(rangeXMin) + ".." + format(rangeXMax)
                + " rangeY=" + format(rangeYMin) + ".." + format(rangeYMax)
                + " aggregateX=" + format(touchMinX) + ".." + format(touchMaxX)
                + " aggregateY=" + format(touchMinY) + ".." + format(touchMaxY));
    }

    private RenderStats renderStats() {
        synchronized (renderTimesNs) {
            if (renderTimesNs.size() < 2) return new RenderStats(0, 0, 0, 0);
            double sum = 0;
            double min = Double.POSITIVE_INFINITY;
            double max = Double.NEGATIVE_INFINITY;
            for (int i = 1; i < renderTimesNs.size(); i++) {
                double delta = (renderTimesNs.get(i) - renderTimesNs.get(i - 1)) / 1_000_000.0;
                sum += delta;
                min = Math.min(min, delta);
                max = Math.max(max, delta);
            }
            return new RenderStats(renderTimesNs.size() - 1,
                    sum / (renderTimesNs.size() - 1), min, max);
        }
    }

    private void setStatus(String text) {
        main.post(() -> status.setText(text));
    }

    private static String format(double value) {
        return String.format(Locale.US, "%.3f", value);
    }

    private static final class AccessUnit {
        final int bytes;
        final long ptsUs;
        AccessUnit(int bytes, long ptsUs) { this.bytes = bytes; this.ptsUs = ptsUs; }
    }

    private static final class RenderStats {
        final int intervals;
        final double averageMs;
        final double minimumMs;
        final double maximumMs;
        RenderStats(int intervals, double averageMs, double minimumMs, double maximumMs) {
            this.intervals = intervals;
            this.averageMs = averageMs;
            this.minimumMs = minimumMs;
            this.maximumMs = maximumMs;
        }
    }

    private final class ProbeLayout extends FrameLayout {
        ProbeLayout(Context context) { super(context); }
        @Override
        public boolean dispatchTouchEvent(MotionEvent event) {
            recordTouch(event);
            return true;
        }
    }
}
