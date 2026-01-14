/**
 * LVGL屏幕亮度步进变化演示
 * 作者：嵌入式助手
 */

 #include "../lvgl/lvgl.h"
 #include <stdlib.h>
 #include <time.h>
 
 /* 全局变量定义 */
 static lv_obj_t *screen;           // 主屏幕
 static lv_obj_t *fullscreen_rect;  // 全屏矩形
 static lv_obj_t *info_label;       // 信息标签
 static lv_timer_t *brightness_timer; // 亮度调整定时器
 
/* 亮度参数 */
static uint8_t current_brightness = 100;   // 当前亮度值 (0-255)
static uint8_t brightness_step = 2;      // 步进值
static uint8_t direction = 1;           // 方向: 1=变亮, 0=变暗
static uint8_t cycle_count = 0;         // 循环计数
static bool is_paused = false;          // 暂停状态
 
 /* 颜色结构体 */
 typedef struct {
     uint8_t r;
     uint8_t g;
     uint8_t b;
 } RGB_Color;
 
 /* 颜色预定义（不同色温） */
 static const RGB_Color color_presets[] = {
     {255, 255, 255},  // 白色
     {255, 200, 150},  // 暖白色
     {200, 255, 200},  // 冷白色
     {255, 255, 200},  // 日光色
 };
 
 static uint8_t current_color_index = 0;  // 当前颜色索引
 
