//
// Created by jayyu on 2022/3/29.
//

#include "ui.h"

void gui_init() {
    lv_example_btn_1();
}

void event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Clicked");
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        LV_LOG_USER("Toggled");
    }
}

void lv_example_btn_1(void) {
    lv_obj_t *label;

    static lv_style_t style_sel;
    lv_style_init(&style_sel);
    lv_style_set_text_font(&style_sel, &lv_font_montserrat_44);

    lv_obj_t *btn1 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -220);
    lv_obj_set_size(btn1, 320, 120);

    label = lv_label_create(btn1);
    lv_obj_add_style(label, &style_sel, 0);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);

    lv_obj_t *btn2 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_size(btn2, 320, 120);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
//    lv_obj_set_height(btn2, LV_SIZE_CONTENT);


    label = lv_label_create(btn2);
    lv_obj_add_style(label, &style_sel, 0);
    lv_label_set_text(label, "Toggle");
    lv_obj_center(label);

}

