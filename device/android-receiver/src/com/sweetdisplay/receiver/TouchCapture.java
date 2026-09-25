package com.sweetdisplay.receiver;

import android.util.Log;
import android.view.MotionEvent;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicLong;

final class TouchCapture {
    private static final String TAG = "SWDPRX";
    private static final int CAPACITY = 64;
    private static final int DOWN = 1, MOVE = 2, UP = 3, CANCEL = 4;

    static final class Item {
        final int slot;
        final int action;
        final byte[] payload;
        Item(int slot, int action, byte[] payload) {
            this.slot = slot;
            this.action = action;
            this.payload = payload;
        }
    }

    final AtomicLong accepted = new AtomicLong();
    final AtomicLong rejectedOutside = new AtomicLong();
    final AtomicLong coalescedMoves = new AtomicLong();
    final AtomicLong queueOverflows = new AtomicLong();
    final AtomicLong sent = new AtomicLong();
    final AtomicLong sessions = new AtomicLong();

    private final ArrayList<Item> queue = new ArrayList<>();
    private final int[] pointerForSlot = {-1, -1};
    private final int[] lastX = new int[2];
    private final int[] lastY = new int[2];
    private Protocol.TouchConfiguration configuration;
    private int surfaceWidth;
    private int surfaceHeight;
    private int rotation;
    private int activeMask;

    synchronized void begin(Protocol.TouchConfiguration value) {
        clear();
        configuration = value;
        sessions.incrementAndGet();
    }

    synchronized void end() {
        configuration = null;
        clear();
    }

    synchronized void updateSurface(int width, int height, int displayRotation) {
        surfaceWidth = width;
        surfaceHeight = height;
        rotation = displayRotation & 3;
    }

    synchronized boolean capture(MotionEvent event) {
        if (configuration == null || surfaceWidth <= 0 || surfaceHeight <= 0) return true;
        final int masked = event.getActionMasked();
        final int index = event.getActionIndex();
        final long timestamp = Math.max(1L, event.getEventTime() * 1_000_000L);
        try {
            if (masked == MotionEvent.ACTION_DOWN || masked == MotionEvent.ACTION_POINTER_DOWN) {
                int pointer = event.getPointerId(index);
                if (findSlot(pointer) >= 0) return true;
                int slot = freeSlot();
                if (slot < 0) return true;
                int[] normalized = normalize(event.getX(index), event.getY(index));
                if (normalized == null) { rejectedOutside.incrementAndGet(); return true; }
                pointerForSlot[slot] = pointer;
                activeMask |= 1 << slot;
                lastX[slot] = normalized[0]; lastY[slot] = normalized[1];
                enqueue(slot, DOWN, normalized[0], normalized[1], event.getPressure(index), timestamp);
                logMap("DOWN", slot, event.getX(index), event.getY(index), normalized);
            } else if (masked == MotionEvent.ACTION_MOVE) {
                for (int i = 0; i < event.getPointerCount(); ++i) {
                    int slot = findSlot(event.getPointerId(i));
                    if (slot < 0) continue;
                    int[] normalized = normalize(event.getX(i), event.getY(i));
                    if (normalized == null) {
                        rejectedOutside.incrementAndGet();
                        activeMask &= ~(1 << slot);
                        enqueue(slot, CANCEL, lastX[slot], lastY[slot], 0, timestamp);
                        pointerForSlot[slot] = -1;
                    } else {
                        lastX[slot] = normalized[0]; lastY[slot] = normalized[1];
                        enqueue(slot, MOVE, normalized[0], normalized[1], event.getPressure(i), timestamp);
                        if ((accepted.get() % 60) == 0) logMap("MOVE", slot,
                                event.getX(i), event.getY(i), normalized);
                    }
                }
            } else if (masked == MotionEvent.ACTION_UP || masked == MotionEvent.ACTION_POINTER_UP) {
                int slot = findSlot(event.getPointerId(index));
                if (slot >= 0) {
                    int[] normalized = normalize(event.getX(index), event.getY(index));
                    activeMask &= ~(1 << slot);
                    enqueue(slot, normalized == null ? CANCEL : UP,
                            lastX[slot], lastY[slot],
                            normalized == null ? 0 : event.getPressure(index), timestamp);
                    if (normalized == null) rejectedOutside.incrementAndGet();
                    else logMap("UP", slot, event.getX(index), event.getY(index),
                            new int[]{lastX[slot], lastY[slot]});
                    pointerForSlot[slot] = -1;
                }
            } else if (masked == MotionEvent.ACTION_CANCEL) {
                for (int slot = 0; slot < pointerForSlot.length; ++slot) if (pointerForSlot[slot] >= 0) {
                    activeMask &= ~(1 << slot);
                    enqueue(slot, CANCEL, lastX[slot], lastY[slot], 0, timestamp);
                    pointerForSlot[slot] = -1;
                }
            }
        } catch (Exception error) {
            queueOverflows.incrementAndGet();
            Log.e(TAG, "TOUCH_CAPTURE_ERROR", error);
        }
        return true;
    }

