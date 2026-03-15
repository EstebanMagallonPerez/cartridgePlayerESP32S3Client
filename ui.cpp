#include "ui.h"
#include <lvgl.h>
#include "EPD.h"
#include "scheduler.h"

extern lv_font_t Thermostat180;
extern lv_font_t Thermostat50;

#define SCREEN_WIDTH 792
#define SCREEN_HEIGHT 272

static uint8_t ImageBW[27200];

/* draw buffer */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 40];

/* LVGL objects */
static lv_obj_t *target_label;
static lv_obj_t *actual_label;
static lv_obj_t *outside_label;
static lv_obj_t *time_label;
static lv_obj_t *fan_label;
static lv_obj_t *mode_label;

/* thermostat state */
static int target_temp = 72;
static int actual_temp = 70;
static const char *fan_modes[] = {"Off","Auto","On"};
static int fan_mode = 0;
static const char *hvac_modes[] = {"Off","+Heat","*Cool","Auto"};
static int hvac_mode = 0;

/* render limiter */
volatile static bool screenDirty = true;
static uint32_t lastRender = 0;
static const uint32_t FRAME_INTERVAL = 100;

/* refresh management */
static int refresh_count = 0;
static unsigned long last_activity = 0;
static bool inactivity_refresh_completed = false;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    int width = area->x2 - area->x1 + 1;
    int height = area->y2 - area->y1 + 1;

    for(int y=0;y<height;y++) {
        for(int x=0;x<width;x++) {
            int px = area->x1 + x;
            int py = area->y1 + y;
            lv_color_t c = color_p[y * width + x];
            if(lv_color_brightness(c) > 128)
                Paint_SetPixel(px,py,WHITE);
            else
                Paint_SetPixel(px,py,BLACK);
        }
    }

    if(lv_disp_flush_is_last(disp)) {
        refresh_count++;
        if(refresh_count > 100) {
            ui_full_refresh();
        } else {
            EPD_Display(ImageBW);
            EPD_PartUpdate();
        }
    }

    lv_disp_flush_ready(disp);
}

void ui_mark_dirty()
{
    screenDirty = true;
    last_activity = millis();
    inactivity_refresh_completed = false;
}

void ui_set_outside(int temp, char icon)
{
    if (outside_label) {
        if (temp == -999) {
            lv_label_set_text(outside_label, "Failed");
        } else {
            lv_label_set_text_fmt(outside_label, "%d°%c", temp, icon);
        }
        ui_mark_dirty();
    }
}

static void format_and_set_time_label(long long epoch_ms)
{
    long long seconds = epoch_ms / 1000LL;
    long long secs_in_day = (seconds % 86400LL + 86400LL) % 86400LL;

    int hour24 = secs_in_day / 3600;
    int minute = (secs_in_day % 3600) / 60;

    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;

    const char *ampm = (hour24 >= 12) ? "PM" : "AM";

    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d%s", hour12, minute, ampm);

    int minutes_now = hour24 * 60 + minute;

//TODO WE NEED TO PARSE THE ACTUAL DAY
    ScheduleAction action = scheduler_tick(minutes_now, 0);

    if (action.trigger_on)
    {
        hvac_mode = action.hvac_mode;
        fan_mode = action.fan_mode;
        //This ui setting is wrong we need to directly set it to the mode
        lv_label_set_text_fmt(fan_label, "_%s", fan_modes[fan_mode]);
        lv_label_set_text_fmt(mode_label, "%s", hvac_modes[hvac_mode]);
    }

    if (action.trigger_off)
    {
        hvac_mode = HVAC_OFF;
        lv_label_set_text_fmt(mode_label, "%s", hvac_modes[hvac_mode]);
    }

    lv_label_set_text(time_label, buf);
    ui_mark_dirty();
}

void ui_set_time_from_epoch(long long epoch_ms)
{
    format_and_set_time_label(epoch_ms);
}

