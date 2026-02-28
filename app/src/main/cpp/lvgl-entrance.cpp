#include <jni.h>
#include <string>
#include <cstring>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <android/native_window.h>

#include "lvgl.h"
#include "lvgl/lvgl.h"
#include "rv_control/ui.h"
#include "rv_control/wifi_utils.h"
#include "rv_control/screens/ui_Screen4.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TouchState {
    int32_t x;
    int32_t y;
    bool is_touched;
};
static TouchState state;

static int WIDTH = 0;
static int HEIGHT = 0;
//#define DISP_BUF_SIZE 128 * 1024
#define DISP_BUF_SIZE 1920 * 1200

static ANativeWindow *window;
static lv_color_t lv_buf_1[DISP_BUF_SIZE];
static lv_color_t lv_buf_2[DISP_BUF_SIZE];
static pthread_t thread;
static bool run = false;

/* 用于从 C 回调 Java 启动应用（launch_android_app） */
static JavaVM *g_app_jvm = nullptr;
static jclass g_lvgl_entrance_class = nullptr;

static lv_disp_draw_buf_t lv_disp_buf;

extern void ui_init1(void);

//填充屏幕为白色
static void clearScreen() {
    //填充白屏
    ANativeWindow_Buffer buffer;
    ANativeWindow_lock(window, &buffer, 0);
    memset(buffer.bits, 0xff, buffer.stride * buffer.height * 4);
    ANativeWindow_unlockAndPost(window);
}

//
/**
 * 将指定像素长度的颜色转换成RGBA数组
 * @param data 目标数组
 * @param color_p 颜色表
 * @param w 像素大小
 */
static void copy_px(uint8_t *data, lv_color_t *color_p, int w) {
    for (int i = 0; i < w; i++) {
        data[0] = color_p->ch.red;
        data[1] = color_p->ch.green;
        data[2] = color_p->ch.blue;
#if LV_COLOR_DEPTH == 32            
        data[3] = color_p->ch.alpha;
#else
        data[3] = 0xff;
#endif        
        color_p++;
        data += 4;
    }
}

static uint32_t *buf;

static void window_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    // __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s,%d,%d", __func__,WIDTH,HEIGHT);
    int left = area->x1;
    if (left < 0)
        left = 0;
    int right = area->x2 + 1;
    if (right > WIDTH)
        right = WIDTH;
    int top = area->y1;
    if (top < 0)
        top = 0;
    int bottom = area->y2 + 1;
    if (bottom > HEIGHT)
        bottom = HEIGHT;
    int32_t y;

    ANativeWindow_Buffer buffer;
    ANativeWindow_lock(window, &buffer, 0);
    uint32_t *data = (uint32_t *) buffer.bits;
    uint32_t *dest = buf + top * WIDTH + left;
    int w = right - left;
    for (y = top; y < bottom; y++) {
        copy_px((uint8_t *) dest, color_p, w);
        dest += WIDTH;
        color_p += w;
    }
    uint32_t *src = buf;
    for (int i = 0; i < buffer.height; i++) {
        memcpy(data, src, WIDTH * 4);
        src += WIDTH;
        data += buffer.stride;
    }
    ANativeWindow_unlockAndPost(window);
    lv_disp_flush_ready(disp_drv);
}

static void *refresh_task(void *data) {
    while (run) {
        lv_task_handler();
        usleep(1000);
    }
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s. refresh task finished.", __func__);
    return 0;
}

