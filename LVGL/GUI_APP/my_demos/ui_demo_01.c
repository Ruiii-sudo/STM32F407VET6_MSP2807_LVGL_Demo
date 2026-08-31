/**
  ******************************************************************************
  * @file    ui_demo_01.c
  * @brief   传感器数据监控UI - 三页结构（展示/设置/设备）+ 底部Tab栏 + 左右滑动
  ******************************************************************************
  */
#include "ui_demo_01.h"
#include "sensor_data.h"
#include "user_task.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

/*********************
 * 颜色定义（奶油风）
 *********************/
#define COLOR_BG        lv_color_hex(0xFFF5E6)   /* 奶油底色 */
#define COLOR_CARD      lv_color_hex(0xFFFFFF)   /* 纯白卡片 */
#define COLOR_TITLEBAR  lv_color_hex(0xF0E0CC)   /* 奶茶色标题栏 */
#define COLOR_TABBAR    lv_color_hex(0xFAEBD7)   /* Tab栏浅奶油 */
#define COLOR_TAB_ACTIVE lv_color_hex(0xE8D5C4)  /* Tab选中色 */
#define COLOR_TEXT      lv_color_hex(0x5C4033)    /* 深棕主文字 */
#define COLOR_TEXT_DIM  lv_color_hex(0xA0896E)    /* 中棕次要文字 */

/* 四个传感器主题色（奶油风柔和色） */
#define COLOR_TEMP      lv_color_hex(0xE8956B)   /* 温度-暖橙 */
#define COLOR_HUMI      lv_color_hex(0x7FB3D5)   /* 湿度-柔蓝 */
#define COLOR_LIGHT     lv_color_hex(0xE8B84B)   /* 光照-暖黄 */
#define COLOR_GAS       lv_color_hex(0x8FB996)    /* 气体-柔绿 */

/*********************
 * 页面枚举
 *********************/
typedef enum {
    PAGE_DISPLAY = 0,   /* 展示页 */
    PAGE_SETTINGS,       /* 设置页 */
    PAGE_DEVICE,         /* 设备详情页 */
    PAGE_COUNT
} PageIndex_t;

/*********************
 * 控件句柄
 *********************/
static lv_obj_t *s_screens[PAGE_COUNT];   /* 三个页面 */
static PageIndex_t s_current_page = PAGE_DISPLAY;

/* 展示页控件 */
static lv_obj_t *s_card_value_labels[SENSOR_COUNT];
static lv_obj_t *s_detail_chart;
static lv_chart_series_t *s_detail_series;
static lv_obj_t *s_detail_cur_label;
static lv_obj_t *s_detail_max_label;
static lv_obj_t *s_detail_min_label;
static lv_obj_t *s_detail_avg_label;
static lv_obj_t *s_y_labels[5];
static SensorType_t s_current_sensor = SENSOR_TEMP;
static lv_obj_t *s_detail_screen;  /* 传感器详情弹窗页 */
static lv_obj_t *s_detail_title_label; /* 详情页标题 */
static lv_obj_t *s_pause_btn;          /* 暂停/继续按钮 */
static lv_obj_t *s_pause_btn_label;    /* 暂停按钮文字 */
static bool s_sensor_paused[SENSOR_COUNT] = {false, false, false, false}; /* 各传感器暂停状态 */

/* 设置页开关状态 */
static bool s_setting_auto_refresh = true;
static bool s_setting_upload = false;

/*********************
 * 锁屏界面
 *********************/
static lv_obj_t *s_lock_screen = NULL;
static lv_obj_t *s_lock_slider = NULL;
static lv_obj_t *s_lock_percent_label = NULL;
static bool s_is_locked = false;

/* 锁屏配色（深奶茶色，和主界面形成对比） */
#define COLOR_LOCK_BG         lv_color_hex(0xF5E6D3)   /* 奶茶色背景（比主界面稍深，有层次） */
#define COLOR_LOCK_TEXT       lv_color_hex(0x5C4033)   /* 深棕文字（和主界面一致） */
#define COLOR_LOCK_HINT       lv_color_hex(0xA0896E)   /* 中棕提示（和主界面一致） */
#define COLOR_LOCK_TRACK      lv_color_hex(0xE0D0BC)   /* 滑块轨道（浅棕） */
#define COLOR_LOCK_INDICATOR  lv_color_hex(0xE8956B)   /* 滑块已滑部分（橙色，保持不变） */
#define COLOR_LOCK_KNOB       lv_color_hex(0xFFFFFF)   /* 滑块按钮（白色） */


static lv_style_t s_style_lock_bg;
static lv_style_t s_style_lock_title;
static lv_style_t s_style_lock_hint;
static lv_style_t s_style_lock_slider_main;
static lv_style_t s_style_lock_slider_indicator;
static lv_style_t s_style_lock_slider_knob;


/*********************
 * 样式
 *********************/
static lv_style_t s_style_bg;
static lv_style_t s_style_card;
static lv_style_t s_style_titlebar;
static lv_style_t s_style_title_text;
static lv_style_t s_style_name_text;
static lv_style_t s_style_btn;
static lv_style_t s_style_chart;
static lv_style_t s_style_tabbar;
static lv_style_t s_style_tab_btn;
static lv_style_t s_style_tab_btn_active;

/*********************
 * 函数前置声明
 *********************/
