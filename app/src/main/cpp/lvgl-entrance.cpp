#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <android/native_window.h>

#include "lvgl.h"
#include "lvgl/lvgl.h"
#include "rv_control/ui.h"

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


static lv_disp_draw_buf_t lv_disp_buf;

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
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s,%d,%d", __func__,WIDTH,HEIGHT);
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
Java_com_hybird_lvgl_android_lvgl_LVGLEntrance_nativeCreate(JNIEnv *env, jclass clazz,
                                                            jobject surface) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    window = ANativeWindow_fromSurface(env, surface);
    lv_init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hybird_lvgl_android_lvgl_LVGLEntrance_nativeChanged(JNIEnv *env, jclass clazz,
                                                             jobject surface, jint width,
                                                             jint height) {
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
    if (run) {
        return;
    }

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

    clearScreen();

    ui_init();

    run = true;
    pthread_create(&thread, 0, refresh_task, 0);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_hybird_lvgl_android_lvgl_LVGLEntrance_nativeDestroy(JNIEnv *env, jclass clazz,
                                                             jobject surface) {
    run = false;
    __android_log_print(ANDROID_LOG_ERROR, "LVGL", "func:%s", __func__);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_hybird_lvgl_android_lvgl_LVGLEntrance_nativeTouch(JNIEnv *env, jclass clazz, jint x,
                                                           jint y, jboolean touch) {
    state.x = x;
    state.y = y;
    state.is_touched = touch;
}

#ifdef __cplusplus
} /*extern "C"*/
#endif