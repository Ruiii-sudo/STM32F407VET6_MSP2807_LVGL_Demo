/**
  ******************************************************************************
  * @file    ui_demo_01.c
  * @brief   LVGL自定义UI界面Demo
  *          包含主菜单、控件演示、关于三个页面，支持页面切换动画
  *          控件演示页结合PC5引脚做硬件交互（开关控制PC5输出）
  ******************************************************************************
  */

#include "ui_demo_01.h"
#include "lvgl.h"
#include "stm32f4xx_hal.h"

/* 如果需要串口输出调试信息，取消下面这行注释并确保包含usart.h */
/* #include "usart.h" */

/* ==================== 全局屏幕指针 ==================== */
static lv_obj_t *screen_menu;     /* 主菜单页面 */
static lv_obj_t *screen_widgets;  /* 控件演示页面 */
static lv_obj_t *screen_about;    /* 关于页面 */

/* ==================== 控件演示页全局控件 ==================== */
static lv_obj_t *label_slider_val;  /* 滑块值显示标签 */
static lv_obj_t *bar_progress;       /* 进度条 */
static lv_obj_t *label_switch_state; /* 开关状态标签 */

/* ==================== 颜色定义 ==================== */
#define COLOR_BG       lv_color_hex(0x1A1A2E)   /* 背景深蓝 */
#define COLOR_CARD     lv_color_hex(0x16213E)   /* 卡片色 */
#define COLOR_ACCENT   lv_color_hex(0x0F3460)   /* 强调色 */
#define COLOR_PRIMARY  lv_color_hex(0xE94560)   /* 主色调（红粉） */
#define COLOR_TEXT     lv_color_hex(0xFFFFFF)    /* 文字白 */
#define COLOR_SUBTEXT  lv_color_hex(0xA0A0B0)   /* 次要文字 */
#define COLOR_GREEN    lv_color_hex(0x4CAF50)    /* 绿色 */
#define COLOR_ORANGE   lv_color_hex(0xFF9800)    /* 橙色 */

/* ==================== 通用样式辅助函数 ==================== */

/**
  * @brief  设置对象背景色
  */
static void set_bg_color(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN);
}

/**
  * @brief  设置文字颜色
  */
static void set_text_color(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_text_color(obj, color, LV_PART_MAIN);
}

/**
  * @brief  创建一个带样式的按钮
  * @param  parent: 父对象
  * @param  text: 按钮文字
  * @param  bg_color: 背景色
  * @return 按钮对象指针
  */
static lv_obj_t *create_styled_btn(lv_obj_t *parent, const char *text, lv_color_t bg_color)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 160, 44);
    lv_obj_set_style_bg_color(btn, bg_color, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    /* 按下时的样式 */
    lv_obj_set_style_bg_color(btn, lv_color_darken(bg_color, 30), LV_PART_MAIN | LV_STATE_PRESSED);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    set_text_color(label, COLOR_TEXT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label);

    return btn;
}

/**
  * @brief  创建页面标题
  */
static lv_obj_t *create_page_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    set_text_color(label, COLOR_PRIMARY);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 15);
    return label;
}

/* ==================== 页面切换 ==================== */

/**
  * @brief  切换到指定屏幕（带动画）
  * @param  new_screen: 新屏幕指针
  * @param  direction: 动画方向 (LV_SCR_LOAD_ANIM_MOVE_LEFT/RIGHT)
  */
static void switch_screen(lv_obj_t *new_screen, lv_scr_load_anim_t direction)
{
    lv_scr_load_anim(new_screen, direction, 250, 0, false);
}

/* ==================== 主菜单页面 ==================== */

/* 主菜单按钮事件回调 */
static void menu_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int user_data = (int)(long)lv_event_get_user_data(e);

    switch (user_data)
    {
    case 0: /* Widgets */
        switch_screen(screen_widgets, LV_SCR_LOAD_ANIM_MOVE_LEFT);
        break;
    case 1: /* About */
        switch_screen(screen_about, LV_SCR_LOAD_ANIM_MOVE_LEFT);
        break;
    default:
        break;
    }
}

/**
  * @brief  创建主菜单页面
  */
