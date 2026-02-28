package com.android.launcher3.lvgl;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.view.Surface;

public class LVGLEntrance {

    private static Context sContext;
    private static Activity sActivity;

    static {
        System.loadLibrary("lvgl-android");
    }

    /** 由 MainActivity 在 onCreate 时调用，供 native 通过 launchApp 拉起其他应用 */
    public static void setContext(Context context) {
        sContext = context != null ? context.getApplicationContext() : null;
    }

    /** 由 MainActivity 在 onCreate/onDestroy 时调用，供 launchAppForResult 使用以收到 onActivityResult */
    public static void setActivity(Activity activity) {
        sActivity = activity;
    }

    /** 由 native 调用：根据包名启动已安装的应用 */
    public static void launchApp(String packageName) {
        if (sContext == null || packageName == null || packageName.isEmpty()) return;
        try {
            Intent intent = sContext.getPackageManager().getLaunchIntentForPackage(packageName);
            if (intent != null) {
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                sContext.startActivity(intent);
            }
        } catch (Exception e) {
            android.util.Log.e("LVGLEntrance", "launchApp failed: " + packageName, e);
        }
    }

    /**
     * 由 native 调用：以 startActivityForResult 方式启动应用，以便对方 setResult() 后 MainActivity 能收到 onActivityResult。
     * 若未设置 Activity 则回退为 launchApp(packageName)。
     */
    public static void launchAppForResult(String packageName, int requestCode) {
        if (packageName == null || packageName.isEmpty()) return;
        if (sActivity == null) {
            launchApp(packageName);
            return;
        }
        try {
            Intent intent = sActivity.getPackageManager().getLaunchIntentForPackage(packageName);
            if (intent != null) {
                sActivity.startActivityForResult(intent, requestCode);
            } else {
                launchApp(packageName);
            }
        } catch (Exception e) {
            android.util.Log.e("LVGLEntrance", "launchAppForResult failed: " + packageName, e);
            launchApp(packageName);
        }
    }

    public static native void nativeCreate(Surface surface);

    public static native void nativeChanged(Surface surface, int width, int height);

    public static native void nativeDestroy(Surface surface);

    public static native void nativeTouch(int x, int y, boolean touch);

    /** 由 Java 调用：根据 regionIndex 通知 native 切换到底部对应屏幕 */
    public static native void nativeSetRegionIndex(int regionIndex);

    // Called from Java camera code to deliver RGBA frame bytes to native
    public static native void nativeCameraFrame(byte[] rgba, int width, int height, int camIndex);
}
