#include "sound_wave_core.h"
#include "sound_wave_ui.h"
#include "key_guard.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "sound_wave";
static QueueHandle_t s_input_queue, s_frame_queue;
static volatile bool s_input_ready;
static key_guard_t s_key_guard;
typedef struct { bsp_btn_t btn; bsp_btn_ev_t event; } input_event_t;
// One owner for audio and analysis; no long recording or full-screen bitmap.
static sound_analyzer_t s_analyzer;

static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user) {
    (void)user;
    if (key_guard_consume(&s_key_guard, ev == BSP_BTN_RELEASE, esp_timer_get_time())) return;
    if (!s_input_ready) return;
    if (ev != BSP_BTN_CLICK && ev != BSP_BTN_LONG) return;
    const input_event_t input = {.btn = btn, .event = ev};
    (void)xQueueSend(s_input_queue, &input, 0);
}

static void audio_task(void *arg) {
    (void)arg;
    int16_t pcm[SOUND_FFT_SIZE];
    sound_frame_t frame;
    bool ready = false;
    int64_t report_at = 0;
    sound_analyzer_init(&s_analyzer);
    for (;;) {
        if (!ready) {
            ready = bsp_audio_init() == ESP_OK &&
                    bsp_audio_set_format(SOUND_SAMPLE_RATE, 16, 1) == ESP_OK;
            if (ready) bsp_audio_set_volume(0);
        }
        if (ready && bsp_audio_read(pcm, sizeof(pcm)) == ESP_OK) {
            int64_t analysis_start = esp_timer_get_time();
            sound_analyze(&s_analyzer, pcm, &frame);
            xQueueOverwrite(s_frame_queue, &frame);
            int64_t now = esp_timer_get_time();
            if (now >= report_at) {
                ESP_LOGI(TAG, "Capture: hz=%d dbfs_x10=%d amplitude_x1000=%d analysis_us=%lld heap=%lu",
                         (int)lroundf(frame.hz), (int)lroundf(frame.dbfs * 10),
                         (int)lroundf(sound_amplitude(frame.dbfs) * 1000),
                         (long long)(now - analysis_start), (unsigned long)esp_get_free_heap_size());
                report_at = now + 5LL * 1000 * 1000;
            }
            // A queued I2S read can return immediately; always give lower priorities CPU time.
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            frame = (sound_frame_t){0};
            xQueueOverwrite(s_frame_queue, &frame);
            ESP_LOGW(TAG, "Microphone unavailable; retrying in one second");
            ready = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Sound Wave starting: 16k mono, 512-point FFT, RMS dBFS");
    if (esp_sleep_get_wakeup_causes() & (1u << ESP_SLEEP_WAKEUP_GPIO))
        key_guard_arm(&s_key_guard, esp_timer_get_time(), 10LL * 1000 * 1000);
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }
    s_input_queue = xQueueCreate(8, sizeof(input_event_t));
    s_frame_queue = xQueueCreate(1, sizeof(sound_frame_t));
    if (!s_input_queue || !s_frame_queue) {
        if (s_input_queue) vQueueDelete(s_input_queue);
        if (s_frame_queue) vQueueDelete(s_frame_queue);
        ESP_LOGE(TAG, "Queue allocation failed");
        return;
    }
    sound_state_t state = {0};
    sound_frame_t displayed = {0}, latest;
    float phase = 0;
    int battery = -1;
    if (!bsp_lvgl_lock(1000)) {
        vQueueDelete(s_input_queue); vQueueDelete(s_frame_queue);
        ESP_LOGE(TAG, "Initial display lock failed");
        return;
    }
    sound_ui_init();
    sound_ui_update(&state, &displayed, phase, true, battery, true);
    bsp_lvgl_unlock();
    bsp_display_backlight(80);
    const int64_t started = esp_timer_get_time();
    const bool keys_ok = bsp_button_init(on_key, NULL) == ESP_OK;
    if (s_key_guard.armed)
        key_guard_sync_released(&s_key_guard, bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN);
    s_input_ready = keys_ok;
    const bool battery_ok = bsp_battery_init() == ESP_OK;
    if (xTaskCreate(audio_task, "sound_capture", 6144, NULL, 4, NULL) != pdPASS)
        ESP_LOGE(TAG, "Capture task creation failed");
    int64_t last = started, battery_at = 0;
    for (;;) {
        int64_t now = esp_timer_get_time();
        bool boot = now - started < 1400 * 1000;
        input_event_t input;
        for (unsigned i = 0; i < 8 && xQueueReceive(s_input_queue, &input, 0) == pdTRUE; ++i) {
            if (boot) continue;
            if (input.btn == BSP_BTN_OK && input.event == BSP_BTN_LONG)
                sound_state_action(&state, SOUND_FULLSCREEN);
            else if (input.event == BSP_BTN_CLICK) {
                if (input.btn == BSP_BTN_OK) sound_state_action(&state, SOUND_PAUSE);
                else if (input.btn == BSP_BTN_UP) sound_state_action(&state, SOUND_NEXT);
                else if (input.btn == BSP_BTN_DOWN) sound_state_action(&state, SOUND_DIRECTION);
            }
            ESP_LOGI(TAG, "View: mode=%d paused=%d full=%d landscape=%d",
                     state.mode, state.paused, state.fullscreen, state.landscape);
        }
        // Drain even while paused; resume uses the newest frame, never a recording backlog.
        if (xQueueReceive(s_frame_queue, &latest, 0) == pdTRUE && !state.paused)
            displayed = latest;
        if (boot || !state.paused)
            phase = fmodf(phase + (now - last) / 1000000.0f * 2.5f, 2 * SOUND_PI);
        last = now;
        if (battery_ok && now >= battery_at) {
            battery = bsp_battery_soc();
            battery_at = now + 5LL * 1000 * 1000;
        }
        if (bsp_lvgl_lock(100)) {
            sound_ui_update(&state, &displayed, phase, boot, battery, keys_ok);
            bsp_lvgl_unlock();
        }
        // Do not catch up missed deadlines by spinning when software rendering takes longer.
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
