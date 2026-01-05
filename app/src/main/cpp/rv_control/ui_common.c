#include "ui.h"


typedef void (*ui_screen_init_t)(void);

lv_obj_t * ui_Screen[] = {&ui_Screen1,&ui_Screen2,&ui_Screen3,&ui_Screen4,&ui_Screen5,&ui_Screen6,&ui_Screen7,&ui_Screen8};

ui_screen_init_t ui_Screen_init_cb[] = {&ui_Screen1_screen_init,&ui_Screen2_screen_init,&ui_Screen3_screen_init,&ui_Screen4_screen_init,&ui_Screen5_screen_init,&ui_Screen6_screen_init,&ui_Screen7_screen_init,&ui_Screen8_screen_init}; 

void _ui_common_screen_change(uint16_t cur_screen_id, lv_event_t * e)
{
    if(cur_screen_id >= 8)
        return;

    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code != LV_EVENT_CLICKED) 
        return;
    
    // 获取屏幕坐标,如果点击不在矩形范围内,则不处理 
    lv_obj_t * cont = lv_event_get_target(e);
    lv_indev_t * indev = lv_indev_get_act(); // 获取输入设备
    if (indev == NULL) 
        return;

    lv_point_t point;
    lv_indev_get_point(indev, &point); // 获取触摸点坐标
    
    if(point.x < 0 || point.x > 1919 || point.y < 980 || point.y > 1199) 
        return;

    uint16_t screen_id =  point.x / (1920 / 8); // 8个屏幕等分
    if(screen_id == cur_screen_id)
        return;

    _ui_screen_change(ui_Screen[screen_id], LV_SCR_LOAD_ANIM_NONE, 50, 0, ui_Screen_init_cb[screen_id]);

}