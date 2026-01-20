#include <jni.h>
#include <stdint.h>
#include <android/log.h>
#include <string.h>

extern "C" void ui_Screen8_update_camera_frame(const uint8_t *rgba, int w, int h, int cam_index);

static JavaVM *g_vm = nullptr;
static jclass g_mainActivityClass = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_vm = vm;
    
    // Get JNIEnv for initialization
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "Failed to get JNIEnv in JNI_OnLoad");
        return JNI_ERR;
    }
    
    // Find MainActivity class and create global reference
    jclass localClass = env->FindClass("com/hybird/lvgl/android/MainActivity");
    if (!localClass) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "Failed to find MainActivity class in JNI_OnLoad");
        return JNI_ERR;
    }
    
    // Create global reference to the class
    g_mainActivityClass = (jclass)env->NewGlobalRef(localClass);
    if (!g_mainActivityClass) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "Failed to create global reference for MainActivity class");
        return JNI_ERR;
    }
    
    return JNI_VERSION_1_6;
}

// Called from Java when a new RGBA frame is available (byte[])
extern "C"
JNIEXPORT void JNICALL
Java_com_hybird_lvgl_android_lvgl_LVGLEntrance_nativeCameraFrame(JNIEnv *env, jclass clazz, jbyteArray data, jint w, jint h, jint cam_index) {
    if (!data) return;
    jsize len = env->GetArrayLength(data);
    jbyte *buf = env->GetByteArrayElements(data, NULL);
    if (buf && len > 0) {
        ui_Screen8_update_camera_frame((const uint8_t*)buf, (int)w, (int)h, (int)cam_index);
    }
    if (buf) env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
}

// Request Java MainActivity to start/stop camera
// Helper function to get JNIEnv and check if we need to attach to thread
static JNIEnv* get_jni_env() {
    if (!g_vm) return nullptr;
    JNIEnv *env = nullptr;
    jint result = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "Failed to attach current thread");
            return nullptr;
        }
    } else if (result != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "Failed to get JNIEnv: %d", result);
        return nullptr;
    }
    return env;
}

extern "C" void camera_bridge_request_start(int index)
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "startCamera", "(I)V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "startCamera method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid, (jint)index);
}

extern "C" void camera_bridge_request_stop(int index)
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "stopCamera", "(I)V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "stopCamera method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid, (jint)index);
}

// Called from C++ to show all SurfaceViews
extern "C" void camera_bridge_show_surface_views()
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "showSurfaceViews", "()V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "showSurfaceViews method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid);
}

// Called from C++ to hide all SurfaceViews
extern "C" void camera_bridge_hide_surface_views()
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "hideSurfaceViews", "()V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "hideSurfaceViews method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid);
}

// Called from C++ to set SurfaceView parameters (position, size)
extern "C" void camera_bridge_set_surface_view_params(int index, int x, int y, int width, int height)
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "setSurfaceViewParams", "(IIIII)V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "setSurfaceViewParams method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid, (jint)index, (jint)x, (jint)y, (jint)width, (jint)height);
}

// Called from C++ to set SurfaceView visibility
extern "C" void camera_bridge_set_surface_view_visibility(int index, bool visible)
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "setSurfaceViewVisibility", "(IZ)V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "setSurfaceViewVisibility method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid, (jint)index, (jboolean)visible);
}

// Called from C++ to reset all SurfaceViews to initial state
extern "C" void camera_bridge_reset_surface_views()
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "resetSurfaceViews", "()V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "resetSurfaceViews method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid);
}

// Called from C++ to set main preview SurfaceView parameters (position, size)
extern "C" void camera_bridge_set_main_surface_view_params(int x, int y, int width, int height)
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "setMainSurfaceViewParams", "(IIII)V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "setMainSurfaceViewParams method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid, (jint)x, (jint)y, (jint)width, (jint)height);
}

// Called from C++ to set current main camera index
extern "C" void camera_bridge_set_current_main_camera_index(int index)
{
    JNIEnv *env = get_jni_env();
    if (!env || !g_mainActivityClass) return;
    
    jmethodID mid = env->GetStaticMethodID(g_mainActivityClass, "setCurrentMainCameraIndex", "(I)V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "setCurrentMainCameraIndex method not found");
        return;
    }
    env->CallStaticVoidMethod(g_mainActivityClass, mid, (jint)index);
}
