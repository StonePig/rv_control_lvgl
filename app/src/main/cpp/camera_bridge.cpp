#include <jni.h>
#include <stdint.h>
#include <android/log.h>
#include <string.h>

extern "C" void ui_Screen8_update_camera_frame(const uint8_t *rgba, int w, int h);

static JavaVM *g_vm = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

// Called from Java when a new RGBA frame is available (byte[])
extern "C"
JNIEXPORT void JNICALL
Java_com_hybird_lvgl_android_lvgl_LVGLEntrance_nativeCameraFrame(JNIEnv *env, jclass clazz, jbyteArray data, jint w, jint h) {
    if (!data) return;
    jsize len = env->GetArrayLength(data);
    jbyte *buf = env->GetByteArrayElements(data, NULL);
    if (buf && len > 0) {
        ui_Screen8_update_camera_frame((const uint8_t*)buf, (int)w, (int)h);
    }
    if (buf) env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
}

// Request Java MainActivity to start/stop camera
extern "C" void camera_bridge_request_start(void)
{
    if (!g_vm) return;
    JNIEnv *env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, NULL) != JNI_OK) return;
    }

    jclass cls = env->FindClass("com/hybird/lvgl/android/MainActivity");
    if (!cls) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "MainActivity class not found");
        return;
    }
    jmethodID mid = env->GetStaticMethodID(cls, "startCamera", "()V");
    if (!mid) {
        __android_log_print(ANDROID_LOG_ERROR, "camera_bridge", "startCamera method not found");
        return;
    }
    env->CallStaticVoidMethod(cls, mid);
}

extern "C" void camera_bridge_request_stop(void)
{
    if (!g_vm) return;
    JNIEnv *env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, NULL) != JNI_OK) return;
    }

    jclass cls = env->FindClass("com/hybird/lvgl/android/MainActivity");
    if (!cls) return;
    jmethodID mid = env->GetStaticMethodID(cls, "stopCamera", "()V");
    if (!mid) return;
    env->CallStaticVoidMethod(cls, mid);
}
