package com.android.launcher3;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.NetworkInfo;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiNetworkSpecifier;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import java.util.List;

/**
 * WiFi 工具类：供 native 通过 JNI 调用，并回调扫描/连接结果到 native。
 * 需在 Application 或 MainActivity 中调用 init(context)。
 */
public class WiFiUtils {
    private static final String TAG = "WiFiUtils";

    static {
        // 确保在调用任何 native 方法前已加载 so
        System.loadLibrary("lvgl-android");
    }

    private static Context sContext;
    private static WifiManager sWifiManager;
    private static boolean sReceiverRegistered;
    private static long sLastScanRequestMs = 0;
    private static final long MIN_SCAN_INTERVAL_MS = 3000;
    private static final Handler sHandler = new Handler(Looper.getMainLooper());

    private static boolean isSecured(android.net.wifi.ScanResult r) {
        String cap = r.capabilities;
        return cap != null && (cap.contains("WPA") || cap.contains("WEP") || cap.contains("PSK"));
    }

    private static void pushScanResultsToNative() {
        if (sWifiManager == null) return;
        try {
            List<android.net.wifi.ScanResult> results = sWifiManager.getScanResults();
            if (results != null && !results.isEmpty()) {
                String[] ssids = new String[results.size()];
                int[] rssis = new int[results.size()];
                int[] secured = new int[results.size()];
                for (int i = 0; i < results.size(); i++) {
                    android.net.wifi.ScanResult r = results.get(i);
                    ssids[i] = r.SSID != null ? r.SSID : "";
                    rssis[i] = r.level;
                    secured[i] = isSecured(r) ? 1 : 0;
                }
                nativeOnScanResultsAvailable(ssids, rssis, secured);
            } else {
                nativeOnScanResultsAvailable(new String[0], new int[0], new int[0]);
            }
        } catch (SecurityException e) {
            Log.w(TAG, "getScanResults permission", e);
            nativeOnScanResultsAvailable(new String[0], new int[0], new int[0]);
        }
    }

