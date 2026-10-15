#pragma once
#include "sound_wave_core.h"

typedef struct {
    int width, height;
    sound_mode_t mode;
    int16_t y[2][SOUND_CURVE_POINTS];
    int peak_pixels;
    // Cached geometry is rebuilt only for a style or size change, never per strip.
    unsigned background_count;
    int16_t background[320][4];
} sound_plot_t;

// Initialize plot to zero before its first use.
void sound_plot_prepare(sound_plot_t *plot, const sound_state_t *state,
                        const sound_frame_t *frame, float phase, int width, int height);
// Render any clipped strip. Stride is in pixels; no dynamic allocations or LVGL dependency.
void sound_plot_strip(const sound_plot_t *plot, int x, int y, int width, int height,
                      uint16_t *pixels, int stride);
uint32_t sound_plot_background(sound_mode_t mode);
uint32_t sound_plot_foreground(sound_mode_t mode);
