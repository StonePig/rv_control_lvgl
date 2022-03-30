//
// Created by jayyu on 2022/3/29.
//

#ifndef LVGL_ANDROID_UI_H
#define LVGL_ANDROID_UI_H

#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <android/native_window.h>

#include "lvgl.h"
#include "lvgl/lvgl.h"

void gui_init();

void event_handler(lv_event_t *e);

void lv_example_btn_1(void);


#endif //LVGL_ANDROID_UI_H
