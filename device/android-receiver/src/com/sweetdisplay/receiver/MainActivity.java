package com.sweetdisplay.receiver;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.graphics.Color;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.BatteryManager;
import android.os.PowerManager;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.util.Locale;

public final class MainActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = "SWDPRX";
    private final Handler main = new Handler(Looper.getMainLooper());
    private SurfaceView surfaceView;
    private TextView status;
    private Receiver receiver;
    private boolean ackOnly;
    private boolean ncmDiagnostic;
    private long touchDown;
    private long touchMove;
    private long touchUp;
    private float touchMinX = Float.POSITIVE_INFINITY;
    private float touchMaxX = Float.NEGATIVE_INFINITY;
    private float touchMinY = Float.POSITIVE_INFINITY;
    private float touchMaxY = Float.NEGATIVE_INFINITY;

    private final Runnable periodic = new Runnable() {
        @Override public void run() {
            if (receiver != null) {
                Intent battery = registerReceiver(null,
                        new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
                int batteryTemperature = battery == null ? -1
                        : battery.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1);
                int batteryStatus = battery == null ? -1
                        : battery.getIntExtra(BatteryManager.EXTRA_STATUS, -1);
                PowerManager power = (PowerManager)getSystemService(POWER_SERVICE);
                Display display = ((DisplayManager)getSystemService(DISPLAY_SERVICE))
                        .getDisplay(Display.DEFAULT_DISPLAY);
                receiver.logSnapshot("PERIODIC", power.getCurrentThermalStatus(),
                        batteryTemperature, batteryStatus,
                        display.getMode().getRefreshRate());
                status.setText("SweetDisplay Receiver\n" + receiver.statusLine()
                        + "\nTouch D/M/U=" + touchDown + "/" + touchMove + "/" + touchUp);
            }
            main.postDelayed(this, 1000);
        }
    };

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        ncmDiagnostic = getIntent().getBooleanExtra("ncm_diagnostic", false);
        ackOnly = ncmDiagnostic || getIntent().getBooleanExtra("ack_only", false);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        if (!ncmDiagnostic) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
        getWindow().setStatusBarColor(Color.TRANSPARENT);
        getWindow().setNavigationBarColor(Color.BLACK);
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);

        ProbeLayout root = new ProbeLayout(this);
        root.setBackgroundColor(Color.BLACK);
        if (!ackOnly) {
            surfaceView = new SurfaceView(this);
            surfaceView.getHolder().addCallback(this);
            root.addView(surfaceView, new FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        }
        status = new TextView(this);
        status.setTextColor(Color.WHITE);
        status.setBackgroundColor(0x90000000);
        status.setTextSize(12.0f);
        status.setPadding(20, 14, 20, 14);
        status.setText(ackOnly ? "SweetDisplay Receiver\nPERF1 ACK-only tanı kipi"
                : "SweetDisplay Receiver\nSurface bekleniyor...");
        root.addView(status, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.TOP | Gravity.START));
        setContentView(root);
        if (ackOnly) {
            receiver = new Receiver(null, main, true,
                    ncmDiagnostic ? "0.0.0.0" : "127.0.0.1");
            Log.i(TAG, "MODE ackOnly=true surface=false decoder=false ncmDiagnostic="
                    + ncmDiagnostic);
        } else {
            Log.i(TAG, "MODE ackOnly=false surface=true decoder=true");
        }
        logDisplay("CREATE");
        main.postDelayed(periodic, 1000);
    }

    @Override
    protected void onDestroy() {
        main.removeCallbacks(periodic);
        if (receiver != null) {
            receiver.stop();
            receiver = null;
        }
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
        if (receiver == null) {
            receiver = new Receiver(holder.getSurface(), main, false, "127.0.0.1");
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i(TAG, "SURFACE_CHANGED width=" + width + " height=" + height
                + " format=" + format + " rotation=" + currentRotation());
        if (receiver != null) receiver.updateSurface(width, height, currentRotation());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "SURFACE_DESTROYED");
        if (receiver != null) {
            receiver.stop();
            receiver = null;
        }
    }

    private void logDisplay(String event) {
        Display display = ((DisplayManager)getSystemService(DISPLAY_SERVICE))
                .getDisplay(Display.DEFAULT_DISPLAY);
        Display.Mode mode = display.getMode();
        Log.i(TAG, "DISPLAY event=" + event
                + " mode=" + mode.getPhysicalWidth() + "x" + mode.getPhysicalHeight()
                + " refresh=" + format(mode.getRefreshRate())
                + " rotation=" + display.getRotation());
    }

    private int currentRotation() {
        return ((DisplayManager)getSystemService(DISPLAY_SERVICE))
                .getDisplay(Display.DEFAULT_DISPLAY).getRotation();
    }

    private void recordTouch(MotionEvent event) {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) touchDown++;
        if (action == MotionEvent.ACTION_MOVE) touchMove++;
        if (action == MotionEvent.ACTION_UP) touchUp++;
        float x = event.getX();
        float y = event.getY();
        touchMinX = Math.min(touchMinX, x);
        touchMaxX = Math.max(touchMaxX, x);
        touchMinY = Math.min(touchMinY, y);
        touchMaxY = Math.max(touchMaxY, y);
        if (action != MotionEvent.ACTION_MOVE || (touchMove % 60) == 0) {
            InputDevice input = event.getDevice();
            InputDevice.MotionRange rangeX = input == null ? null
                    : input.getMotionRange(MotionEvent.AXIS_X, event.getSource());
            InputDevice.MotionRange rangeY = input == null ? null
                    : input.getMotionRange(MotionEvent.AXIS_Y, event.getSource());
            Log.i(TAG, "TOUCH action=" + MotionEvent.actionToString(action)
                    + " pointerId=" + event.getPointerId(0)
                    + " x=" + format(x) + " y=" + format(y)
                    + " pressure=" + format(event.getPressure())
                    + " eventTimeMs=" + event.getEventTime()
                    + " logicalX=" + (rangeX == null ? "unknown"
                    : format(rangeX.getMin()) + ".." + format(rangeX.getMax()))
                    + " logicalY=" + (rangeY == null ? "unknown"
                    : format(rangeY.getMin()) + ".." + format(rangeY.getMax()))
                    + " observedX=" + format(touchMinX) + ".." + format(touchMaxX)
                    + " observedY=" + format(touchMinY) + ".." + format(touchMaxY));
        }
    }

    private static String format(double value) {
        return String.format(Locale.US, "%.3f", value);
    }

    private final class ProbeLayout extends FrameLayout {
        ProbeLayout(Context context) { super(context); }
        @Override public boolean dispatchTouchEvent(MotionEvent event) {
            recordTouch(event);
            return receiver == null || receiver.captureTouch(event);
        }
    }
}