static void create_menu_screen(void)
{
    screen_menu = lv_obj_create(NULL);
    set_bg_color(screen_menu, COLOR_BG);
    lv_obj_set_style_border_width(screen_menu, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen_menu, 0, LV_PART_MAIN);

    /* 标题 */
    create_page_title(screen_menu, "LVGL UI Demo");

    /* 副标题 */
    lv_obj_t *subtitle = lv_label_create(screen_menu);
    lv_label_set_text(subtitle, "STM32F407 + ILI9341");
    set_text_color(subtitle, COLOR_SUBTEXT);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 45);

    /* 分隔线 */
    lv_obj_t *line = lv_obj_create(screen_menu);
    lv_obj_set_size(line, 180, 2);
    lv_obj_set_style_bg_color(line, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line, 1, LV_PART_MAIN);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 70);

    /* 按钮1: Widgets */
    lv_obj_t *btn_widgets = create_styled_btn(screen_menu, "Widgets", COLOR_PRIMARY);
    lv_obj_align(btn_widgets, LV_ALIGN_CENTER, 0, -20);
    lv_obj_add_event_cb(btn_widgets, menu_btn_event_cb, LV_EVENT_CLICKED, (void *)(long)0);

    /* 按钮2: About */
    lv_obj_t *btn_about = create_styled_btn(screen_menu, "About", COLOR_ACCENT);
    lv_obj_align(btn_about, LV_ALIGN_CENTER, 0, 45);
    lv_obj_add_event_cb(btn_about, menu_btn_event_cb, LV_EVENT_CLICKED, (void *)(long)1);

    /* 底部提示 */
    lv_obj_t *footer = lv_label_create(screen_menu);
    lv_label_set_text(footer, "Tap button to enter");
    set_text_color(footer, COLOR_SUBTEXT);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -15);
}

/* ==================== 控件演示页面 ==================== */

/* 滑块值变化回调 */
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);

    /* 更新标签显示 */
    lv_label_set_text_fmt(label_slider_val, "%d", value);

    /* 同步更新进度条 */
    lv_bar_set_value(bar_progress, value, LV_ANIM_ON);
}

/* 开关状态变化回调 */
static void switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    int state = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (state)
    {
        lv_label_set_text(label_switch_state, "PC5: ON");
        set_text_color(label_switch_state, COLOR_GREEN);
        /* 控制PC5输出高电平 */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
    }
    else
    {
        lv_label_set_text(label_switch_state, "PC5: OFF");
        set_text_color(label_switch_state, COLOR_SUBTEXT);
        /* 控制PC5输出低电平 */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
    }
}

/* 返回按钮回调 */
static void back_to_menu_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    switch_screen(screen_menu, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}

/**
  * @brief  创建控件演示页面
  */
