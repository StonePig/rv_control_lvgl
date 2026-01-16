package com.hybird.lvgl.android.lvgl;

import android.view.Surface;

public class LVGLEntrance {

    static {
        System.loadLibrary("lvgl-android");
    }

    public static native void nativeCreate(Surface surface);

    public static native void nativeChanged(Surface surface, int width, int height);

    public static native void nativeDestroy(Surface surface);

    public static native void nativeTouch(int x, int y, boolean touch);

    // Called from Java camera code to deliver RGBA frame bytes to native
    public static native void nativeCameraFrame(byte[] rgba, int width, int height);
}
