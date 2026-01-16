package com.hybird.lvgl.android;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.media.Image;
import android.media.ImageReader;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.hybird.lvgl.android.databinding.ActivityMainBinding;
import com.hybird.lvgl.android.lvgl.LVGLEntrance;

import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private ActivityMainBinding binding;
    private static MainActivity instance;

    // Camera2
    private CameraDevice cameraDevice;
    private CameraCaptureSession captureSession;
    private ImageReader imageReader;
    private HandlerThread cameraThread;
    private Handler cameraHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        // 设置全屏
        setFullscreenImmersive();

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        stopCamera();
        instance = null;
    }

    public static MainActivity getInstance() {
        return instance;
    }

    private void setFullscreenImmersive() {
        View decorView = getWindow().getDecorView();

        // 新API（Android 4.4+）沉浸式全屏
        int uiOptions =
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

        decorView.setSystemUiVisibility(uiOptions);

        // 旧API全屏支持
        getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
        );

        // 保持屏幕常亮
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // 对于刘海屏设备
        WindowManager.LayoutParams params = getWindow().getAttributes();
        params.layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        getWindow().setAttributes(params);
    }

    // Called from native bridge to request camera start
    public static void startCamera() {
        MainActivity a = getInstance();
        if (a != null) a._startCameraInternal();
    }

    // Called from native bridge to request camera stop
    public static void stopCamera() {
        MainActivity a = getInstance();
        if (a != null) a._stopCameraInternal();
    }

    private void _startCameraInternal() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, 1001);
            return;
        }

        if (cameraDevice != null) return;

        cameraThread = new HandlerThread("CameraThread");
        cameraThread.start();
        cameraHandler = new Handler(cameraThread.getLooper());

        CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            String cameraId = manager.getCameraIdList()[0];
            imageReader = ImageReader.newInstance(640, 480, ImageFormat.YUV_420_888, 2);
            imageReader.setOnImageAvailableListener(reader -> {
                Image image = reader.acquireLatestImage();
                if (image == null) return;
                byte[] rgba = yuv420ToRgba(image);
                image.close();
                if (rgba != null) {
                    LVGLEntrance.nativeCameraFrame(rgba, 640, 480);
                }
            }, cameraHandler);

            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                return;
            }
            manager.openCamera(cameraId, new CameraDevice.StateCallback() {
                @Override
                public void onOpened(CameraDevice camera) {
                    cameraDevice = camera;
                    try {
                        Surface surface = imageReader.getSurface();
                        final CaptureRequest.Builder builder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                        builder.addTarget(surface);
                        cameraDevice.createCaptureSession(Arrays.asList(surface), new CameraCaptureSession.StateCallback() {
                            @Override
                            public void onConfigured(CameraCaptureSession session) {
                                captureSession = session;
                                try {
                                    session.setRepeatingRequest(builder.build(), null, cameraHandler);
                                } catch (CameraAccessException e) {
                                    Log.e(TAG, "setRepeatingRequest failed", e);
                                }
                            }

                            @Override
                            public void onConfigureFailed(CameraCaptureSession session) {
                                Log.e(TAG, "configure failed");
                            }
                        }, cameraHandler);
                    } catch (CameraAccessException e) {
                        Log.e(TAG, "openCamera onOpened exception", e);
                    }
                }

                @Override
                public void onDisconnected(CameraDevice camera) {
                    camera.close();
                    cameraDevice = null;
                }

                @Override
                public void onError(CameraDevice camera, int error) {
                    camera.close();
                    cameraDevice = null;
                }
            }, cameraHandler);

        } catch (CameraAccessException e) {
            Log.e(TAG, "CameraAccessException", e);
        }
    }

    private void _stopCameraInternal() {
        if (captureSession != null) {
            try { captureSession.close(); } catch (Exception e) {}
            captureSession = null;
        }
        if (cameraDevice != null) {
            try { cameraDevice.close(); } catch (Exception e) {}
            cameraDevice = null;
        }
        if (imageReader != null) {
            try { imageReader.close(); } catch (Exception e) {}
            imageReader = null;
        }
        if (cameraThread != null) {
            cameraThread.quitSafely();
            cameraThread = null;
            cameraHandler = null;
        }
    }

    // Convert YUV_420_888 Image to RGBA byte[] (naive, not optimized)
    private byte[] yuv420ToRgba(Image image) {
        try {
            int width = image.getWidth();
            int height = image.getHeight();
            Image.Plane[] planes = image.getPlanes();
            ByteBuffer yBuf = planes[0].getBuffer();
            ByteBuffer uBuf = planes[1].getBuffer();
            ByteBuffer vBuf = planes[2].getBuffer();

            int yRowStride = planes[0].getRowStride();
            int uRowStride = planes[1].getRowStride();
            int vRowStride = planes[2].getRowStride();
            int uPixelStride = planes[1].getPixelStride();

            byte[] y = new byte[yBuf.remaining()];
            yBuf.get(y);
            byte[] u = new byte[uBuf.remaining()];
            uBuf.get(u);
            byte[] v = new byte[vBuf.remaining()];
            vBuf.get(v);

            byte[] out = new byte[width * height * 4];
            int yp = 0;
            for (int j = 0; j < height; j++) {
                int pY = j * yRowStride;
                int pUV = (j >> 1) * uRowStride;
                for (int i = 0; i < width; i++) {
                    int Y = y[pY + i] & 0xff;
                    int U = u[pUV + (i >> 1) * uPixelStride] & 0xff;
                    int V = v[pUV + (i >> 1) * uPixelStride] & 0xff;

                    int C = Y - 16;
                    int D = U - 128;
                    int E = V - 128;

                    int R = (298 * C + 409 * E + 128) >> 8;
                    int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
                    int B = (298 * C + 516 * D + 128) >> 8;

                    if (R < 0) R = 0; else if (R > 255) R = 255;
                    if (G < 0) G = 0; else if (G > 255) G = 255;
                    if (B < 0) B = 0; else if (B > 255) B = 255;

                    int outIndex = yp * 4;
                    out[outIndex] = (byte) B;
                    out[outIndex + 1] = (byte) G;
                    out[outIndex + 2] = (byte) R;
                    out[outIndex + 3] = (byte) 0xFF;
                    yp++;
                }
            }
            return out;
        } catch (Exception ex) {
            Log.e(TAG, "yuv convert error", ex);
            return null;
        }
    }
}