static void create_widgets_screen(void)
{
    screen_widgets = lv_obj_create(NULL);
    set_bg_color(screen_widgets, COLOR_BG);
    lv_obj_set_style_border_width(screen_widgets, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen_widgets, 0, LV_PART_MAIN);

    /* 标题 */
    create_page_title(screen_widgets, "Widgets Demo");

    /* ---------- 滑块区域 ---------- */
    lv_obj_t *label_slider_title = lv_label_create(screen_widgets);
    lv_label_set_text(label_slider_title, "Slider");
    set_text_color(label_slider_title, COLOR_TEXT);
    lv_obj_set_style_text_font(label_slider_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_slider_title, LV_ALIGN_TOP_LEFT, 20, 75);

    /* 滑块值显示 */
    label_slider_val = lv_label_create(screen_widgets);
    lv_label_set_text(label_slider_val, "50");
    set_text_color(label_slider_val, COLOR_ORANGE);
    lv_obj_set_style_text_font(label_slider_val, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label_slider_val, LV_ALIGN_TOP_RIGHT, -20, 72);

    /* 滑块 */
    lv_obj_t *slider = lv_slider_create(screen_widgets);
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 100);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, COLOR_PRIMARY, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, COLOR_PRIMARY, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ---------- 进度条 ---------- */
    lv_obj_t *label_bar_title = lv_label_create(screen_widgets);
    lv_label_set_text(label_bar_title, "Progress");
    set_text_color(label_bar_title, COLOR_TEXT);
    lv_obj_set_style_text_font(label_bar_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_bar_title, LV_ALIGN_TOP_LEFT, 20, 140);

    bar_progress = lv_bar_create(screen_widgets);
    lv_obj_set_size(bar_progress, 200, 16);
    lv_obj_align(bar_progress, LV_ALIGN_TOP_MID, 0, 165);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_bar_set_value(bar_progress, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_progress, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_progress, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_progress, 8, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_progress, 8, LV_PART_INDICATOR);

    /* ---------- 开关区域 ---------- */
    lv_obj_t *label_switch_title = lv_label_create(screen_widgets);
    lv_label_set_text(label_switch_title, "Switch (PC5)");
    set_text_color(label_switch_title, COLOR_TEXT);
    lv_obj_set_style_text_font(label_switch_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_switch_title, LV_ALIGN_TOP_LEFT, 20, 200);

    /* 开关 */
    lv_obj_t *sw = lv_switch_create(screen_widgets);
    lv_obj_set_size(sw, 50, 28);
    lv_obj_align(sw, LV_ALIGN_TOP_LEFT, 20, 225);
    lv_obj_set_style_bg_color(sw, COLOR_CARD, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, COLOR_GREEN, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 开关状态标签 */
    label_switch_state = lv_label_create(screen_widgets);
    lv_label_set_text(label_switch_state, "PC5: OFF");
    set_text_color(label_switch_state, COLOR_SUBTEXT);
    lv_obj_set_style_text_font(label_switch_state, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_switch_state, LV_ALIGN_TOP_LEFT, 85, 228);

    /* ---------- 返回按钮 ---------- */
    lv_obj_t *btn_back = create_styled_btn(screen_widgets, "< Back", COLOR_ACCENT);
    lv_obj_set_size(btn_back, 100, 38);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_add_event_cb(btn_back, back_to_menu_cb, LV_EVENT_CLICKED, NULL);
}

/* ==================== 关于页面 ==================== */

/**
  * @brief  创建关于页面
  */
static void create_about_screen(void)
{
    screen_about = lv_obj_create(NULL);
    set_bg_color(screen_about, COLOR_BG);
    lv_obj_set_style_border_width(screen_about, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(screen_about, 0, LV_PART_MAIN);

    /* 标题 */
    create_page_title(screen_about, "About");

    /* 信息卡片容器 */
    lv_obj_t *card = lv_obj_create(screen_about);
    lv_obj_set_size(card, 200, 170);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, -10);
    set_bg_color(card, COLOR_CARD);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 15, LV_PART_MAIN);

    /* 信息行 */
    const char *info_lines[] = {
        "MCU:  STM32F407VET6",
        "LCD:  ILI9341 240x320",
        "Touch: XPT2046",
        "LVGL: v8.4.0",
        "Core:  168MHz Cortex-M4",
        "UI Demo: v1.0",
    };

    for (int i = 0; i < 6; i++)
    {
        lv_obj_t *line = lv_label_create(card);
        lv_label_set_text(line, info_lines[i]);
        set_text_color(line, (i == 5) ? COLOR_PRIMARY : COLOR_TEXT);
        lv_obj_set_style_text_font(line, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_align(line, LV_ALIGN_TOP_LEFT, 5, 5 + i * 26);
    }

    /* 返回按钮 */
    lv_obj_t *btn_back = create_styled_btn(screen_about, "< Back", COLOR_ACCENT);
    lv_obj_set_size(btn_back, 100, 38);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_add_event_cb(btn_back, back_to_menu_cb, LV_EVENT_CLICKED, NULL);
}

/* ==================== 初始化入口 ==================== */

/**
  * @brief  UI Demo初始化入口
  * @note   在main.c中调用此函数替换lv_demo_widgets()
  */
void ui_demo_01_init(void)
{
    /* 创建三个页面 */
    create_menu_screen();
    create_widgets_screen();
    create_about_screen();

    /* 加载主菜单页面（无动画，直接显示） */
    lv_scr_load(screen_menu);

    /* 确保PC5初始为低电平 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
}
