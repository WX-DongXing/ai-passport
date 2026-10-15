#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SOUND_SAMPLE_RATE 16000
#define SOUND_FFT_SIZE 512
#define SOUND_CURVE_POINTS 81
#define SOUND_PI 3.14159265358979323846f

typedef enum { SOUND_SINE, SOUND_MIRROR, SOUND_RAW, SOUND_MODE_COUNT } sound_mode_t;
typedef enum { SOUND_NEXT, SOUND_DIRECTION, SOUND_PAUSE, SOUND_FULLSCREEN } sound_action_t;

typedef struct {
    sound_mode_t mode;
    bool paused;
    bool fullscreen;
    bool landscape;
} sound_state_t;

typedef struct {
    bool valid;
    bool clipped;
    float hz;
    float dbfs;
    float bands[3];
    float envelope[SOUND_CURVE_POINTS];
    int32_t waveform[SOUND_CURVE_POINTS];
} sound_frame_t;

typedef struct {
    int32_t real[SOUND_FFT_SIZE], imag[SOUND_FFT_SIZE];
    int16_t window[SOUND_FFT_SIZE];
    int16_t twiddle_real[SOUND_FFT_SIZE / 2], twiddle_imag[SOUND_FFT_SIZE / 2];
} sound_analyzer_t;

void sound_state_action(sound_state_t *state, sound_action_t action);
void sound_analyzer_init(sound_analyzer_t *analyzer);
void sound_analyze(sound_analyzer_t *analyzer, const int16_t *pcm, sound_frame_t *frame);
float sound_amplitude(float dbfs);
// Normalized -1..1 coordinates, independent of display size and LVGL.
void sound_curve(const sound_state_t *state, const sound_frame_t *frame,
                 float phase, unsigned layer, float *points);