static void init_styles(void);
static lv_color_t get_sensor_color(SensorType_t type);
static void create_display_page(lv_obj_t *parent);
static void create_settings_page(lv_obj_t *parent);
static void create_device_page(lv_obj_t *parent);
static lv_obj_t *create_titlebar(lv_obj_t *parent, const char *title);
static lv_obj_t *create_tabbar(lv_obj_t *parent, PageIndex_t active);
static void tab_event_cb(lv_event_t *e);
static void tabbar_gesture_cb(lv_event_t *e);
//static void gesture_event_cb(lv_event_t *e);
static void switch_to_page(PageIndex_t page, bool slide_left);
static void card_event_cb(lv_event_t *e);
static void back_from_detail_cb(lv_event_t *e);
static void pause_btn_event_cb(lv_event_t *e);
static void update_detail_chart(void);
static void update_detail_stats(void);
static void refresh_display_cards(void);
static void setting_switch_cb(lv_event_t *e);

/* 锁屏相关 */
static void create_lock_screen(void);
static void lock_slider_event_cb(lv_event_t *e);
static void unlock_screen(void);
static void setting_lock_btn_cb(lv_event_t *e);


/*********************
 * 初始化样式
 *********************/
static void init_styles(void)
{
    lv_style_init(&s_style_bg);
    lv_style_set_bg_color(&s_style_bg, COLOR_BG);
    lv_style_set_bg_opa(&s_style_bg, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_bg, 0);
    lv_style_set_pad_all(&s_style_bg, 0);
    lv_style_set_radius(&s_style_bg, 0);

    lv_style_init(&s_style_card);
    lv_style_set_bg_color(&s_style_card, COLOR_CARD);
    lv_style_set_bg_opa(&s_style_card, LV_OPA_COVER);
    lv_style_set_radius(&s_style_card, 12);
    lv_style_set_border_width(&s_style_card, 0);
    lv_style_set_pad_all(&s_style_card, 8);
    lv_style_set_shadow_width(&s_style_card, 8);
    lv_style_set_shadow_color(&s_style_card, lv_color_hex(0xD4B896));
    lv_style_set_shadow_opa(&s_style_card, LV_OPA_30);
    lv_style_set_shadow_ofs_y(&s_style_card, 2);

    lv_style_init(&s_style_titlebar);
    lv_style_set_bg_color(&s_style_titlebar, COLOR_TITLEBAR);
    lv_style_set_bg_opa(&s_style_titlebar, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_titlebar, 0);
    lv_style_set_pad_all(&s_style_titlebar, 0);
    lv_style_set_radius(&s_style_titlebar, 0);

    lv_style_init(&s_style_title_text);
    lv_style_set_text_color(&s_style_title_text, COLOR_TEXT);
    lv_style_set_text_font(&s_style_title_text, &lv_font_montserrat_16);
    lv_style_set_text_align(&s_style_title_text, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&s_style_name_text);
    lv_style_set_text_color(&s_style_name_text, COLOR_TEXT_DIM);
    lv_style_set_text_font(&s_style_name_text, &lv_font_montserrat_14);
    lv_style_set_text_align(&s_style_name_text, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&s_style_btn);
    lv_style_set_bg_color(&s_style_btn, COLOR_TITLEBAR);
    lv_style_set_bg_opa(&s_style_btn, LV_OPA_COVER);
    lv_style_set_radius(&s_style_btn, 8);
    lv_style_set_border_width(&s_style_btn, 1);
    lv_style_set_border_color(&s_style_btn, lv_color_hex(0xD4B896));
    lv_style_set_text_color(&s_style_btn, COLOR_TEXT);
    lv_style_set_text_font(&s_style_btn, &lv_font_montserrat_14);
    lv_style_set_pad_all(&s_style_btn, 6);

    lv_style_init(&s_style_chart);
    lv_style_set_bg_color(&s_style_chart, COLOR_CARD);
    lv_style_set_bg_opa(&s_style_chart, LV_OPA_COVER);
    lv_style_set_radius(&s_style_chart, 10);
    lv_style_set_border_width(&s_style_chart, 0);
    lv_style_set_pad_all(&s_style_chart, 8);
    lv_style_set_text_color(&s_style_chart, COLOR_TEXT_DIM);
    lv_style_set_line_color(&s_style_chart, lv_color_hex(0xE8DCC8));
    lv_style_set_line_width(&s_style_chart, 1);

    /* 底部Tab栏样式 */
    lv_style_init(&s_style_tabbar);
    lv_style_set_bg_color(&s_style_tabbar, COLOR_TABBAR);
    lv_style_set_bg_opa(&s_style_tabbar, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_tabbar, 0);
    lv_style_set_pad_all(&s_style_tabbar, 0);
    lv_style_set_radius(&s_style_tabbar, 0);

    lv_style_init(&s_style_tab_btn);
    lv_style_set_bg_color(&s_style_tab_btn, COLOR_TABBAR);
    lv_style_set_bg_opa(&s_style_tab_btn, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_style_tab_btn, 0);
    lv_style_set_radius(&s_style_tab_btn, 0);
    lv_style_set_text_color(&s_style_tab_btn, COLOR_TEXT_DIM);
    lv_style_set_text_font(&s_style_tab_btn, &lv_font_montserrat_12);
    lv_style_set_pad_all(&s_style_tab_btn, 0);

    lv_style_init(&s_style_tab_btn_active);
    lv_style_set_bg_color(&s_style_tab_btn_active, COLOR_TAB_ACTIVE);
    lv_style_set_bg_opa(&s_style_tab_btn_active, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_tab_btn_active, 0);
    lv_style_set_radius(&s_style_tab_btn_active, 8);
    lv_style_set_text_color(&s_style_tab_btn_active, COLOR_TEXT);
    lv_style_set_text_font(&s_style_tab_btn_active, &lv_font_montserrat_12);
    lv_style_set_pad_all(&s_style_tab_btn_active, 0);
	
	/* 锁屏样式 */
    lv_style_init(&s_style_lock_bg);
    lv_style_set_bg_color(&s_style_lock_bg, COLOR_LOCK_BG);
    lv_style_set_bg_opa(&s_style_lock_bg, LV_OPA_COVER);
    lv_style_set_border_width(&s_style_lock_bg, 0);
    lv_style_set_pad_all(&s_style_lock_bg, 0);
    lv_style_set_radius(&s_style_lock_bg, 0);

    lv_style_init(&s_style_lock_title);
    lv_style_set_text_color(&s_style_lock_title, COLOR_LOCK_TEXT);
    lv_style_set_text_font(&s_style_lock_title, &lv_font_montserrat_20);
    lv_style_set_text_align(&s_style_lock_title, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&s_style_lock_hint);
    lv_style_set_text_color(&s_style_lock_hint, COLOR_LOCK_HINT);
    lv_style_set_text_font(&s_style_lock_hint, &lv_font_montserrat_14);
    lv_style_set_text_align(&s_style_lock_hint, LV_TEXT_ALIGN_CENTER);

    /* 滑块轨道 */
    lv_style_init(&s_style_lock_slider_main);
    lv_style_set_bg_color(&s_style_lock_slider_main, COLOR_LOCK_TRACK);
    lv_style_set_bg_opa(&s_style_lock_slider_main, LV_OPA_COVER);
    lv_style_set_radius(&s_style_lock_slider_main, 15);
    lv_style_set_pad_all(&s_style_lock_slider_main, 0);
    lv_style_set_border_width(&s_style_lock_slider_main, 0);

    /* 滑块已滑部分（指示器） */
    lv_style_init(&s_style_lock_slider_indicator);
    lv_style_set_bg_color(&s_style_lock_slider_indicator, COLOR_LOCK_INDICATOR);
    lv_style_set_bg_opa(&s_style_lock_slider_indicator, LV_OPA_COVER);
    lv_style_set_radius(&s_style_lock_slider_indicator, 15);

    /* 滑块按钮 */
    lv_style_init(&s_style_lock_slider_knob);
    lv_style_set_bg_color(&s_style_lock_slider_knob, COLOR_LOCK_KNOB);
    lv_style_set_bg_opa(&s_style_lock_slider_knob, LV_OPA_COVER);
    lv_style_set_radius(&s_style_lock_slider_knob, 15);
    lv_style_set_shadow_width(&s_style_lock_slider_knob, 6);
    lv_style_set_shadow_color(&s_style_lock_slider_knob, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&s_style_lock_slider_knob, LV_OPA_40);
    lv_style_set_shadow_ofs_y(&s_style_lock_slider_knob, 2);

}

