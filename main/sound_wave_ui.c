#include "sound_wave_ui.h"
#include "sound_wave_raster.h"
#include "lvgl.h"
// LVGL 9.5 custom decoder callbacks expose their session fields through this extension header.
#include "src/draw/lv_image_decoder_private.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include <stdlib.h>

#if LV_DRAW_SW_DRAW_UNIT_CNT != 1
#error "Sound Wave strip buffer requires one software draw unit"
#endif

LV_FONT_DECLARE(sound_font_14);
LV_FONT_DECLARE(sound_font_22);
static lv_obj_t *s_screen, *s_art, *s_title, *s_battery, *s_hz, *s_db, *s_hint;
static lv_obj_t *s_boot_title, *s_boot_caption;
static sound_plot_t s_plot;
static uint8_t s_marker;
static lv_image_dsc_t s_image;
static lv_draw_buf_t s_draw_buf;
static _Alignas(4) uint16_t s_strip[320 * 8];
static const char *NAMES[] = {"正弦", "镜像", "原始"};
static bool s_layout_ready, s_last_boot, s_drew_wave;
static sound_state_t s_last_state;
static int64_t s_refresh_start, s_report_start;
static unsigned s_refreshes;
static uint64_t s_render_us;
static uint64_t s_raster_us, s_raster_frame_us;
static int s_level_x10, s_last_hz = -1, s_last_db = 1, s_last_battery = -2;
static int s_last_status = -1;

