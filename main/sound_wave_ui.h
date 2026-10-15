#pragma once
#include "sound_wave_core.h"

// All functions require the LVGL lock; one permanent application screen.
void sound_ui_init(void);
void sound_ui_update(const sound_state_t *state, const sound_frame_t *frame,
                     float phase, bool boot, int battery, bool keys_ok);