    synchronized List<Item> drain(int maximum) {
        int count = Math.min(maximum, queue.size());
        ArrayList<Item> result = new ArrayList<>(queue.subList(0, count));
        queue.subList(0, count).clear();
        sent.addAndGet(count);
        return result;
    }

    synchronized int activeMask() { return activeMask; }
    synchronized int queued() { return queue.size(); }

    private void enqueue(int slot, int action, int x, int y, float rawPressure,
                         long timestamp) throws Exception {
        int pressure = action == CANCEL ? 0 : Math.max(0, Math.min(1024,
                Math.round(rawPressure * 1024.0f)));
        int flags = (action == CANCEL ? 0 : 1) | (slot == 0 ? 2 : 0) | (rotation << 8);
        Item item = new Item(slot, action, Protocol.touchContact(slot, action, x, y,
                pressure, flags, activeMask, timestamp));
        if (action == MOVE && !queue.isEmpty()) {
            Item tail = queue.get(queue.size() - 1);
            if (tail.action == MOVE && tail.slot == slot) {
                queue.set(queue.size() - 1, item);
                coalescedMoves.incrementAndGet();
                return;
            }
        }
        if (queue.size() >= CAPACITY) {
            int removable = -1;
            for (int i = 0; i < queue.size(); ++i) if (queue.get(i).action == MOVE) {
                removable = i; break;
            }
            if (removable < 0) throw new Protocol.Violation("TOUCH queue transition overflow");
            queue.remove(removable);
            coalescedMoves.incrementAndGet();
        }
        queue.add(item);
        accepted.incrementAndGet();
    }

    private int[] normalize(float x, float y) {
        double[] rectangle = contentRectangle();
        double left = rectangle[0], top = rectangle[1], width = rectangle[2],
                height = rectangle[3];
        if (x < left || y < top || x > left + width || y > top + height) return null;
        int nx = (int)Math.round((x - left) * configuration.coordinateMaximum / width);
        int ny = (int)Math.round((y - top) * configuration.coordinateMaximum / height);
        nx = Math.max(0, Math.min(configuration.coordinateMaximum, nx));
        ny = Math.max(0, Math.min(configuration.coordinateMaximum, ny));
        return new int[]{nx, ny};
    }

    private double[] contentRectangle() {
        double video = (double)configuration.targetWidth / configuration.targetHeight;
        double surface = (double)surfaceWidth / surfaceHeight;
        double left = 0, top = 0, width = surfaceWidth, height = surfaceHeight;
        if (surface > video) {
            width = surfaceHeight * video;
            left = (surfaceWidth - width) / 2.0;
        } else {
            height = surfaceWidth / video;
            top = (surfaceHeight - height) / 2.0;
        }
        return new double[]{left, top, width, height};
    }

    private void logMap(String action, int slot, float x, float y, int[] normalized) {
        double[] r = contentRectangle();
        Log.i(TAG, "TOUCH_MAP action=" + action + " slot=" + slot
                + " surface=" + x + "," + y
                + " content=" + r[0] + "," + r[1] + "," + r[2] + "," + r[3]
                + " normalized=" + normalized[0] + "," + normalized[1]
                + " rotation=" + rotation);
    }

    private int findSlot(int pointer) {
        for (int i = 0; i < pointerForSlot.length; ++i) if (pointerForSlot[i] == pointer) return i;
        return -1;
    }

    private int freeSlot() {
        for (int i = 0; i < pointerForSlot.length; ++i) if (pointerForSlot[i] < 0) return i;
        return -1;
    }

    private void clear() {
        queue.clear(); activeMask = 0;
        for (int i = 0; i < pointerForSlot.length; ++i) pointerForSlot[i] = -1;
    }
}