static lv_result_t image_info(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc,
                               lv_image_header_t *header) {
    (void)decoder;
    if (dsc->src != &s_image) return LV_RESULT_INVALID;
    *header = s_image.header;
    header->cf = LV_COLOR_FORMAT_RGB565;
    header->stride = s_plot.width * 2;
    return LV_RESULT_OK;
}
static lv_result_t image_open(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc) {
    (void)decoder;
    if (dsc->src != &s_image) return LV_RESULT_INVALID;
    dsc->decoded = NULL;
    dsc->args.no_cache = true;
    return LV_RESULT_OK;
}
static lv_result_t image_area(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc,
                              const lv_area_t *full, lv_area_t *area) {
    (void)decoder;
    if (area->y1 == LV_COORD_MIN) { *area = *full; }
    else area->y1 = area->y2 + 1;
    if (area->y1 > full->y2) return LV_RESULT_INVALID;
    area->y2 = LV_MIN(area->y1 + 7, full->y2);
    int width = lv_area_get_width(area), height = lv_area_get_height(area);
    if (width <= 0 || width > 320 || height <= 0 || height > 8) return LV_RESULT_INVALID;
    if (lv_draw_buf_init(&s_draw_buf, width, height, LV_COLOR_FORMAT_RGB565,
                         width * 2, s_strip, sizeof(s_strip)) != LV_RESULT_OK) return LV_RESULT_INVALID;
    int64_t started = esp_timer_get_time();
    sound_plot_strip(&s_plot, area->x1, area->y1, width, height, s_strip, width);
    s_raster_frame_us += esp_timer_get_time() - started;
    dsc->decoded = &s_draw_buf;
    return LV_RESULT_OK;
}
static void image_close(lv_image_decoder_t *decoder, lv_image_decoder_dsc_t *dsc) {
    (void)decoder; (void)dsc;
}
static void draw_event(lv_event_t *event) { (void)event; s_drew_wave = true; }
static void refresh_event(lv_event_t *event) {
    int64_t now = esp_timer_get_time();
    if (lv_event_get_code(event) == LV_EVENT_REFR_START) {
        s_refresh_start = now; s_drew_wave = false; s_raster_frame_us = 0;
    } else if (s_drew_wave) {
        ++s_refreshes; s_render_us += now - s_refresh_start;
        s_raster_us += s_raster_frame_us;
        if (now - s_report_start >= 5LL * 1000 * 1000) {
            ESP_LOGI("sound_ui", "Refresh: mode=%d rate_x10=%u average_us=%llu raster_us=%llu dbfs_x10=%d peak_px=%d",
                     s_plot.mode, (unsigned)(s_refreshes * 10000000LL / (now - s_report_start)),
                     (unsigned long long)(s_render_us / s_refreshes),
                     (unsigned long long)(s_raster_us / s_refreshes), s_level_x10, s_plot.peak_pixels);
            s_refreshes = 0; s_render_us = 0; s_raster_us = 0; s_report_start = now;
        }
    }
}
static lv_obj_t *label(int x, int y, const lv_font_t *font) {
    lv_obj_t *obj = lv_label_create(s_screen);
    lv_obj_set_pos(obj, x, y); lv_obj_set_style_text_font(obj, font, 0);
    return obj;
}
static void visibility(lv_obj_t *obj, bool visible) {
    if (visible) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void sound_ui_init(void) {
    const lv_font_t *fonts[] = {&sound_font_14, &sound_font_22};
    const char *inventory = "声波正弦镜像原始实时暂停长按全屏麦克错误过载看见声音键";
    for (unsigned f = 0; f < 2; ++f) {
        uint32_t index = 0;
        while (inventory[index]) {
            const unsigned char *b = (const unsigned char *)inventory + index;
            uint32_t codepoint = ((b[0] & 15) << 12) | ((b[1] & 63) << 6) | (b[2] & 63);
            index += 3;
            lv_font_glyph_dsc_t glyph = {0};
            if (!lv_font_get_glyph_dsc(fonts[f], &glyph, codepoint, 0) || glyph.is_placeholder)
                ESP_LOGE("sound_font", "Missing glyph U+%04lx", (unsigned long)codepoint);
        }
    }
    lv_image_decoder_t *decoder = lv_image_decoder_create();
    LV_ASSERT_NULL(decoder);
    lv_image_decoder_set_info_cb(decoder, image_info);
    lv_image_decoder_set_open_cb(decoder, image_open);
    lv_image_decoder_set_get_area_cb(decoder, image_area);
    lv_image_decoder_set_close_cb(decoder, image_close);
    s_image.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_image.header.cf = LV_COLOR_FORMAT_RAW;
    s_image.data = &s_marker; s_image.data_size = 1;
    s_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    s_art = lv_image_create(s_screen);
    lv_obj_add_event_cb(s_art, draw_event, LV_EVENT_DRAW_MAIN, NULL);
    s_title = label(20, 24, &sound_font_14);
    s_battery = label(182, 24, &lv_font_montserrat_14);
    s_hz = label(20, 265, &lv_font_montserrat_14);
    s_db = label(119, 265, &lv_font_montserrat_14);
    s_hint = label(24, 289, &sound_font_14);
    lv_label_set_text(s_hint, "长按OK全屏");
    s_boot_title = label(0, 82, &sound_font_22);
    lv_label_set_text(s_boot_title, "声波");
    s_boot_caption = label(0, 246, &sound_font_14);
    lv_label_set_text(s_boot_caption, "看见声音");
    lv_obj_set_width(s_boot_title, 240); lv_obj_set_width(s_boot_caption, 240);
    lv_obj_set_style_text_align(s_boot_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(s_boot_caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_screen_load(s_screen);
    s_report_start = esp_timer_get_time();
    lv_display_add_event_cb(lv_display_get_default(), refresh_event, LV_EVENT_REFR_START, NULL);
    lv_display_add_event_cb(lv_display_get_default(), refresh_event, LV_EVENT_REFR_READY, NULL);
}

void sound_ui_update(const sound_state_t *s, const sound_frame_t *f,
                     float phase, bool boot, int battery, bool keys_ok) {
    lv_display_t *display = lv_display_get_default();
    lv_display_rotation_t rotation = !boot && s->landscape ? LV_DISPLAY_ROTATION_90 : LV_DISPLAY_ROTATION_0;
    if (lv_display_get_rotation(display) != rotation) lv_display_set_rotation(display, rotation);
    int sw = lv_display_get_horizontal_resolution(display), sh = lv_display_get_vertical_resolution(display);
    bool layout = !s_layout_ready || s_last_boot != boot || s_last_state.mode != s->mode ||
                  s_last_state.fullscreen != s->fullscreen || s_last_state.landscape != s->landscape;
    sound_state_t render = *s;
    sound_frame_t intro = {.valid = true, .hz = 125, .dbfs = -28};
    if (boot) { render.mode = SOUND_SINE; f = &intro; }
    int width = sw - (s->fullscreen && !boot ? 8 : 24);
    int height = boot ? 100 : s->fullscreen ? sh - 44 : sh - 130;
    if (layout) {
        uint32_t ink = 0xE8F1F4;
        lv_obj_set_style_bg_color(s_screen, lv_color_hex(sound_plot_background(render.mode)), 0);
        lv_obj_t *labels[] = {s_title, s_battery, s_hz, s_db, s_hint, s_boot_title, s_boot_caption};
        for (unsigned i = 0; i < sizeof(labels) / sizeof(labels[0]); ++i)
            lv_obj_set_style_text_color(labels[i], lv_color_hex(ink), 0);
        lv_obj_set_style_text_color(s_title, lv_color_hex(sound_plot_foreground(render.mode)), 0);
        bool info = !boot && !s->fullscreen;
        visibility(s_title, info); visibility(s_battery, info); visibility(s_hint, info);
        visibility(s_hz, !boot); visibility(s_db, !boot);
        visibility(s_boot_title, boot); visibility(s_boot_caption, boot);
        lv_obj_set_pos(s_art, s->fullscreen && !boot ? 4 : 12, boot ? 124 : s->fullscreen ? 12 : 58);
        lv_obj_set_pos(s_battery, sw - 56, 24);
        lv_obj_set_pos(s_hz, 20, sh - (s->fullscreen ? 27 : 55));
        lv_obj_set_pos(s_db, sw - 115, sh - (s->fullscreen ? 27 : 55));
        lv_obj_set_y(s_hint, sh - 28);
        s_last_hz = -1; s_last_db = 1; s_last_battery = -2;
    }
    if (boot || !s->paused || layout) {
        sound_plot_prepare(&s_plot, &render, f, phase, width, height);
        s_level_x10 = (int)lroundf(f->dbfs * 10);
        if (layout) {
            lv_image_cache_drop(&s_image);
            s_image.header.w = width; s_image.header.h = height;
            lv_image_set_src(s_art, &s_image);
        }
        lv_obj_invalidate(s_art);
    }
    if (!boot) {
        bool changed = layout || s_last_state.paused != s->paused;
        int status = !keys_ok ? 0 : !f->valid ? 1 : s->paused ? 2 : f->clipped ? 3 : 4;
        if (!s->fullscreen && (changed || battery != s_last_battery || status != s_last_status)) {
            lv_label_set_text_fmt(s_title, "%s / %s", NAMES[s->mode], !keys_ok ? "按键错误" :
                                  !f->valid ? "麦克错误" : s->paused ? "暂停" : f->clipped ? "过载" : "实时");
            if (battery >= 0) lv_label_set_text_fmt(s_battery, "%d%%", battery);
            else lv_label_set_text(s_battery, "--%");
            s_last_battery = battery;
            s_last_status = status;
        }
        int hz = f->valid && f->hz > 0 ? (int)lroundf(f->hz) : 0;
        int db = f->valid ? (int)lroundf(f->dbfs * 10) : 1;
        if (hz != s_last_hz) {
            if (hz) lv_label_set_text_fmt(s_hz, "%d Hz", hz); else lv_label_set_text(s_hz, "-- Hz");
            s_last_hz = hz;
        }
        if (db != s_last_db || layout) {
            int a = abs(db);
            if (f->valid) lv_label_set_text_fmt(s_db, "%s%d.%d dBFS", a ? "-" : "", a / 10, a % 10);
            else lv_label_set_text(s_db, "-- dBFS");
            s_last_db = db;
        }
    }
    s_layout_ready = true; s_last_boot = boot; s_last_state = *s;
}
