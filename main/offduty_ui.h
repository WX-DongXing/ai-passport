#pragma once
#include "offduty_model.h"
#include "lvgl.h"

typedef struct {
    od_settings_t settings;
    od_time_t time;
    int64_t utc_us, animation_ms;
    bool boot;
    int boot_ms;
    int mood, egg, battery, menu, selected, edit, notice;
    int feedback; // 1: phone settings saved, 2: save failed, 3: phone time applied.
    bool connected, synced, phone_time, portal, early_done, plan_ready, phone_connected;
    int hold_remaining_ms;
    int confetti_ms;
    uint32_t confetti_seed;
    int relax, effect_ms;
    bool off_home, resume_picker, work_choice, overtime_picker, overtime;
    int work_option, overtime_min;
    int resume_end_min;
    int64_t free_us;
    char ap_name[24], ap_password[17];
} od_view_t;

// The application owns the lifetime; call both functions while holding the LVGL lock.
lv_obj_t *od_ui_create(void);
void od_ui_update(lv_obj_t *screen, const od_view_t *view);
const char *od_menu_name(int row);
#define OD_MENU_ROWS 13
