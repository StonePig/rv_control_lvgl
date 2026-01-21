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
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.widget.FrameLayout;
import android.widget.TextView;
import java.util.ArrayList;
import java.util.List;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private ActivityMainBinding binding;
    private static MainActivity instance;

    // Camera2
    private CameraDevice[] cameraDevices = new CameraDevice[3];
    private CameraCaptureSession[] captureSessions = new CameraCaptureSession[3];
    private ImageReader[] imageReaders = new ImageReader[3];
    private HandlerThread cameraThread;
    private Handler cameraHandler;
    // SurfaceViews used as preview targets (hidden in layout)
    private SurfaceView[] previewViews = new SurfaceView[3];
    private SurfaceHolder[] previewHolders = new SurfaceHolder[3];
    private boolean[] previewReady = new boolean[3];
    // Main preview SurfaceView
    private SurfaceView mainPreviewView;
    private SurfaceHolder mainPreviewHolder;
    private boolean mainPreviewReady = false;
    private int currentMainCameraIndex = 0;
    
    // TextView elements for camera info display
    private TextView[] camInfoTextViews = new TextView[3];
    private TextView mainCamInfoTextView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        // 设置全屏
        setFullscreenImmersive();

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // find hidden SurfaceViews used for camera preview targets
        try {
            previewViews[0] = findViewById(R.id.surface_cam0);
            previewViews[1] = findViewById(R.id.surface_cam1);
            previewViews[2] = findViewById(R.id.surface_cam2);
            // Initialize main preview SurfaceView
            mainPreviewView = findViewById(R.id.surface_cam_main);
            if (mainPreviewView != null) {
                mainPreviewHolder = mainPreviewView.getHolder();
                mainPreviewHolder.addCallback(new SurfaceHolder.Callback() {
                    @Override
                    public void surfaceCreated(SurfaceHolder holder) {
                        mainPreviewHolder = holder;
                        mainPreviewReady = true;
                        // Restart current main camera if it's already open
                        if (cameraDevices[currentMainCameraIndex] != null) {
                            _stopCameraInternal(currentMainCameraIndex);
                            _startCameraInternal(currentMainCameraIndex);
                        }
                    }

                    @Override
                    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                        // Restart current main camera if it's already open
                        if (cameraDevices[currentMainCameraIndex] != null) {
                            _stopCameraInternal(currentMainCameraIndex);
                            _startCameraInternal(currentMainCameraIndex);
                        }
                    }

                    @Override
                    public void surfaceDestroyed(SurfaceHolder holder) {
                        mainPreviewReady = false;
                        mainPreviewHolder = null;
                    }
                });
            }
            for (int i = 0; i < 3; i++) {
                final int idx = i;
                if (previewViews[i] != null) {
                    previewHolders[i] = previewViews[i].getHolder();
                    previewHolders[i].addCallback(new SurfaceHolder.Callback() {
                        @Override
                        public void surfaceCreated(SurfaceHolder holder) {
                            previewHolders[idx] = holder;
                            previewReady[idx] = true;
                            // Restart camera if it's already open but using fallback ImageReader
                            if (cameraDevices[idx] != null) {
                                _stopCameraInternal(idx);
                                _startCameraInternal(idx);
                            }
                        }

                        @Override
                        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                            // Restart camera when surface changes
                            if (cameraDevices[idx] != null) {
                                _stopCameraInternal(idx);
                                _startCameraInternal(idx);
                            }
                        }

                        @Override
                        public void surfaceDestroyed(SurfaceHolder holder) {
                            previewReady[idx] = false;
                            previewHolders[idx] = null;
                        }
                    });
                }
            }
            
            // Initialize camera info TextViews
            camInfoTextViews[0] = findViewById(R.id.text_cam0_info);
            camInfoTextViews[1] = findViewById(R.id.text_cam1_info);
            camInfoTextViews[2] = findViewById(R.id.text_cam2_info);
            mainCamInfoTextView = findViewById(R.id.text_cam_main_info);
        } catch (Exception ex) {
            Log.w(TAG, "preview surface views not found or not available", ex);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // stop all cameras
        for (int i = 0; i < 3; i++) stopCamera(i);
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

    // Called from native bridge to request camera start for index
    public static void startCamera(int index) {
        MainActivity a = getInstance();
        if (a != null) {
            // Ensure camera startup is performed on the camera thread
            synchronized (a) {
                // Ensure camera thread is properly initialized
                if (a.cameraThread == null) {
                    Log.d(TAG, "startCamera3: Initializing camera thread for index=" + index);
                    a.cameraThread = new HandlerThread("CameraThread");
                    a.cameraThread.start();
                    a.cameraHandler = new Handler(a.cameraThread.getLooper());
                }
            }
            
            // Now ensure camera startup is performed on the camera thread
            if (a.cameraHandler != null) {
                final MainActivity finalA = a;
                final int finalIndex = index;
                a.cameraHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        // 打印启动相机的索引
                        Log.d(TAG, "startCamera1: index=" + finalIndex);
                        finalA._startCameraInternal(finalIndex);
                    }
                });
            } else {
                // Fallback: if cameraHandler is still null, try to start directly on current thread
                Log.e(TAG, "startCamera2: index=" + index + ", cameraHandler is null");
                // Ensure camera thread is started
                if (a.cameraThread == null) {
                    a.cameraThread = new HandlerThread("CameraThread");
                    a.cameraThread.start();
                    a.cameraHandler = new Handler(a.cameraThread.getLooper());
                    // Try again with the newly created handler
                    if (a.cameraHandler != null) {
                        final MainActivity finalA = a;
                        final int finalIndex = index;
                        a.cameraHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                Log.d(TAG, "startCamera4: index=" + finalIndex + ", using newly created handler");
                                finalA._startCameraInternal(finalIndex);
                            }
                        });
                        return;
                    }
                }
                // Last resort: start directly on current thread
                a._startCameraInternal(index);
            }
        }
    }

    // Called from native bridge to request camera stop for index
    public static void stopCamera(int index) {
        MainActivity a = getInstance();
        if (a != null) {
            // Ensure camera stop is performed on the camera thread
            if (a.cameraHandler != null) {
                final MainActivity finalA = a;
                final int finalIndex = index;
                a.cameraHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        finalA._stopCameraInternal(finalIndex);
                    }
                });
            } else {
                // If cameraHandler is not yet initialized, stop it on the current thread
                a._stopCameraInternal(index);
            }
        }
    }

    // Called from native bridge to show/hide SurfaceViews
    public static void showSurfaceViews() {
        MainActivity a = getInstance();
        if (a != null) {
            for (SurfaceView view : a.previewViews) {
                if (view != null) {
                    view.setVisibility(View.VISIBLE);
                }
            }
        }
    }

    public static void hideSurfaceViews() {
        MainActivity a = getInstance();
        if (a != null) {
            for (SurfaceView view : a.previewViews) {
                if (view != null) {
                    view.setVisibility(View.GONE);
                }
            }
        }
    }

    // Called from native bridge to resize and reposition SurfaceView
    public static void setSurfaceViewParams(int index, int x, int y, int width, int height) {
        MainActivity a = getInstance();
        if (a != null && index >= 0 && index < a.previewViews.length) {
            SurfaceView view = a.previewViews[index];
            if (view != null) {
                final SurfaceView finalView = view;
                final TextView finalTextView = a.camInfoTextViews[index];
                final int finalX = x;
                final int finalY = y;
                final int finalWidth = width;
                final int finalHeight = height;
                // Use post() instead of runOnUiThread for more efficient UI updates
                finalView.post(new Runnable() {
                    @Override
                    public void run() {
                        // Set SurfaceView visibility to visible
                        finalView.setVisibility(View.VISIBLE);
                        // Set SurfaceView layout parameters
                        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) finalView.getLayoutParams();
                        params.leftMargin = finalX;
                        params.topMargin = finalY;
                        params.width = finalWidth;
                        params.height = finalHeight;
                        finalView.setLayoutParams(params);
                        
                        // Set TextView visibility and position
                        if (finalTextView != null) {
                            finalTextView.setVisibility(View.VISIBLE);
                            FrameLayout.LayoutParams textParams = (FrameLayout.LayoutParams) finalTextView.getLayoutParams();
                            // Position TextView at top-left corner of SurfaceView
                            textParams.leftMargin = finalX + 8; // 8dp margin
                            textParams.topMargin = finalY + 8; // 8dp margin
                            finalTextView.setLayoutParams(textParams);
                        }
                    }
                });
            }
        }
    }

    // Called from native bridge to set SurfaceView visibility
    public static void setSurfaceViewVisibility(int index, boolean visible) {
        MainActivity a = getInstance();
        if (a != null && index >= 0 && index < a.previewViews.length) {
            SurfaceView view = a.previewViews[index];
            TextView textView = a.camInfoTextViews[index];
            if (view != null) {
                final SurfaceView finalView = view;
                final TextView finalTextView = textView;
                final boolean finalVisible = visible;
                // Ensure UI operations are performed on the UI thread
                a.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        finalView.setVisibility(finalVisible ? View.VISIBLE : View.GONE);
                        if (finalTextView != null) {
                            finalTextView.setVisibility(finalVisible ? View.VISIBLE : View.GONE);
                        }
                    }
                });
            }
        }
    }

    // Called from native bridge to reset SurfaceView to initial state
    public static void resetSurfaceViews() {
        MainActivity a = getInstance();
        if (a != null) {
            final MainActivity finalA = a;
            // Ensure UI operations are performed on the UI thread
            a.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    for (int i = 0; i < finalA.previewViews.length; i++) {
                        SurfaceView view = finalA.previewViews[i];
                        TextView textView = finalA.camInfoTextViews[i];
                        if (view != null) {
                            // Reset layout parameters
                            FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) view.getLayoutParams();
                            params.leftMargin = 0;
                            params.topMargin = 0;
                            params.width = 1;
                            params.height = 1;
                            // view.setLayoutParams(params);
                            // Hide the SurfaceView
                            view.setVisibility(View.GONE);
                        }
                        // Hide the corresponding TextView
                        if (textView != null) {
                            textView.setVisibility(View.GONE);
                        }
                    }
                    // Reset main preview SurfaceView
                    if (finalA.mainPreviewView != null) {
                        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) finalA.mainPreviewView.getLayoutParams();
                        params.leftMargin = 0;
                        params.topMargin = 0;
                        params.width = 1;
                        params.height = 1;
                        // finalA.mainPreviewView.setLayoutParams(params);
                        finalA.mainPreviewView.setVisibility(View.GONE);
                    }
                    // Hide main camera info TextView
                    if (finalA.mainCamInfoTextView != null) {
                        finalA.mainCamInfoTextView.setVisibility(View.GONE);
                    }
                }
            });
        }
    }

    // Called from native bridge to set main preview SurfaceView params
    public static void setMainSurfaceViewParams(int x, int y, int width, int height) {
        MainActivity a = getInstance();
        if (a != null && a.mainPreviewView != null) {
            final SurfaceView finalView = a.mainPreviewView;
            final TextView finalTextView = a.mainCamInfoTextView;
            final MainActivity finalA = a;
            final int finalX = x;
            final int finalY = y;
            final int finalWidth = width;
            final int finalHeight = height;
            // Use post() instead of runOnUiThread for more efficient UI updates
            finalView.post(new Runnable() {
                @Override
                public void run() {
                    finalView.setVisibility(View.VISIBLE);
                    FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) finalView.getLayoutParams();
                    params.leftMargin = finalX;
                    params.topMargin = finalY;
                    params.width = finalWidth;
                    params.height = finalHeight;
                    finalView.setLayoutParams(params);
                    
                    // Set main camera info TextView visibility, text and position
                    if (finalTextView != null) {
                        // Update text to show current selected camera
                        finalTextView.setText("camera" + (finalA.currentMainCameraIndex + 1));
                        finalTextView.setVisibility(View.VISIBLE);
                        FrameLayout.LayoutParams textParams = (FrameLayout.LayoutParams) finalTextView.getLayoutParams();
                        // Position TextView at top-left corner of main SurfaceView
                        textParams.leftMargin = finalX + 8; // 8dp margin
                        textParams.topMargin = finalY + 8; // 8dp margin
                        finalTextView.setLayoutParams(textParams);
                    }
                }
            });
        }
    }

    // Called from native bridge to set current main camera index
    public static void setCurrentMainCameraIndex(int index) {
        MainActivity a = getInstance();
        if (a != null && index >= 0 && index < 3) {
            final int finalIndex = index;
            final MainActivity finalA = a;
            // Ensure UI operations are performed on the UI thread
            a.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (finalA.currentMainCameraIndex != finalIndex) {
                        int oldIndex = finalA.currentMainCameraIndex;
                        finalA.currentMainCameraIndex = finalIndex;
                        // Update main camera info text
                        if (finalA.mainCamInfoTextView != null) {
                            finalA.mainCamInfoTextView.setText("camera" + (finalIndex + 1));
                        }
                        // Restart the cameras to update their preview targets
                        if (finalA.cameraDevices[oldIndex] != null) {
                            finalA._stopCameraInternal(oldIndex);
                            finalA._startCameraInternal(oldIndex);
                        }
                        if (finalA.cameraDevices[finalIndex] != null) {
                            finalA._stopCameraInternal(finalIndex);
                            finalA._startCameraInternal(finalIndex);
                        }
                    }
                }
            });
        }
    }

    private void _startCameraInternal(int index) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, 1001);
            return;
        }
        if (cameraDevices[index] != null) return;

        if (cameraThread == null) {
            cameraThread = new HandlerThread("CameraThread");
            cameraThread.start();
            cameraHandler = new Handler(cameraThread.getLooper());
        }

        CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        String[] ids = null;
        try {
            ids = manager.getCameraIdList();
        } catch (CameraAccessException e) {
            Log.e(TAG, "CameraAccessException getting camera list", e);
            return;
        }
        
        if (index < 0 || index >= ids.length) return;
        String cameraId = ids[index];

        final int camIdx = index;

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        
        try {
            manager.openCamera(cameraId, new CameraDevice.StateCallback() {
                @Override
                    public void onOpened(CameraDevice camera) {
                        // Store the camera device reference
                        cameraDevices[camIdx] = camera;
                        List<Surface> targets = new ArrayList<>();
                        // Add the small preview SurfaceView
                        boolean surfaceViewUsed = false;
                        try {
                            if (previewReady[camIdx] && previewHolders[camIdx] != null) {
                                Surface previewSurface = previewHolders[camIdx].getSurface();
                                if (previewSurface != null && previewSurface.isValid()) {
                                    targets.add(previewSurface);
                                    surfaceViewUsed = true;
                                }
                            }
                        } catch (Exception ex) {
                            Log.w(TAG, "preview surface unavailable for cam " + camIdx, ex);
                        }
                        // If this is the current main camera, also add the main preview SurfaceView
                        if (camIdx == currentMainCameraIndex) {
                            try {
                                if (mainPreviewReady && mainPreviewHolder != null) {
                                    Surface mainSurface = mainPreviewHolder.getSurface();
                                    if (mainSurface != null && mainSurface.isValid()) {
                                        targets.add(mainSurface);
                                    }
                                }
                            } catch (Exception ex) {
                                Log.w(TAG, "main preview surface unavailable for cam " + camIdx, ex);
                            }
                        }
                        // If no preview surface is available yet, use ImageReader as fallback
                        if (targets.isEmpty()) {
                            try {
                                imageReaders[camIdx] = ImageReader.newInstance(640*2, 480*2, ImageFormat.YUV_420_888, 2);
                                targets.add(imageReaders[camIdx].getSurface());
                            } catch (Exception ex) {
                                Log.e(TAG, "failed to create fallback ImageReader for cam " + camIdx, ex);
                            }
                        }
                        
                        // Create capture request with exception handling
                        final CaptureRequest.Builder builder;
                        try {
                            builder = camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
                            for (Surface s : targets) builder.addTarget(s);
                        } catch (IllegalStateException e) {
                            // Camera was already closed, handle gracefully
                            Log.w(TAG, "CameraDevice was already closed when creating capture request for cam " + camIdx, e);
                            // Release the camera
                            camera.close();
                            cameraDevices[camIdx] = null;
                            return;
                        } catch (CameraAccessException e) {
                            Log.e(TAG, "Failed to create capture request for cam " + camIdx, e);
                            // Release the camera
                            camera.close();
                            cameraDevices[camIdx] = null;
                            return;
                        }
                        
                        // Create capture session with exception handling
                        try {
                            camera.createCaptureSession(targets, new CameraCaptureSession.StateCallback() {
                                @Override
                                public void onConfigured(CameraCaptureSession session) {
                                    // Check if camera was already closed while waiting for onConfigured
                                    if (cameraDevices[camIdx] != camera) {
                                        // Camera was already closed, release this session
                                        try {
                                            session.close();
                                        } catch (Exception e) {
                                            Log.w(TAG, "failed to close session for cam " + camIdx, e);
                                        }
                                        return;
                                    }
                                    
                                    captureSessions[camIdx] = session;
                                    try {
                                        session.setRepeatingRequest(builder.build(), null, cameraHandler);
                                    } catch (CameraAccessException e) {
                                        Log.e(TAG, "setRepeatingRequest failed", e);
                                    } catch (IllegalStateException e) {
                                        // Camera was already closed, handle gracefully
                                        Log.w(TAG, "CameraDevice was already closed when setting repeating request for cam " + camIdx, e);
                                        // Release the session
                                        try {
                                            session.close();
                                        } catch (Exception ex) {
                                            Log.w(TAG, "failed to close session for cam " + camIdx, ex);
                                        }
                                    }
                                }

                                @Override
                                public void onConfigureFailed(CameraCaptureSession session) {
                                    Log.e(TAG, "configure failed for cam " + camIdx);
                                    // Release the session
                                    try {
                                        session.close();
                                    } catch (Exception e) {
                                        Log.w(TAG, "failed to close session for cam " + camIdx, e);
                                    }
                                }
                            }, cameraHandler);
                        } catch (IllegalStateException e) {
                            // Camera was already closed, handle gracefully
                            Log.w(TAG, "CameraDevice was already closed when creating capture session for cam " + camIdx, e);
                            // Release the camera
                            camera.close();
                            cameraDevices[camIdx] = null;
                        } catch (CameraAccessException e) {
                            Log.e(TAG, "Failed to create capture session for cam " + camIdx, e);
                            // Release the camera
                            camera.close();
                            cameraDevices[camIdx] = null;
                        }
                    }

                @Override
                public void onDisconnected(CameraDevice camera) {
                    camera.close();
                    cameraDevices[camIdx] = null;
                }

                @Override
                public void onError(CameraDevice camera, int error) {
                    camera.close();
                    cameraDevices[camIdx] = null;
                }
            }, cameraHandler);
        } catch (CameraAccessException e) {
            Log.e(TAG, "CameraAccessException opening camera", e);
        }
    }

    private void _stopCameraInternal(int index) {
        if (index < 0 || index >= 3) return;
        if (captureSessions[index] != null) {
            try { captureSessions[index].close(); } catch (Exception e) {}
            captureSessions[index] = null;
        }
        if (cameraDevices[index] != null) {
            try { cameraDevices[index].close(); } catch (Exception e) {}
            cameraDevices[index] = null;
        }
        if (imageReaders[index] != null) {
            try { imageReaders[index].close(); } catch (Exception e) {}
            imageReaders[index] = null;
        }

        // if all cameras stopped, stop thread
        boolean any = false;
        for (int i = 0; i < 3; i++) if (cameraDevices[i] != null) any = true;
        if (!any && cameraThread != null) {
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