void ui_change_fan_mode()
{
    fan_mode = (fan_mode + 1) % 3;
    if (fan_label) lv_label_set_text_fmt(fan_label, "_%s", fan_modes[fan_mode]);
    ui_mark_dirty();
}

void ui_change_hvac_mode()
{
    hvac_mode = (hvac_mode + 1) % 4;
    if (mode_label) lv_label_set_text_fmt(mode_label, "%s", hvac_modes[hvac_mode]);
    ui_mark_dirty();
}

void ui_inc_target_temp()
{
    target_temp++;
    if (target_label) lv_label_set_text_fmt(target_label, "%d°", target_temp);
    ui_mark_dirty();
}

void ui_dec_target_temp()
{
    target_temp--;
    if (target_label) lv_label_set_text_fmt(target_label, "%d°", target_temp);
    ui_mark_dirty();
}

void ui_full_refresh()
{
    Serial.println("Full refresh");

    EPD_Init();
    EPD_Display_Clear();
    EPD_Update();
    EPD_Clear_R26A6H();

    lv_obj_invalidate(lv_scr_act());
    lv_timer_handler();

    EPD_Display(ImageBW);
    EPD_Update();

    EPD_FastMode1Init();

    refresh_count = 0;
    ui_mark_dirty();
}

void ui_init()
{
    EPD_GPIOInit();
    EPD_FastMode1Init();

    Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
    Paint_Clear(WHITE);

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(scr, lv_color_black(), LV_PART_MAIN);

    target_label = lv_label_create(scr);
    lv_label_set_text_fmt(target_label, "%d°", target_temp);
    lv_obj_set_style_text_font(target_label, &Thermostat50, 0);
    lv_obj_align(target_label, LV_ALIGN_TOP_MID, 0, 5);

    outside_label = lv_label_create(scr);
    lv_label_set_text_fmt(outside_label, "%d°!", actual_temp);
    lv_obj_set_style_text_font(outside_label, &Thermostat50, 0);
    lv_obj_align(outside_label, LV_ALIGN_TOP_LEFT, 20, 5);

    actual_label = lv_label_create(scr);
    lv_label_set_text_fmt(actual_label, "%d°", actual_temp);
    lv_obj_set_style_text_font(actual_label, &Thermostat180, 0);
    lv_obj_align(actual_label, LV_ALIGN_CENTER, 0, 30);

    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 4);
    lv_style_set_line_rounded(&style_line, true);

    static lv_point_t line_points[] = { {20, 60}, {772, 60} };
    lv_obj_t * line1 = lv_line_create(scr);
    lv_obj_add_style(line1, &style_line, 0);
    lv_line_set_points(line1, line_points, 2);

    time_label = lv_label_create(scr);
    lv_label_set_text_fmt(time_label, "3:38PM");
    lv_obj_set_style_text_font(time_label, &Thermostat50, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -20, 5);

    fan_label = lv_label_create(scr);
    lv_label_set_text_fmt(fan_label, "_%s", fan_modes[fan_mode]);
    lv_obj_set_style_text_font(fan_label, &Thermostat50, 0);
    lv_obj_align(fan_label, LV_ALIGN_TOP_RIGHT, -20, 65);

    mode_label = lv_label_create(scr);
    lv_label_set_text_fmt(mode_label, "%s", hvac_modes[hvac_mode]);
    lv_obj_set_style_text_font(mode_label, &Thermostat50, 0);
    lv_obj_align(mode_label, LV_ALIGN_BOTTOM_RIGHT, -20, -5);

}

void ui_render_loop()
{
    if (!screenDirty) return;
    uint32_t now = millis();
    if (now - lastRender < FRAME_INTERVAL) return;
    lastRender = now;
    screenDirty = false;
    lv_timer_handler();
}

void ui_handle_inactivity()
{
    if (!inactivity_refresh_completed && millis() - last_activity > 60000) {
        Serial.println("Inactivity full refresh");
        ui_full_refresh();
        inactivity_refresh_completed = true;
    }
}