static void LvTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (state.is_touched) {
        data->point.x = state.x;
        data->point.y = state.y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_lvgl_LVGLEntrance_nativeCreate(JNIEnv *env, jclass clazz,
                                                            jobject surface) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    window = ANativeWindow_fromSurface(env, surface);
    lv_init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_lvgl_LVGLEntrance_nativeChanged(JNIEnv *env, jclass clazz,
                                                             jobject surface, jint width,
                                                             jint height) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    if (run) {
        return;
    }
    if (!g_app_jvm) env->GetJavaVM(&g_app_jvm);
    if (!g_lvgl_entrance_class && clazz) g_lvgl_entrance_class = (jclass)env->NewGlobalRef(clazz);

    //显示屏初始化~
    WIDTH = width;
    HEIGHT = height;
    buf = new uint32_t[WIDTH * HEIGHT];

    lv_disp_draw_buf_init(&lv_disp_buf, lv_buf_1, lv_buf_2, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = WIDTH;
    disp_drv.ver_res = HEIGHT;
    disp_drv.draw_buf = &lv_disp_buf;

    // TODO Use acceleration structure for drawing
    disp_drv.flush_cb = window_flush;

    lv_disp_drv_register(&disp_drv);

    //输入设备注册
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = LvTouchRead;
    lv_indev_drv_register(&indev_drv);

    //设置格式
    ANativeWindow_setBuffersGeometry(window, WIDTH, HEIGHT, WINDOW_FORMAT_RGBA_8888);

    // clearScreen();

    ui_init();

    run = true;
    pthread_create(&thread, 0, refresh_task, 0);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_lvgl_LVGLEntrance_nativeDestroy(JNIEnv *env, jclass clazz,
                                                             jobject surface) {
    run = false;
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    lv_indev_delete(lv_indev_get_next(NULL));

    // 删除buf
    if (buf) {
        delete[] buf;
        buf = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_lvgl_LVGLEntrance_nativeTouch(JNIEnv *env, jclass clazz, jint x,
                                                           jint y, jboolean touch) {
    state.x = x;
    state.y = y;
    state.is_touched = touch;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_lvgl_LVGLEntrance_nativeSetRegionIndex(JNIEnv *env, jclass clazz, jint regionIndex) {
    (void)env;
    (void)clazz;
    if (regionIndex < 0) return;
    ui_switch_to_region_deferred((uint16_t)regionIndex);
}

void launch_android_app(const char *package_name) {
    if (!package_name || !g_app_jvm || !g_lvgl_entrance_class) return;
    JNIEnv *env = nullptr;
    jint res = g_app_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED && g_app_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
    if (!env) return;
    jmethodID mid = env->GetStaticMethodID(g_lvgl_entrance_class, "launchApp", "(Ljava/lang/String;)V");
    if (!mid) return;
    jstring jpkg = env->NewStringUTF(package_name);
    if (!jpkg) return;
    env->CallStaticVoidMethod(g_lvgl_entrance_class, mid, jpkg);
    env->DeleteLocalRef(jpkg);
}

void launch_android_app_for_result(const char *package_name, int request_code) {
    if (!package_name || !g_app_jvm || !g_lvgl_entrance_class) return;
    JNIEnv *env = nullptr;
    jint res = g_app_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED && g_app_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
    if (!env) return;
    jmethodID mid = env->GetStaticMethodID(g_lvgl_entrance_class, "launchAppForResult", "(Ljava/lang/String;I)V");
    if (!mid) return;
    jstring jpkg = env->NewStringUTF(package_name);
    if (!jpkg) return;
    env->CallStaticVoidMethod(g_lvgl_entrance_class, mid, jpkg, (jint)request_code);
    env->DeleteLocalRef(jpkg);
}

// WiFi 平台层：由 JNI 调用 Java WiFiUtils
static JavaVM *g_wifi_jvm = nullptr;
static jclass g_wifi_utils_class = nullptr;

static JNIEnv* wifi_get_jni_env() {
    JNIEnv *env = nullptr;
    if (!g_wifi_jvm) return nullptr;
    jint res = g_wifi_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED && g_wifi_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK)
        return nullptr;
    return env;
}

bool wifi_platform_set_enabled(bool enabled) {
    JNIEnv *env = wifi_get_jni_env();
    if (!env || !g_wifi_utils_class) return false;
    jmethodID mid = env->GetStaticMethodID(g_wifi_utils_class, "setWifiEnabled", "(Z)Z");
    if (!mid) return false;
    jboolean ret = env->CallStaticBooleanMethod(g_wifi_utils_class, mid, (jboolean)enabled);
    return (bool)ret;
}

bool wifi_platform_is_enabled(void) {
    JNIEnv *env = wifi_get_jni_env();
    if (!env || !g_wifi_utils_class) return false;
    jmethodID mid = env->GetStaticMethodID(g_wifi_utils_class, "isWifiEnabled", "()Z");
    if (!mid) return false;
    return (bool)env->CallStaticBooleanMethod(g_wifi_utils_class, mid);
}

void wifi_platform_scan(void) {
    JNIEnv *env = wifi_get_jni_env();
    if (!env || !g_wifi_utils_class) return;
    jmethodID mid = env->GetStaticMethodID(g_wifi_utils_class, "startScan", "()V");
    if (!mid) return;
    env->CallStaticVoidMethod(g_wifi_utils_class, mid);
}

void wifi_platform_connect(const char *ssid, const char *password) {
    JNIEnv *env = wifi_get_jni_env();
    if (!env || !g_wifi_utils_class) return;
    jmethodID mid = env->GetStaticMethodID(g_wifi_utils_class, "connect", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (!mid) return;
    jstring jssid = env->NewStringUTF(ssid ? ssid : "");
    jstring jpwd = env->NewStringUTF(password ? password : "");
    env->CallStaticVoidMethod(g_wifi_utils_class, mid, jssid, jpwd);
    if (jssid) env->DeleteLocalRef(jssid);
    if (jpwd) env->DeleteLocalRef(jpwd);
}

void wifi_platform_disconnect(void) {
    JNIEnv *env = wifi_get_jni_env();
    if (!env || !g_wifi_utils_class) return;
    jmethodID mid = env->GetStaticMethodID(g_wifi_utils_class, "disconnect", "()V");
    if (!mid) return;
    env->CallStaticVoidMethod(g_wifi_utils_class, mid);
}

bool wifi_platform_get_connected_ssid(char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return false;
    JNIEnv *env = wifi_get_jni_env();
    if (!env || !g_wifi_utils_class) return false;
    jmethodID mid = env->GetStaticMethodID(g_wifi_utils_class, "getConnectedSsid", "()Ljava/lang/String;");
    if (!mid) return false;
    jstring jstr = (jstring)env->CallStaticObjectMethod(g_wifi_utils_class, mid);
    if (!jstr) { buf[0] = '\0'; return false; }
    const char *utf = env->GetStringUTFChars(jstr, nullptr);
    if (!utf) { env->DeleteLocalRef(jstr); return false; }
    strncpy(buf, utf, buf_size - 1);
    buf[buf_size - 1] = '\0';
    env->ReleaseStringUTFChars(jstr, utf);
    env->DeleteLocalRef(jstr);
    return true;
}

// WiFi相关的NDK接口

// 设置WiFi开关状态
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_android_launcher3_WiFiUtils_nativeSetWiFiEnabled(JNIEnv *env, jclass clazz, jboolean enabled) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    return set_wifi_enabled(enabled);
}

// 检查WiFi是否开启
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_android_launcher3_WiFiUtils_nativeIsWiFiEnabled(JNIEnv *env, jclass clazz) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    return is_wifi_enabled();
}

// 搜索WiFi热点
extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_WiFiUtils_nativeScanWiFiNetworks(JNIEnv *env, jclass clazz) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    // 搜索WiFi热点
    scan_wifi_networks();
}

// 连接到指定的WiFi热点
extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_WiFiUtils_nativeConnectToWiFi(JNIEnv *env, jclass clazz, jstring ssid, jstring password) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    
    // 转换Java字符串为C字符串
    const char *ssid_str = env->GetStringUTFChars(ssid, NULL);
    const char *password_str = env->GetStringUTFChars(password, NULL);
    
    // 连接WiFi
    connect_to_wifi(ssid_str, password_str, NULL);
    
    // 释放字符串资源
    env->ReleaseStringUTFChars(ssid, ssid_str);
    env->ReleaseStringUTFChars(password, password_str);
}

// WiFi扫描结果回调（ssids + rssis + secured）
extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_WiFiUtils_nativeOnScanResultsAvailable(JNIEnv *env, jclass clazz, jobjectArray ssids, jintArray rssis, jintArray secured) {
    if (ssids == nullptr) return;
    jsize length = env->GetArrayLength(ssids);
    if (length <= 0) return;

    const char **c_ssids = (const char **)malloc((size_t)length * sizeof(const char *));
    int *c_rssis = (int *)malloc((size_t)length * sizeof(int));
    int *c_secured = (int *)malloc((size_t)length * sizeof(int));
    if (!c_ssids || !c_rssis || !c_secured) {
        if (c_ssids) free(c_ssids);
        if (c_rssis) free(c_rssis);
        if (c_secured) free(c_secured);
        return;
    }
    for (jsize i = 0; i < length; i++) {
        jstring ssid = (jstring)env->GetObjectArrayElement(ssids, i);
        c_ssids[i] = ssid ? env->GetStringUTFChars(ssid, nullptr) : nullptr;
        if (!c_ssids[i]) c_ssids[i] = "";
    }
    if (rssis && env->GetArrayLength(rssis) >= length) {
        jint *r = env->GetIntArrayElements(rssis, nullptr);
        for (jsize i = 0; i < length; i++) c_rssis[i] = (int)r[i];
        env->ReleaseIntArrayElements(rssis, r, JNI_ABORT);
    } else {
        for (jsize i = 0; i < length; i++) c_rssis[i] = 0;
    }
    if (secured && env->GetArrayLength(secured) >= length) {
        jint *s = env->GetIntArrayElements(secured, nullptr);
        for (jsize i = 0; i < length; i++) c_secured[i] = (int)s[i];
        env->ReleaseIntArrayElements(secured, s, JNI_ABORT);
    } else {
        for (jsize i = 0; i < length; i++) c_secured[i] = 0;
    }
    handle_wifi_scan_results(c_ssids, c_rssis, c_secured, (int)length);
    for (jsize i = 0; i < length; i++) {
        jstring ssid = (jstring)env->GetObjectArrayElement(ssids, i);
        if (ssid && c_ssids[i]) env->ReleaseStringUTFChars(ssid, c_ssids[i]);
    }
    free(c_ssids);
    free(c_rssis);
    free(c_secured);
}

// WiFi连接结果回调（success 时传入 ssid，便于 Connected 行立即显示）
extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_WiFiUtils_nativeOnConnectionResult(JNIEnv *env, jclass clazz, jboolean success, jstring ssid) {
    const char *c_ssid = nullptr;
    if (success && ssid) {
        c_ssid = env->GetStringUTFChars(ssid, nullptr);
    }
    handle_wifi_connect_result((bool)success, c_ssid ? c_ssid : "");
    if (c_ssid)
        env->ReleaseStringUTFChars(ssid, c_ssid);
    if (success)
        ui_Screen4_refresh_wifi_connected_row();
}

// 注册WiFiUtils类到C代码
extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_WiFiUtils_nativeRegisterWiFiUtilsClass(JNIEnv *env, jclass clazz) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    
    // 获取JavaVM实例
    JavaVM *jvm;
    int result = env->GetJavaVM(&jvm);
    if (result != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "LVGL", "Failed to get JavaVM: %d", result);
        return;
    }
    
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "Got JavaVM: %p", jvm);
    
    g_wifi_jvm = jvm;
}

// 设置WiFiUtils类的Class对象
extern "C"
JNIEXPORT void JNICALL
Java_com_android_launcher3_WiFiUtils_nativeSetWiFiUtilsClass(JNIEnv *env, jclass clazz, jobject wifiUtilsClass) {
    if (g_wifi_utils_class) {
        env->DeleteGlobalRef(g_wifi_utils_class);
        g_wifi_utils_class = nullptr;
    }
    JavaVM *jvm = nullptr;
    if (env->GetJavaVM(&jvm) == JNI_OK)
        g_wifi_jvm = jvm;
    if (!wifiUtilsClass) return;
    jclass global_clazz = (jclass)env->NewGlobalRef(wifiUtilsClass);
    if (global_clazz) {
        g_wifi_utils_class = global_clazz;
    }
}

#ifdef __cplusplus
} /*extern "C"*/
#endif