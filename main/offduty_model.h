#pragma once
#include "offduty_version.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t start_min, end_min, sleep_min, idle_sec;
    uint8_t brightness, volume, sleep_mode, random_eggs;
    uint16_t wait_min;
} od_settings_t;

typedef enum { OD_WAIT, OD_BEFORE, OD_WORK, OD_FINAL, OD_DONE } od_phase_t;
typedef struct {
    od_phase_t phase;
    int64_t start_utc, end_utc, remaining_us;
    int percent;
} od_time_t;

typedef enum { OD_READY, OD_NEED_WIFI, OD_NEED_TIME, OD_NEED_PLAN } od_setup_t;
od_setup_t od_setup_state(bool connected, bool synced, bool plan_ready);

void od_settings_default(od_settings_t *settings);
bool od_settings_valid(const od_settings_t *settings);
void od_wait_link(od_settings_t *settings);
int od_wait_percent(int64_t remaining_us,int start_min,int wait_min);
int od_wait_step(const od_settings_t *settings,int direction);
bool od_settings_blob(const void *blob,size_t size,od_settings_t *settings);
int64_t od_wait_boundary(int64_t end,int wait_min);
int od_sleep_choice(const od_settings_t *settings);
void od_sleep_set(od_settings_t *settings, int choice);
void od_sleep_migrate(od_settings_t *settings);
int od_shift_step(int minute, int direction);
bool od_settings_form(const char *body, od_settings_t *settings, char token[33]);
bool od_phone_time_form(const char *body, const char *token, int64_t *utc_us);
int64_t od_clock_utc(int64_t utc_anchor, int64_t mono_anchor, int64_t now);
bool od_completion_due(const od_time_t *time, int64_t last_end);
enum { OD_BOOT_MS = 3000 };
int od_boot_age(int64_t started_us,int64_t now_us);
bool od_control_form(const char *body, bool *forget, const char *token);
od_time_t od_time_at(int64_t utc_us, const od_settings_t *settings);
od_time_t od_time_between(int64_t utc_us,int64_t start,int64_t end);
od_time_t od_time_keep_finished(int64_t utc_us,od_time_t current,od_time_t previous,bool finished,int wait_min);
int64_t od_resume_end(int64_t utc_us,int minute);
bool od_resume_applies(int64_t utc_us,int64_t start,int64_t end,od_time_t scheduled);
enum { OD_OVERTIME_MAX_MIN = 480 };
int od_overtime_step(int minutes,int direction);
bool od_overtime_valid(int64_t start,int64_t end);
bool od_overtime_applies(int64_t utc_us,int64_t start,int64_t end,int64_t scheduled_start,int wait_min);
bool od_form_parse(const char *body, char ssid[33], char password[65], char token[33]);
int od_next_egg(int previous, uint32_t random);
int od_next_relax(int previous, uint32_t random);
int od_break_age(int64_t started_us, int64_t now_us);
int64_t od_free_time(int64_t now_us, int64_t scheduled_end, int64_t early_end, int64_t early_at_us);
bool od_press_is_short(int64_t started_us, int64_t released_us);
enum { OD_HOLD_MS = 5000 };
int od_hold_remaining(int64_t started_us, int64_t now_us, bool still_down);
int od_confetti_age(int64_t started_us, int64_t now_us);
bool od_sleep_due(const od_settings_t *settings, int64_t idle_us, int local_min,
                  od_phase_t phase, bool woke_this_day);