    private static final BroadcastReceiver sScanReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(intent.getAction()) && sWifiManager != null) {
                pushScanResultsToNative();
            }
        }
    };

    public static void init(Context context) {
        if (context == null) return;
        sContext = context.getApplicationContext();
        sWifiManager = (WifiManager) sContext.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (sWifiManager != null && !sReceiverRegistered) {
            IntentFilter filter = new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
            // 兼容低 compileSdk / 旧 androidx：避免引用新常量与新重载；Android 13+ 尝试用 flags 注册
            if (Build.VERSION.SDK_INT >= 33) {
                final int RECEIVER_NOT_EXPORTED = 2; // android.content.Context.RECEIVER_NOT_EXPORTED
                boolean registered = false;
                try {
                    // API 33+: Intent registerReceiver(BroadcastReceiver, IntentFilter, int flags)
                    java.lang.reflect.Method m = Context.class.getMethod(
                            "registerReceiver",
                            BroadcastReceiver.class,
                            IntentFilter.class,
                            int.class
                    );
                    m.invoke(sContext, sScanReceiver, filter, RECEIVER_NOT_EXPORTED);
                    registered = true;
                } catch (Throwable ignore) {
                    // Some devices/SDKs might only expose the 5-arg overload
                }
                if (!registered) {
                    try {
                        // API 33+: Intent registerReceiver(BroadcastReceiver, IntentFilter, String, Handler, int flags)
                        java.lang.reflect.Method m = Context.class.getMethod(
                                "registerReceiver",
                                BroadcastReceiver.class,
                                IntentFilter.class,
                                String.class,
                                Handler.class,
                                int.class
                        );
                        m.invoke(sContext, sScanReceiver, filter, null, null, RECEIVER_NOT_EXPORTED);
                        registered = true;
                    } catch (Throwable ignore) {
                        // fallback below
                    }
                }
                if (!registered) {
                    sContext.registerReceiver(sScanReceiver, filter);
                }
            } else {
                sContext.registerReceiver(sScanReceiver, filter);
            }
            sReceiverRegistered = true;
        }
        nativeSetWiFiUtilsClass(WiFiUtils.class);
        nativeRegisterWiFiUtilsClass();
    }

    // ----- 以下由 native 调用 -----

    public static boolean setWifiEnabled(boolean enabled) {
        if (sWifiManager == null) return false;
        try {
            return sWifiManager.setWifiEnabled(enabled);
        } catch (Exception e) {
            Log.e(TAG, "setWifiEnabled", e);
            return false;
        }
    }

    public static boolean isWifiEnabled() {
        if (sWifiManager == null) return false;
        return sWifiManager.isWifiEnabled();
    }

    public static void startScan() {
        if (sWifiManager == null) return;
        long now = System.currentTimeMillis();
        if (now - sLastScanRequestMs < MIN_SCAN_INTERVAL_MS) {
            pushScanResultsToNative();
            return;
        }
        sLastScanRequestMs = now;
        try {
            sWifiManager.startScan();
        } catch (SecurityException e) {
            Log.w(TAG, "startScan permission", e);
        }
        sHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                pushScanResultsToNative();
            }
        }, 2500);
    }

    public static void connect(String ssid, String password) {
        if (sWifiManager == null || sContext == null || ssid == null) return;
        final String pwd = (password != null) ? password : "";
        final String ssidForCallback = ssid;
        // Android 10+ 带密码连接：使用 WifiNetworkSpecifier，否则旧 API 常失败
        if (Build.VERSION.SDK_INT >= 29 && !pwd.isEmpty()) {
            try {
                ConnectivityManager cm = (ConnectivityManager) sContext.getSystemService(Context.CONNECTIVITY_SERVICE);
                if (cm == null) {
                    nativeOnConnectionResult(false, null);
                    return;
                }
                WifiNetworkSpecifier specifier = new WifiNetworkSpecifier.Builder()
                        .setSsid(ssid)
                        .setWpa2Passphrase(pwd)
                        .build();
                NetworkRequest request = new NetworkRequest.Builder()
                        .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                        .setNetworkSpecifier(specifier)
                        .build();
                ConnectivityManager.NetworkCallback callback = new ConnectivityManager.NetworkCallback() {
                    private void unregisterLater() {
                        sHandler.postDelayed(() -> {
                            try {
                                cm.unregisterNetworkCallback(this);
                            } catch (Exception ignored) {}
                        }, 500);
                    }
                    @Override
                    public void onAvailable(Network network) {
                        nativeOnConnectionResult(true, ssidForCallback);
                        unregisterLater();
                    }
                    @Override
                    public void onUnavailable() {
                        nativeOnConnectionResult(false, null);
                        unregisterLater();
                    }
                };
                cm.requestNetwork(request, callback, sHandler, 30_000);
            } catch (Exception e) {
                Log.e(TAG, "connect (API29+)", e);
                nativeOnConnectionResult(false, null);
            }
            return;
        }
        // 旧 API 或开放网络：WifiConfiguration + addNetwork
        new Thread(() -> {
            try {
                WifiConfiguration config = new WifiConfiguration();
                config.SSID = "\"" + ssid.replace("\"", "\\\"") + "\"";
                if (pwd.isEmpty()) {
                    config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
                } else {
                    config.preSharedKey = "\"" + pwd + "\"";
                    config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
                    config.allowedProtocols.set(WifiConfiguration.Protocol.WPA);
                    config.allowedProtocols.set(WifiConfiguration.Protocol.RSN);
                    config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
                    config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
                    config.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.TKIP);
                    config.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.CCMP);
                }
                int netId = sWifiManager.addNetwork(config);
                if (netId == -1 && Build.VERSION.SDK_INT < 29) {
                    List<WifiConfiguration> list = sWifiManager.getConfiguredNetworks();
                    if (list != null) {
                        for (WifiConfiguration c : list) {
                            if (c.SSID != null && c.SSID.equals(config.SSID)) {
                                netId = c.networkId;
                                if (!pwd.isEmpty()) {
                                    c.preSharedKey = "\"" + pwd + "\"";
                                    c.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
                                    sWifiManager.updateNetwork(c);
                                }
                                break;
                            }
                        }
                    }
                }
                boolean ok = false;
                if (netId != -1) {
                    sWifiManager.disconnect();
                    ok = sWifiManager.enableNetwork(netId, true) && sWifiManager.reconnect();
                }
                final boolean result = ok;
                sHandler.post(() -> nativeOnConnectionResult(result, result ? ssidForCallback : null));
            } catch (Exception e) {
                Log.e(TAG, "connect", e);
                sHandler.post(() -> nativeOnConnectionResult(false, null));
            }
        }).start();
    }

    /**
     * 断开当前 WiFi：先禁用再 disconnect、移除配置。若系统不允许（如非本应用添加的网络），
     * 则打开系统 WiFi 设置并提示用户手动断开。
     */
    public static void disconnect() {
        if (sWifiManager == null || sContext == null) return;
        Runnable run = () -> {
            try {
                WifiInfo info = sWifiManager.getConnectionInfo();
                int netId = (info != null) ? info.getNetworkId() : -1;
                if (netId != -1) {
                    boolean okDisable = sWifiManager.disableNetwork(netId);
                    sWifiManager.disconnect();
                    boolean okRemove = sWifiManager.removeNetwork(netId);
                    if (!okDisable && !okRemove) {
                        Intent intent = new Intent(Settings.ACTION_WIRELESS_SETTINGS);
                        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        sContext.startActivity(intent);
                        Toast.makeText(sContext, R.string.wifi_disconnect_open_settings, Toast.LENGTH_LONG).show();
                    }
                } else {
                    sWifiManager.disconnect();
                }
            } catch (Exception e) {
                Log.e(TAG, "disconnect", e);
            }
        };
        if (Looper.myLooper() == Looper.getMainLooper()) {
            run.run();
        } else {
            sHandler.post(run);
        }
    }

    public static String getConnectedSsid() {
        if (sWifiManager == null) return "";
        try {
            WifiInfo info = sWifiManager.getConnectionInfo();
            if (info != null && info.getNetworkId() != -1) {
                String ssid = info.getSSID();
                if (ssid != null && (ssid.startsWith("\"") && ssid.endsWith("\""))) {
                    ssid = ssid.substring(1, ssid.length() - 1);
                }
                return ssid != null ? ssid : "";
            }
        } catch (SecurityException e) {
            Log.w(TAG, "getConnectionInfo permission", e);
        }
        return "";
    }

    // ----- Native 方法 -----

    public static native boolean nativeSetWiFiEnabled(boolean enabled);
    public static native boolean nativeIsWiFiEnabled();
    public static native void nativeOnScanResultsAvailable(String[] ssids, int[] rssis, int[] secured);
    public static native void nativeOnConnectionResult(boolean success, String ssid);
    public static native void nativeRegisterWiFiUtilsClass();
    public static native void nativeSetWiFiUtilsClass(Class<?> wifiUtilsClass);
}
