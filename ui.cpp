#include "ui.h"
#include <lvgl.h>
#include "EPD.h"

extern lv_font_t Roboto32;
extern lv_font_t Roboto37;

#define SCREEN_WIDTH 792
#define SCREEN_HEIGHT 272
#define MAX_VISIBLE_TRACKS 5

static uint8_t ImageBW[27200];

/* draw buffer */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 40];

/* LVGL objects */
static lv_obj_t *album_label;
static lv_obj_t *now_playing_label;
static lv_obj_t *artwork;
static lv_obj_t *track1_label;
static lv_obj_t *track2_label;
static lv_obj_t *track3_label;
static lv_obj_t *track4_label;
static lv_obj_t *track5_label;

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

void ui_setText(const char* text)
{
    lv_label_set_text_fmt(album_label, "%s", text);
}

void ui_updateNowPlayingTracks(String titles[], int selectedIndex) {

    lv_label_set_text_fmt(track1_label, "%s%s",selectedIndex==0 ?">":"  " , titles[0].c_str());
    lv_label_set_text_fmt(track2_label, "%s%s",selectedIndex==1 ?">":"  " , titles[1].c_str());
    lv_label_set_text_fmt(track3_label, "%s%s",selectedIndex==2 ?">":"  " , titles[2].c_str());
    lv_label_set_text_fmt(track4_label, "%s%s",selectedIndex==3 ?">":"  " , titles[3].c_str());
    lv_label_set_text_fmt(track5_label, "%s%s",selectedIndex==4 ?">":"  " , titles[4].c_str());
    last_activity = millis();
    screenDirty = true;
}
lv_img_dsc_t artwork_dsc;
void ui_updateArtwork(uint8_t * buf){
    artwork_dsc.header.always_zero = 0;
    artwork_dsc.header.w = 256;
    artwork_dsc.header.h = 256;
    artwork_dsc.header.cf = LV_IMG_CF_ALPHA_1BIT;  // correct for v8

    artwork_dsc.data_size = 256 * 256 / 8;
    artwork_dsc.data = buf;

    lv_img_set_src(artwork, &artwork_dsc);
    inactivity_refresh_completed = false;
}

void ui_full_refresh()
{

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

    // Root container
    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_remove_style_all(container);
    lv_obj_set_style_pad_top(container, 8, 0);
    lv_obj_set_style_pad_left(container, 12, 0);
    lv_obj_set_style_pad_gap(container, 8, 0);
    lv_obj_set_size(container, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // --------------------
    // LEFT: ARTWORK
    // --------------------
    artwork = lv_img_create(container);
    lv_obj_set_size(artwork, 256, 256); // slightly smaller for padding

    // --------------------
    // RIGHT: TEXT AREA
    // --------------------
    lv_obj_t *text_area = lv_obj_create(container);
    lv_obj_remove_style_all(text_area);
    lv_obj_set_style_pad_gap(text_area, 4, 8);
    lv_obj_set_width(text_area, SCREEN_WIDTH - 256);
    lv_obj_set_height(text_area, SCREEN_HEIGHT);
    lv_obj_set_flex_flow(text_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(text_area, 1);
    // --------------------
    // Artist
    // --------------------
    album_label = lv_label_create(text_area);
    lv_obj_set_style_text_font(album_label, &Roboto37, 0);
    lv_obj_set_style_pad_bottom(album_label, 32, 0);
    lv_obj_set_style_pad_left(album_label, 16, 0);
    lv_obj_set_width(album_label, LV_PCT(100));
    lv_label_set_text(album_label, "Insert a Cartridge");

    // --------------------
    // Divider
    // --------------------
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 4);
    lv_style_set_line_rounded(&style_line, true);

    static lv_point_t line_points[] = { {288, 64}, {746, 64} };
    lv_obj_t * line1 = lv_line_create(scr);
    lv_obj_add_style(line1, &style_line, 0);
    lv_line_set_points(line1, line_points, 2);



    lv_obj_t *list = lv_obj_create(text_area);
    lv_obj_remove_style_all(list);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    track1_label= lv_label_create(list);
    lv_obj_set_style_text_font(track1_label, &Roboto32, 0);
    lv_label_set_text(track1_label, "");
    track2_label= lv_label_create(list);
    lv_obj_set_style_text_font(track2_label, &Roboto32, 0);
    lv_label_set_text(track2_label, "");
    track3_label= lv_label_create(list);
    lv_obj_set_style_text_font(track3_label, &Roboto32, 0);
    lv_label_set_text(track3_label, "");
    track4_label= lv_label_create(list);
    lv_obj_set_style_text_font(track4_label, &Roboto32, 0);
    lv_label_set_text(track4_label, "");
    track5_label= lv_label_create(list);
    lv_obj_set_style_text_font(track5_label, &Roboto32, 0);
    lv_label_set_text(track5_label, "");
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
        ui_full_refresh();
        inactivity_refresh_completed = true;
    }
}