/*********************
 * 获取传感器主题色
 *********************/
static lv_color_t get_sensor_color(SensorType_t type)
{
    switch (type) {
        case SENSOR_TEMP:  return COLOR_TEMP;
        case SENSOR_HUMI:  return COLOR_HUMI;
        case SENSOR_LIGHT: return COLOR_LIGHT;
        case SENSOR_GAS:   return COLOR_GAS;
        default:            return COLOR_TEXT;
    }
}

/*********************
 * 创建顶部标题栏
 *********************/
static lv_obj_t *create_titlebar(lv_obj_t *parent, const char *title)
{
    lv_obj_t *titlebar = lv_obj_create(parent);
    lv_obj_set_pos(titlebar, 0, 0);
    lv_obj_set_size(titlebar, 240, 32);
    lv_obj_add_style(titlebar, &s_style_titlebar, 0);
    lv_obj_clear_flag(titlebar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(titlebar);
    lv_label_set_text(label, title);
    lv_obj_add_style(label, &s_style_title_text, 0);
    lv_obj_center(label);

    return titlebar;
}

/*********************
 * 创建底部Tab栏
 *********************/
static lv_obj_t *create_tabbar(lv_obj_t *parent, PageIndex_t active)
{
    lv_obj_t *tabbar = lv_obj_create(parent);
    lv_obj_set_pos(tabbar, 0, 282);
    lv_obj_set_size(tabbar, 240, 38);
    lv_obj_add_style(tabbar, &s_style_tabbar, 0);
    lv_obj_clear_flag(tabbar, LV_OBJ_FLAG_SCROLLABLE);
    /* 阻止Tab栏上的手势事件冒泡到页面，避免点击时触发页面滑动 */
    lv_obj_add_event_cb(tabbar, tabbar_gesture_cb, LV_EVENT_GESTURE, NULL);

    const char *tab_names[PAGE_COUNT] = {"Display", "Settings", "Device"};
    lv_coord_t tab_w = 80;

    for (uint8_t i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t *tab = lv_btn_create(tabbar);
        lv_obj_set_pos(tab, i * tab_w + 2, 3);
        lv_obj_set_size(tab, tab_w - 4, 32);
        if (i == active) {
            lv_obj_add_style(tab, &s_style_tab_btn_active, 0);
        } else {
            lv_obj_add_style(tab, &s_style_tab_btn, 0);
        }
        lv_obj_t *label = lv_label_create(tab);
        lv_label_set_text(label, tab_names[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(tab, tab_event_cb, LV_EVENT_CLICKED, (void *)(long)i);
        lv_obj_add_event_cb(tab, tabbar_gesture_cb, LV_EVENT_GESTURE, NULL);
    }

    return tabbar;
}

/*********************
 * Tab栏手势阻止冒泡（避免点击Tab时触发页面滑动）
 *********************/
static void tabbar_gesture_cb(lv_event_t *e)
{
    lv_event_stop_bubbling(e);
}

/*********************
 * Tab点击事件
 *********************/
static void tab_event_cb(lv_event_t *e)
{
    PageIndex_t page = (PageIndex_t)(long)lv_event_get_user_data(e);
    if (page == s_current_page) return;
    bool slide_left = (page > s_current_page);
    switch_to_page(page, slide_left);
}

/*********************
 * 左右滑动手势事件
 *********************/
//static void gesture_event_cb(lv_event_t *e)
//{
//    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
//    if (dir == LV_DIR_LEFT && s_current_page < PAGE_COUNT - 1) {
//        switch_to_page((s_current_page + 1, true);
//    } else if (dir == LV_DIR_RIGHT && s_current_page > 0) {
//        switch_to_page((s_current_page - 1, false);
//    }
//}

/*********************
 * 切换页面
 *********************/
static void switch_to_page(PageIndex_t page, bool slide_left)
{
    if (page >= PAGE_COUNT) return;
    s_current_page = page;
    lv_scr_load_anim_t anim = slide_left ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    lv_scr_load_anim(s_screens[page], anim, 200, 0, false);
}

/*********************
 * 创建单个传感器卡片
 *********************/
static lv_obj_t *create_sensor_card(lv_obj_t *parent, SensorType_t type,
                                      lv_coord_t x, lv_coord_t y,
                                      lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_add_style(card, &s_style_card, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *color_bar = lv_obj_create(card);
    lv_obj_set_pos(color_bar, 0, 0);
    lv_obj_set_size(color_bar, w, 4);
    lv_obj_set_style_bg_color(color_bar, get_sensor_color(type), 0);
    lv_obj_set_style_bg_opa(color_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(color_bar, 0, 0);
    lv_obj_set_style_border_width(color_bar, 0, 0);
    lv_obj_set_style_pad_all(color_bar, 0, 0);

    lv_obj_t *name_label = lv_label_create(card);
    lv_label_set_text(name_label, SensorData_GetName(type));
    lv_obj_add_style(name_label, &s_style_name_text, 0);
    lv_obj_align(name_label, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *value_label = lv_label_create(card);
    lv_label_set_text(value_label, "--");
    lv_obj_set_style_text_color(value_label, get_sensor_color(type), 0);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, 5);
    s_card_value_labels[type] = value_label;

    lv_obj_t *unit_label = lv_label_create(card);
    lv_label_set_text(unit_label, SensorData_GetUnit(type));
    lv_obj_set_style_text_color(unit_label, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(unit_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(unit_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_add_event_cb(card, card_event_cb, LV_EVENT_CLICKED, (void *)type);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

    return card;
}

/*********************
 * 创建设置页的一行开关
 *********************/
static lv_obj_t *create_setting_row(lv_obj_t *parent, const char *name, bool init_val, lv_coord_t y)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_pos(row, 15, y);
    lv_obj_set_size(row, 210, 44);
    lv_obj_add_style(row, &s_style_card, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 12, 0);

    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 44, 24);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -10, 0);
    /* 轨道（关闭状态）- 浅灰棕色，和白色卡片区分开 */
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xD4C8B8), LV_PART_MAIN);
    lv_obj_set_style_radius(sw, 12, LV_PART_MAIN);
    /* 指示器（开启状态）- 主题橙色 */
    lv_obj_set_style_bg_color(sw, COLOR_TEMP, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_radius(sw, 12, LV_PART_INDICATOR);
    /* 滑块 - 白色带阴影，在两种状态下都清晰 */
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_shadow_width(sw, 4, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(sw, lv_color_hex(0x888888), LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(sw, LV_OPA_50, LV_PART_KNOB);
    lv_obj_set_style_shadow_ofs_y(sw, 1, LV_PART_KNOB);
    if (init_val) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, setting_switch_cb, LV_EVENT_VALUE_CHANGED, (void *)name);

    return row;
}

/*********************
 * 设置开关回调
 *********************/
static void setting_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    const char *name = (const char *)lv_event_get_user_data(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (strcmp(name, "Auto Refresh") == 0) s_setting_auto_refresh = state;
    else if (strcmp(name, "Data Upload") == 0) s_setting_upload = state;
}

/*********************
 * 展示页
 *********************/
static void create_display_page(lv_obj_t *parent)
{
    create_titlebar(parent, "Env Monitor");

    /* 2x2 传感器卡片，缩小高度给底部Tab栏留空间 */
    create_sensor_card(parent, SENSOR_TEMP,  10,  42, 105, 110);
    create_sensor_card(parent, SENSOR_HUMI,  125, 42, 105, 110);
    create_sensor_card(parent, SENSOR_LIGHT, 10,  160, 105, 110);
    create_sensor_card(parent, SENSOR_GAS,   125, 160, 105, 110);

    create_tabbar(parent, PAGE_DISPLAY);
}

/*********************
 * 设置页
 *********************/
static void create_settings_page(lv_obj_t *parent)
{
    create_titlebar(parent, "Settings");

    create_setting_row(parent, "Auto Refresh", s_setting_auto_refresh, 50);
    create_setting_row(parent, "Data Upload", s_setting_upload, 102);
	
	 /* 锁定屏幕按钮 */
    lv_obj_t *lock_btn = lv_btn_create(parent);
    lv_obj_set_pos(lock_btn, 15, 155);
    lv_obj_set_size(lock_btn, 210, 36);
    lv_obj_add_style(lock_btn, &s_style_btn, 0);
    lv_obj_t *lock_btn_label = lv_label_create(lock_btn);
    lv_label_set_text(lock_btn_label, "  Lock Screen");
    lv_obj_center(lock_btn_label);
    lv_obj_add_event_cb(lock_btn, setting_lock_btn_cb, LV_EVENT_CLICKED, NULL);

	
    create_tabbar(parent, PAGE_SETTINGS);
}

/*********************
 * 设备详情页
 *********************/
static void create_device_page(lv_obj_t *parent)
{
    create_titlebar(parent, "Device Info");

    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_pos(panel, 15, 50);
    lv_obj_set_size(panel, 210, 218);
    lv_obj_add_style(panel, &s_style_card, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    const char *info_lines[] = {
        "MCU:  STM32F407VET6",
        "Core:  Cortex-M4 168MHz",
        "LCD:  ILI9341 240x320",
        "Touch: XPT2046",
        "LVGL:  v8.3.x",
        "Sensor: SHT30 Temp/Humi",
        "Sensor: BH1750 Light",
        "Sensor: MQ2 Gas",
        "FW Ver: v1.0.0",
    };

    for (uint8_t i = 0; i < 9; i++) {
        lv_obj_t *line = lv_label_create(panel);
        lv_label_set_text(line, info_lines[i]);
        lv_obj_set_style_text_color(line, (i == 8) ? COLOR_TEMP : COLOR_TEXT, 0);
        lv_obj_set_style_text_font(line, &lv_font_montserrat_12, 0);
        lv_obj_align(line, LV_ALIGN_TOP_LEFT, 12, 10 + i * 22);
    }

    create_tabbar(parent, PAGE_DEVICE);
}


/*********************
 * 创建锁屏界面
 *********************/
static void create_lock_screen(void)
{
    /* 锁屏页面（独立屏幕，父对象为NULL） */
    s_lock_screen = lv_obj_create(NULL);
    lv_obj_add_style(s_lock_screen, &s_style_lock_bg, 0);
    lv_obj_clear_flag(s_lock_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 标题 */
    lv_obj_t *title_label = lv_label_create(s_lock_screen);
    lv_label_set_text(title_label, "Env Monitor");
    lv_obj_add_style(title_label, &s_style_lock_title, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 115);

    /* 提示文字 */
    lv_obj_t *hint_label = lv_label_create(s_lock_screen);
    lv_label_set_text(hint_label, "Swipe right to unlock");
    lv_obj_add_style(hint_label, &s_style_lock_hint, 0);
    lv_obj_align(hint_label, LV_ALIGN_TOP_MID, 0, 170);

    /* 滑块 */
    s_lock_slider = lv_slider_create(s_lock_screen);
    lv_obj_set_size(s_lock_slider, 200, 30);
    lv_obj_align(s_lock_slider, LV_ALIGN_TOP_MID, 15, 215);
    lv_slider_set_range(s_lock_slider, 0, 100);
    lv_slider_set_value(s_lock_slider, 0, LV_ANIM_OFF);
    lv_obj_add_style(s_lock_slider, &s_style_lock_slider_main, LV_PART_MAIN);
    lv_obj_add_style(s_lock_slider, &s_style_lock_slider_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(s_lock_slider, &s_style_lock_slider_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(s_lock_slider, lock_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_lock_slider, lock_slider_event_cb, LV_EVENT_RELEASED, NULL);

    /* 百分比文字 */
    s_lock_percent_label = lv_label_create(s_lock_screen);
    lv_label_set_text(s_lock_percent_label, "0%");
    lv_obj_set_style_text_font(s_lock_percent_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_lock_percent_label, COLOR_LOCK_HINT, 0);
    lv_obj_align(s_lock_percent_label, LV_ALIGN_TOP_MID, 5, 260);
}

/*********************
 * 滑块事件回调
 *********************/
static void lock_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    lv_event_code_t code = lv_event_get_code(e);

    /* 实时更新百分比 */
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)value);
    lv_label_set_text(s_lock_percent_label, buf);

    /* 滑到100%触发解锁 */
    if (value >= 90)
    {
        unlock_screen();
        return;
    }

    /* 松手时如果没到100%，弹回0 */
    if (code == LV_EVENT_RELEASED && value < 100)
    {
        lv_slider_set_value(slider, 0, LV_ANIM_ON);
        lv_label_set_text(s_lock_percent_label, "0%");
    }
}

/*********************
 * 解锁屏幕
 *********************/
static void unlock_screen(void)
{
    if (!s_is_locked) return;
    s_is_locked = false;
	s_current_page = PAGE_DISPLAY;

    //USART1_Printf("[UI] Screen unlocked\r\n");

    /* 切换到主界面（上移动画，300ms） */
    lv_scr_load_anim(s_screens[PAGE_DISPLAY], LV_SCR_LOAD_ANIM_MOVE_TOP, 300, 0, false);

    /* 重置滑块，下次锁屏从0开始 */
    lv_slider_set_value(s_lock_slider, 0, LV_ANIM_OFF);
    lv_label_set_text(s_lock_percent_label, "0%");
	
	backlight_reset();   /* 解锁后重置背光计时 */
}

/*********************
 * 手动锁定屏幕（公共接口）
 *********************/
void ui_lock_screen_now(void)
{
    if (s_is_locked) return;
    if (s_lock_screen == NULL) return;  /* 还没创建锁屏 */

    s_is_locked = true;
    //USART1_Printf("[UI] Screen locked\r\n");

    /* 切换到锁屏界面（下移动画） */
    lv_scr_load_anim(s_lock_screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
}

static void setting_lock_btn_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    ui_lock_screen_now();
}


/*********************
 * 传感器详情页（弹窗）
 *********************/
static void create_detail_screen(void)
{
    s_detail_screen = lv_obj_create(NULL);
    lv_obj_add_style(s_detail_screen, &s_style_bg, 0);
    lv_obj_clear_flag(s_detail_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 标题栏 */
    lv_obj_t *titlebar = lv_obj_create(s_detail_screen);
    lv_obj_set_pos(titlebar, 0, 0);
    lv_obj_set_size(titlebar, 240, 32);
    lv_obj_add_style(titlebar, &s_style_titlebar, 0);
    lv_obj_clear_flag(titlebar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_btn = lv_btn_create(titlebar);
    lv_obj_set_pos(back_btn, 6, 4);
    lv_obj_set_size(back_btn, 50, 24);
    lv_obj_add_style(back_btn, &s_style_btn, 0);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, back_from_detail_cb, LV_EVENT_CLICKED, NULL);

    s_detail_title_label = lv_label_create(titlebar);
    lv_label_set_text(s_detail_title_label, "Temp");
    lv_obj_add_style(s_detail_title_label, &s_style_title_text, 0);
    lv_obj_align(s_detail_title_label, LV_ALIGN_CENTER, 5, 0);

    /* 右上角暂停/继续按钮 */
    s_pause_btn = lv_btn_create(titlebar);
    lv_obj_set_pos(s_pause_btn, 184, 4);
    lv_obj_set_size(s_pause_btn, 50, 24);
    lv_obj_add_style(s_pause_btn, &s_style_btn, 0);
    s_pause_btn_label = lv_label_create(s_pause_btn);
    lv_label_set_text(s_pause_btn_label, "STOP");
    lv_obj_set_style_text_color(s_pause_btn_label, COLOR_TEMP, 0);
    lv_obj_center(s_pause_btn_label);
    lv_obj_add_event_cb(s_pause_btn, pause_btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* 折线图 */
    s_detail_chart = lv_chart_create(s_detail_screen);
    lv_obj_set_pos(s_detail_chart, 44, 44);
    lv_obj_set_size(s_detail_chart, 186, 170);
    lv_obj_add_style(s_detail_chart, &s_style_chart, 0);
    lv_chart_set_type(s_detail_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_detail_chart, HISTORY_MAX * 2);
    lv_chart_set_range(s_detail_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(s_detail_chart, 6, 6);
    lv_chart_set_update_mode(s_detail_chart, LV_CHART_UPDATE_MODE_SHIFT);

    s_detail_series = lv_chart_add_series(s_detail_chart, COLOR_TEMP, LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_size(s_detail_chart, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(s_detail_chart, 2, LV_PART_INDICATOR);

    /* Y轴刻度标签 */
    for (uint8_t i = 0; i < 5; i++) {
        s_y_labels[i] = lv_label_create(s_detail_screen);
        lv_label_set_text(s_y_labels[i], "--");
        lv_obj_set_style_text_color(s_y_labels[i], COLOR_TEXT_DIM, 0);
        lv_obj_set_style_text_font(s_y_labels[i], &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_align(s_y_labels[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_size(s_y_labels[i], 36, 14);
        lv_obj_set_pos(s_y_labels[i], 6, 45 + i * 39);
    }

    /* 底部统计面板 */
    lv_obj_t *stats_panel = lv_obj_create(s_detail_screen);
    lv_obj_set_pos(stats_panel, 10, 224);
    lv_obj_set_size(stats_panel, 220, 86);
    lv_obj_add_style(stats_panel, &s_style_card, 0);
    lv_obj_clear_flag(stats_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cur_title = lv_label_create(stats_panel);
    lv_label_set_text(cur_title, "Cur");
    lv_obj_set_style_text_color(cur_title, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(cur_title, &lv_font_montserrat_12, 0);
    lv_obj_align(cur_title, LV_ALIGN_TOP_LEFT, 10, 4);

    s_detail_cur_label = lv_label_create(stats_panel);
    lv_label_set_text(s_detail_cur_label, "--");
    lv_obj_set_style_text_color(s_detail_cur_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_detail_cur_label, &lv_font_montserrat_14, 0);
    lv_obj_align(s_detail_cur_label, LV_ALIGN_TOP_LEFT, 10, 20);

    lv_obj_t *max_title = lv_label_create(stats_panel);
    lv_label_set_text(max_title, "Max");
    lv_obj_set_style_text_color(max_title, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(max_title, &lv_font_montserrat_12, 0);
    lv_obj_align(max_title, LV_ALIGN_TOP_RIGHT, -10, 4);

    s_detail_max_label = lv_label_create(stats_panel);
    lv_label_set_text(s_detail_max_label, "--");
    lv_obj_set_style_text_color(s_detail_max_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_detail_max_label, &lv_font_montserrat_14, 0);
    lv_obj_align(s_detail_max_label, LV_ALIGN_TOP_RIGHT, -10, 20);

    lv_obj_t *min_title = lv_label_create(stats_panel);
    lv_label_set_text(min_title, "Min");
    lv_obj_set_style_text_color(min_title, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(min_title, &lv_font_montserrat_12, 0);
    lv_obj_align(min_title, LV_ALIGN_TOP_LEFT, 10, 46);

    s_detail_min_label = lv_label_create(stats_panel);
    lv_label_set_text(s_detail_min_label, "--");
    lv_obj_set_style_text_color(s_detail_min_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_detail_min_label, &lv_font_montserrat_14, 0);
    lv_obj_align(s_detail_min_label, LV_ALIGN_TOP_LEFT, 10, 62);

    lv_obj_t *avg_title = lv_label_create(stats_panel);
    lv_label_set_text(avg_title, "Avg");
    lv_obj_set_style_text_color(avg_title, COLOR_TEXT_DIM, 0);
    lv_obj_set_style_text_font(avg_title, &lv_font_montserrat_12, 0);
    lv_obj_align(avg_title, LV_ALIGN_TOP_RIGHT, -10, 46);

    s_detail_avg_label = lv_label_create(stats_panel);
    lv_label_set_text(s_detail_avg_label, "--");
    lv_obj_set_style_text_color(s_detail_avg_label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(s_detail_avg_label, &lv_font_montserrat_14, 0);
    lv_obj_align(s_detail_avg_label, LV_ALIGN_TOP_RIGHT, -10, 62);
}

/*********************
 * 卡片点击 - 进入传感器详情
 *********************/
static void card_event_cb(lv_event_t *e)
{
    SensorType_t type = (SensorType_t)lv_event_get_user_data(e);
    s_current_sensor = type;

    lv_label_set_text(s_detail_title_label, SensorData_GetName(type));
    lv_obj_set_style_text_color(s_detail_title_label, get_sensor_color(type), 0);

    lv_chart_set_series_color(s_detail_chart, s_detail_series, get_sensor_color(type));

    /* 更新暂停按钮状态 */
    if (s_sensor_paused[type]) {
        lv_label_set_text(s_pause_btn_label, "START");
        lv_obj_set_style_text_color(s_pause_btn_label, COLOR_GAS, 0);
    } else {
        lv_label_set_text(s_pause_btn_label, "STOP");
        lv_obj_set_style_text_color(s_pause_btn_label, COLOR_TEMP, 0);
    }

    update_detail_chart();
    update_detail_stats();
    lv_scr_load_anim(s_detail_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
}

/*********************
 * 从详情页返回
 *********************/
static void back_from_detail_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    lv_scr_load_anim(s_screens[s_current_page], LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
}

/*********************
 * 暂停/继续按钮回调 - 仅控制当前传感器的刷新
 *********************/
static void pause_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_sensor_paused[s_current_sensor] = !s_sensor_paused[s_current_sensor];
    if (s_sensor_paused[s_current_sensor]) {
        lv_label_set_text(s_pause_btn_label, "START");
        lv_obj_set_style_text_color(s_pause_btn_label, COLOR_GAS, 0);
    } else {
        lv_label_set_text(s_pause_btn_label, "STOP");
        lv_obj_set_style_text_color(s_pause_btn_label, COLOR_TEMP, 0);
    }
}

/*********************
 * 更新折线图
 *********************/
static void update_detail_chart(void)
{
    float hist_buf[HISTORY_MAX];
    uint16_t count = SensorData_GetHistory(s_current_sensor, hist_buf, HISTORY_MAX);
    uint16_t display_count = HISTORY_MAX * 2;

    lv_chart_set_all_value(s_detail_chart, s_detail_series, 0);

    if (count == 0) {
        for (uint8_t i = 0; i < 5; i++) lv_label_set_text(s_y_labels[i], "--");
        lv_chart_refresh(s_detail_chart);
        return;
    }

    float vmin = hist_buf[0], vmax = hist_buf[0];
    for (uint16_t i = 1; i < count; i++) {
        if (hist_buf[i] < vmin) vmin = hist_buf[i];
        if (hist_buf[i] > vmax) vmax = hist_buf[i];
    }
    float range = vmax - vmin;
    if (range < 1.0f) range = 1.0f;
    float y_min = vmin - range * 0.1f;
    float y_max = vmax + range * 0.1f;
    if (y_min < 0) y_min = 0;
    lv_chart_set_range(s_detail_chart, LV_CHART_AXIS_PRIMARY_Y, (lv_coord_t)y_min, (lv_coord_t)y_max);

    float y_range = y_max - y_min;
    uint8_t y_decimals = (y_range < 2) ? 2 : (y_range < 10 ? 1 : 0);
    for (uint8_t i = 0; i < 5; i++) {
        float val = y_max - (y_max - y_min) * (float)i / 4.0f;
        char ybuf[16];
        snprintf(ybuf, sizeof(ybuf), "%.*f", y_decimals, val);
        lv_label_set_text(s_y_labels[i], ybuf);
    }

    for (uint16_t i = 0; i < display_count; i++) {
        float val;
        if (count == 1) {
            val = hist_buf[0];
        } else {
            float pos = (float)i / (float)(display_count - 1) * (float)(count - 1);
            uint16_t idx0 = (uint16_t)pos;
            uint16_t idx1 = (idx0 + 1 < count) ? (idx0 + 1) : (count - 1);
            float frac = pos - (float)idx0;
            val = hist_buf[idx0] * (1.0f - frac) + hist_buf[idx1] * frac;
        }
        lv_chart_set_next_value(s_detail_chart, s_detail_series, val);
    }

    lv_chart_refresh(s_detail_chart);
}

/*********************
 * 更新统计信息
 *********************/
static void update_detail_stats(void)
{
    float hist_buf[HISTORY_MAX];
    uint16_t count = SensorData_GetHistory(s_current_sensor, hist_buf, HISTORY_MAX);
    const char *unit = SensorData_GetUnit(s_current_sensor);
    char buf[32];

    if (count == 0) {
        lv_label_set_text(s_detail_cur_label, "--");
        lv_label_set_text(s_detail_max_label, "--");
        lv_label_set_text(s_detail_min_label, "--");
        lv_label_set_text(s_detail_avg_label, "--");
        return;
    }

    float vmin = hist_buf[0], vmax = hist_buf[0], vsum = 0;
    for (uint16_t i = 0; i < count; i++) {
        if (hist_buf[i] < vmin) vmin = hist_buf[i];
        if (hist_buf[i] > vmax) vmax = hist_buf[i];
        vsum += hist_buf[i];
    }
    float vavg = vsum / count;
    float vcur = SensorData_GetValue(s_current_sensor);
    uint8_t decimals = (s_current_sensor == SENSOR_TEMP || s_current_sensor == SENSOR_HUMI) ? 1 : 0;

    snprintf(buf, sizeof(buf), "%.*f %s", decimals, vcur, unit);
    lv_label_set_text(s_detail_cur_label, buf);
    snprintf(buf, sizeof(buf), "%.*f %s", decimals, vmax, unit);
    lv_label_set_text(s_detail_max_label, buf);
    snprintf(buf, sizeof(buf), "%.*f %s", decimals, vmin, unit);
    lv_label_set_text(s_detail_min_label, buf);
    snprintf(buf, sizeof(buf), "%.*f %s", decimals, vavg, unit);
    lv_label_set_text(s_detail_avg_label, buf);
}

/*********************
 * 刷新展示页卡片
 *********************/
static void refresh_display_cards(void)
{
    EnvData_t data = SensorData_GetCurrent();
    char buf[24];
    if (!s_sensor_paused[SENSOR_TEMP]) {
        snprintf(buf, sizeof(buf), "%.1f", data.temp);
        lv_label_set_text(s_card_value_labels[SENSOR_TEMP], buf);
    }
    if (!s_sensor_paused[SENSOR_HUMI]) {
        snprintf(buf, sizeof(buf), "%.1f", data.humi);
        lv_label_set_text(s_card_value_labels[SENSOR_HUMI], buf);
    }
    if (!s_sensor_paused[SENSOR_LIGHT]) {
        snprintf(buf, sizeof(buf), "%d", data.light);
        lv_label_set_text(s_card_value_labels[SENSOR_LIGHT], buf);
    }
    if (!s_sensor_paused[SENSOR_GAS]) {
        snprintf(buf, sizeof(buf), "%d", data.gas);
        lv_label_set_text(s_card_value_labels[SENSOR_GAS], buf);
    }
}

/*********************
 * 公共接口：UI初始化
 *********************/
void ui_demo_01_init(void)
{
    SensorData_Init();
    init_styles();

    for (uint8_t i = 0; i < PAGE_COUNT; i++) {
        s_screens[i] = lv_obj_create(NULL);
        lv_obj_add_style(s_screens[i], &s_style_bg, 0);
        lv_obj_clear_flag(s_screens[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    create_display_page(s_screens[PAGE_DISPLAY]);
    create_settings_page(s_screens[PAGE_SETTINGS]);
    create_device_page(s_screens[PAGE_DEVICE]);
	
    create_detail_screen();

    s_current_page = PAGE_DISPLAY;
	
    /* ===== 创建锁屏界面并显示（解锁后才进入主界面） ===== */
    create_lock_screen();
    s_is_locked = true;
    lv_scr_load(s_lock_screen);
}

/*********************
 * 公共接口：刷新数据
 *********************/
void ui_refresh_sensor_data(void)
{
    if (!s_setting_auto_refresh) return;
    refresh_display_cards();
    if (lv_scr_act() == s_detail_screen && !s_sensor_paused[s_current_sensor]) {
        update_detail_chart();
        update_detail_stats();
    }
}

/*********************
 * 公共接口：查询数据上传开关状态
 *********************/
bool ui_is_upload_enabled(void)
{
    return s_setting_upload;
}