/* 函数声明 */
static void create_ui(void);
static void update_brightness(lv_timer_t *timer);
static RGB_Color calculate_color(void);
static void update_info_label(void);
static void cycle_color(lv_event_t *e);
static void reset_animation(lv_event_t *e);
static void handle_key_event(lv_event_t *e);
void pause_animation(bool pause);
 
 /**
  * @brief 创建用户界面
  */
 static void create_ui(void) {
    /* 创建全屏矩形 */
    fullscreen_rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(fullscreen_rect, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(fullscreen_rect, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(fullscreen_rect, 0, 0);
    lv_obj_set_style_pad_all(fullscreen_rect, 0, 0);
    lv_obj_center(fullscreen_rect);
    
    /* 为全屏矩形添加按键事件处理（用于接收遥控器OK键） */
    lv_obj_add_event_cb(fullscreen_rect, handle_key_event, LV_EVENT_KEY, NULL);
    
    /* 将全屏矩形添加到默认组，以便接收按键事件 */
    lv_group_t *default_group = lv_group_get_default();
    if (default_group == NULL) {
        default_group = lv_group_create();
        lv_group_set_default(default_group);
    }
    lv_group_add_obj(default_group, fullscreen_rect);
    lv_group_focus_obj(fullscreen_rect);
     
     /* 创建信息标签 */
     info_label = lv_label_create(lv_scr_act());
     lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
     lv_obj_set_style_text_font(info_label, &lv_font_montserrat_20, 0);
     lv_obj_set_style_bg_opa(info_label, LV_OPA_50, 0);
     lv_obj_set_style_pad_all(info_label, 10, 0);
     lv_obj_align(info_label, LV_ALIGN_TOP_MID, 0, 20);
     
     #if 0
     /* 创建控制按钮容器 */
     lv_obj_t *btn_container = lv_obj_create(lv_scr_act());
     lv_obj_set_size(btn_container, 200, 60);
     lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -20);
     lv_obj_set_style_bg_opa(btn_container, LV_OPA_50, 0);
     lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
     lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, 
                           LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
     
     /* 创建控制按钮 */
     lv_obj_t *color_btn = lv_btn_create(btn_container);
     lv_obj_t *color_label = lv_label_create(color_btn);
     lv_label_set_text(color_label, "change color");
     lv_obj_set_width(color_btn, 80);
     lv_obj_add_event_cb(color_btn, 
                        (lv_event_cb_t)cycle_color, 
                        LV_EVENT_CLICKED, 
                        NULL);
     
     lv_obj_t *reset_btn = lv_btn_create(btn_container);
     lv_obj_t *reset_label = lv_label_create(reset_btn);
     lv_label_set_text(reset_label, "reset");
     lv_obj_set_width(reset_btn, 80);
     lv_obj_add_event_cb(reset_btn, 
                        (lv_event_cb_t)reset_animation, 
                        LV_EVENT_CLICKED, 
                        NULL);
     #endif
 }
 
 /**
  * @brief 计算当前颜色
  * @return RGB_Color 计算后的颜色
  */
 static RGB_Color calculate_color(void) {
     RGB_Color base_color = color_presets[current_color_index];
     RGB_Color current_color;
     
     /* 根据亮度值调整颜色 */
     current_color.r = (base_color.r * current_brightness) / 255;
     current_color.g = (base_color.g * current_brightness) / 255;
     current_color.b = (base_color.b * current_brightness) / 255;
     
     return current_color;
 }
 
 /**
  * @brief 更新信息标签
  */
 static void update_info_label(void) {
     static char info_text[128];
     
     /* 获取当前颜色名称 */
     const char *color_names[] = {"white", "warm white", "cold white", "daylight"};
     const char *color_name = color_names[current_color_index];
     
     /* 计算亮度百分比 */
     uint8_t brightness_percent = (current_brightness * 100) / 255;
     
    /* 更新标签文本 */
    snprintf(info_text, sizeof(info_text),
             "brightness: %d%%\n"
             "brightness value: %d\n"
             "color: %s\n"
             "cycle: %d\n"
             "status: %s",
             brightness_percent,
             current_brightness,
             color_name,
             cycle_count,
             is_paused ? "PAUSED" : "RUNNING");
    
    lv_label_set_text(info_label, info_text);
 }
 
 /**
  * @brief 更新亮度（定时器回调）
  * @param timer 定时器指针
  */
 static void update_brightness(lv_timer_t *timer) {
     /* 更新亮度值 */
     if (direction == 1) {
         // 变亮
         if (current_brightness + brightness_step > 255) {
             current_brightness = 0;
//             direction = 0;  // 切换为变暗
             cycle_count++;  // 增加循环计数
         } else {
             current_brightness += brightness_step;
         }
     } else {
         // 变暗
         if (current_brightness < brightness_step) {
             current_brightness = 0;
             direction = 1;  // 切换为变亮
         } else {
             current_brightness -= brightness_step;
         }
     }
     
     /* 计算新颜色 */
     RGB_Color new_color = calculate_color();
     lv_color_t lv_color = lv_color_make(new_color.r, new_color.g, new_color.b);
     
     /* 更新屏幕颜色 */
     lv_obj_set_style_bg_color(fullscreen_rect, lv_color, 0);
     
     /* 更新信息标签 */
     update_info_label();
 }
 
 /**
  * @brief 切换颜色
  */
 static void cycle_color(lv_event_t *e) {
     LV_UNUSED(e);
     
     /* 循环选择下一个颜色 */
     current_color_index = (current_color_index + 1) % 
                          (sizeof(color_presets) / sizeof(color_presets[0]));
     
     /* 立即更新颜色 */
     RGB_Color new_color = calculate_color();
     lv_color_t lv_color = lv_color_make(new_color.r, new_color.g, new_color.b);
     lv_obj_set_style_bg_color(fullscreen_rect, lv_color, 0);
     
     update_info_label();
 }
 
/**
 * @brief 重置动画
 */
static void reset_animation(lv_event_t *e) {
    LV_UNUSED(e);
    
    current_brightness = 0;
    direction = 1;
    cycle_count = 0;
    
    /* 立即更新显示 */
    RGB_Color new_color = calculate_color();
    lv_color_t lv_color = lv_color_make(new_color.r, new_color.g, new_color.b);
    lv_obj_set_style_bg_color(fullscreen_rect, lv_color, 0);
    
    update_info_label();
}

/**
 * @brief 处理按键事件（遥控器OK键）
 */
static void handle_key_event(lv_event_t *e) {
    uint32_t key = lv_event_get_key(e);
    
    /* 检查是否为OK键（ENTER键） */
    if (key == LV_KEY_ENTER) {
        /* 切换暂停/恢复状态 */
        is_paused = !is_paused;
        pause_animation(is_paused);
        
        /* 更新信息标签以显示新状态 */
        update_info_label();
    }
}
 
 /**
  * @brief 初始化应用
  */
 void brightness_demo_init(void) {
     /* 创建UI */
     create_ui();
     
     /* 设置初始颜色（黑色） */
     lv_obj_set_style_bg_color(fullscreen_rect, lv_color_black(), 0);
     
     /* 更新信息标签 */
     update_info_label();
     
     /* 创建亮度调整定时器（50ms更新一次） */
     brightness_timer = lv_timer_create(update_brightness, 1000, NULL);
     lv_timer_set_repeat_count(brightness_timer, -1);  // 无限循环
 }
 
 /**
  * @brief 清理应用资源
  */
 void brightness_demo_deinit(void) {
     if (brightness_timer) {
         lv_timer_del(brightness_timer);
         brightness_timer = NULL;
     }
     
     lv_obj_clean(lv_scr_act());
 }
 
 /**
  * @brief 主应用入口
  */
 void ui_init1(void) {
     /* 初始化LVGL（假设已经完成） */
     // lv_init();
     // lv_disp_init();
     
     /* 启动亮度演示 */
     brightness_demo_init();
 }
 
 /* 可选的配置选项 */
 void set_brightness_step(uint8_t step) {
     if (step > 0 && step <= 255) {
         brightness_step = step;
     }
 }
 
 uint8_t get_current_brightness(void) {
     return current_brightness;
 }
 
 uint8_t get_cycle_count(void) {
     return cycle_count;
 }
 
 void pause_animation(bool pause) {
     if (brightness_timer) {
         if (pause) {
             lv_timer_pause(brightness_timer);
         } else {
             lv_timer_resume(brightness_timer);
         }
     }
 }