#include "ui.h"

typedef void (*ui_screen_init_t)(void);

// 底部导航栏图标
static lv_obj_t *ui_nav_icons[NAV_ICON_NUM] = {NULL};
static lv_obj_t *highlight[NAV_ICON_NUM] = {NULL};

lv_obj_t *ui_Screen[] = {&ui_Screen1, &ui_Screen2, &ui_Screen3, &ui_Screen4, &ui_Screen5, &ui_Screen6, &ui_Screen7, &ui_Screen8};

ui_screen_init_t ui_Screen_init_cb[] = {&ui_Screen1_screen_init, &ui_Screen2_screen_init, &ui_Screen3_screen_init, &ui_Screen4_screen_init, &ui_Screen5_screen_init, &ui_Screen6_screen_init, &ui_Screen7_screen_init, &ui_Screen8_screen_init};


void bar_navi_event(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    
    if (event_code != LV_EVENT_CLICKED)
        return;
    
    lv_obj_t *target = lv_event_get_target(e);
    int index = (int)lv_obj_get_user_data(target);
    
    if (index >= 0 && index < NAV_ICON_NUM)
    {
        _ui_screen_change(ui_Screen[index], LV_SCR_LOAD_ANIM_NONE, 50, 0, ui_Screen_init_cb[index]);
    }
}

// 判断导航栏的项是否为选中项
static bool is_navi_index_selected(uint8_t index, lv_obj_t *parent)
{
    if (index >= NAV_ICON_NUM)
        return false;

    lv_obj_t **target_screen = ui_Screen[index];

    if(parent == *target_screen)
        return true;
    else
        return false;
}


void ui_draw_navigation_bar(lv_obj_t *parent)
{
    // 创建底部导航栏（8个图标）
    int nav_y = 980;
    int nav_icon_width = 1920 / 8;
    int nav_icon_size = 190;
    const lv_img_dsc_t *dsc;
    lv_color_t bg_color;

    // 导航图标资源数组（需要根据实际资源调整）
    const lv_img_dsc_t *nav_icons[] = {
        &ui_img_homepage_sel_png,  // 温度/湿度
        &ui_img_charger_sel_png,   // 充电
        &ui_img_water_box_sel_png, // 水箱
        &ui_img_setting_sel_png,   // 设置
        &ui_img_balance_sel_png,   // 平衡
        &ui_img_device_sel_png,    // 设备
        &ui_img_light_sel_png,     // 灯光（当前页面，高亮）
        &ui_img_camera_sel_png     // 摄像头
    };

    const lv_img_dsc_t *nav_icons_sel[] = {
        &ui_img_homepage_unsel_png,  // 温度/湿度
        &ui_img_charger_unsel_png,   // 充电
        &ui_img_water_box_unsel_png, // 水箱
        &ui_img_setting_unsel_png,   // 设置
        &ui_img_balance_unsel_png,   // 平衡
        &ui_img_device_unsel_png,    // 设备
        &ui_img_light_unsel_png,     // 灯光（当前页面，高亮）
        &ui_img_camera_unsel_png     // 摄像头
    };    

    for (int i = 0; i < NAV_ICON_NUM; i++)
    {
        if (is_navi_index_selected(i, parent))
        {
            dsc = nav_icons_sel[i];
            bg_color = lv_color_hex(0x189FBF);
        }
        else
        {
            dsc = nav_icons[i];
            bg_color = lv_color_hex(0x141432);
        }

        highlight[i] = lv_obj_create(parent);
        lv_obj_set_user_data(highlight[i], (void*)i);
        lv_obj_clear_flag(highlight[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(highlight[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(highlight[i], nav_icon_width - 18, nav_icon_size);
        lv_obj_set_pos(highlight[i], i * nav_icon_width + 9, nav_y);
        lv_obj_set_style_bg_color(highlight[i], bg_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(highlight[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(highlight[i], 20, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_background(highlight[i]);
        /* 去掉可见边框 */
        lv_obj_set_style_border_width(highlight[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(highlight[i], LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(highlight[i], LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_add_event_cb(highlight[i], bar_navi_event, LV_EVENT_ALL, NULL);

        ui_nav_icons[i] = lv_img_create(highlight[i]);
        lv_img_set_src(ui_nav_icons[i], dsc);

        /* 计算缩放因子并应用 zoom（256 = 1.0）以按比例缩放大图像 */      
        int img_w = dsc->header.w;
        int img_h = dsc->header.h;
        int padding = 24; /* 每边留白 */
        int max_w = nav_icon_width - padding * 2;
        int max_h = nav_icon_size - padding * 2;
        float scale_w = (float)max_w / (float)img_w;
        float scale_h = (float)max_h / (float)img_h;
        float scale = scale_w < scale_h ? scale_w : scale_h;
        if (scale > 1.0f)
            scale = 1.0f; /* 不放大，只有缩小 */
        uint16_t zoom = (uint16_t)(scale * 256.0f);
        lv_img_set_zoom(ui_nav_icons[i], zoom);

        /* 将图标在槽内居中显示 */
        lv_obj_set_align(ui_nav_icons[i], LV_ALIGN_CENTER);
        /* 禁用图标的滚动响应，但保留可点击性 */
        lv_obj_clear_flag(ui_nav_icons[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(ui_nav_icons[i], LV_OBJ_FLAG_ADV_HITTEST); /// Flags
        lv_obj_clear_flag(ui_nav_icons[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(ui_nav_icons[i], LV_OBJ_FLAG_EVENT_BUBBLE);
    }
}

void ui_navigation_bar_destroy(void)
{
    for (int i = 0; i < NAV_ICON_NUM; i++)
    {
        if (ui_nav_icons[i])
        {
            lv_obj_del(ui_nav_icons[i]);
            ui_nav_icons[i] = NULL;
            lv_obj_del(highlight[i]);
            highlight[i] = NULL;
        }
    }
}