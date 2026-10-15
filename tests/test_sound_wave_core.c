#include "sound_wave_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static sound_analyzer_t analyzer;
static int16_t pcm[SOUND_FFT_SIZE];

static sound_frame_t tone(float hz, float amplitude, float dc) {
    for (unsigned i = 0; i < SOUND_FFT_SIZE; ++i)
        pcm[i] = (int16_t)(32767 * (dc + amplitude * sinf(2 * SOUND_PI * hz * i / SOUND_SAMPLE_RATE)));
    sound_frame_t frame;
    sound_analyze(&analyzer, pcm, &frame);
    return frame;
}

int main(void) {
    sound_analyzer_init(&analyzer);
    sound_frame_t frame = tone(1000, .5f, 0);
    assert(frame.valid && !frame.clipped);
    assert(fabsf(frame.hz - 1000) < 1);
    assert(fabsf(frame.dbfs - (-9.031f)) < .02f);
    float db = frame.dbfs;
    frame = tone(1000, .25f, .1f);
    assert(fabsf(frame.dbfs - db + 6.0206f) < .03f);
    assert(fabsf(frame.hz - 1000) < 1);
    const float frequencies[] = {125, 440, 2400, 6000};
    for (unsigned i = 0; i < 4; ++i) {
        frame = tone(frequencies[i], .3f, 0);
        assert(fabsf(frame.hz - frequencies[i]) < 12);
        assert(fabsf(frame.bands[0] + frame.bands[1] + frame.bands[2] - 1) < .001f);
    }
    frame = tone(1000, .0001f, 0);
    assert(frame.hz == 0);
    memset(pcm, 0, sizeof(pcm));
    sound_analyze(&analyzer, pcm, &frame);
    assert(frame.dbfs == -96 && frame.hz == 0);
    for (unsigned i = 0; i < SOUND_FFT_SIZE; ++i) pcm[i] = 12000;
    sound_analyze(&analyzer, pcm, &frame);
    assert(frame.dbfs == -96 && frame.hz == 0);
    for (unsigned i = 0; i < SOUND_FFT_SIZE; ++i) pcm[i] = i % 2 ? 32767 : -32768;
    sound_analyze(&analyzer, pcm, &frame);
    assert(frame.clipped && fabsf(frame.dbfs) < .001f);

    sound_state_t state = {0};
    sound_state_action(&state, SOUND_FULLSCREEN);
    sound_state_action(&state, SOUND_PAUSE);
    assert(state.fullscreen && state.paused && state.landscape);
    sound_state_action(&state, SOUND_DIRECTION);
    assert(state.fullscreen && !state.landscape && state.paused);
    sound_state_action(&state, SOUND_FULLSCREEN);
    assert(!state.fullscreen && state.paused);
    sound_state_action(&state, SOUND_PAUSE);
    for (unsigned i = 0; i < SOUND_MODE_COUNT; ++i) sound_state_action(&state, SOUND_NEXT);
    assert(state.mode == SOUND_SINE);
    frame = tone(2400, .7f, 0);
    db = frame.dbfs;
    float points[SOUND_CURVE_POINTS];
    state.mode = SOUND_SINE;
    for (unsigned direction = 0; direction < 2; ++direction) {
        state.landscape = direction;
        float cycles = fminf(12, fmaxf(3, 2 + log2f(frame.hz / 125 + 1) * 1.8f));
        if (direction) cycles *= 1.6f;
        sound_curve(&state, &frame, 1.5f, 0, points);
        for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) {
            float expected = sinf((float)i / (SOUND_CURVE_POINTS - 1) * 2 * SOUND_PI * cycles + 1.5f)
                             * sound_amplitude(frame.dbfs) * .8f;
            assert(fabsf(points[i] - expected) < .0001f);
        }
    }
    for (unsigned mode = 0; mode < SOUND_MODE_COUNT; ++mode) {
        state.mode = mode;
        for (unsigned direction = 0; direction < 2; ++direction) {
            state.landscape = direction;
            for (unsigned layer = 0; layer < 5; ++layer) {
                sound_curve(&state, &frame, 3.1f, layer, points);
                for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i)
                    assert(isfinite(points[i]) && fabsf(points[i]) <= .90001f);
            }
        }
    }
    state.mode = SOUND_MIRROR;
    float upper[SOUND_CURVE_POINTS], lower[SOUND_CURVE_POINTS];
    sound_curve(&state, &frame, 0, 0, upper);
    sound_curve(&state, &frame, 0, 1, lower);
    for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) assert(fabsf(upper[i] + lower[i]) < 1e-6f);
    assert(frame.dbfs == db);
    // Same-frequency input twice as loud must produce twice the curve height in every style.
    sound_frame_t quiet = tone(1000, .025f, 0);
    sound_frame_t loud = tone(1000, .05f, 0);
    assert(fabsf(loud.dbfs - quiet.dbfs - 6.0206f) < .03f);
    assert(fabsf(sound_amplitude(loud.dbfs) / sound_amplitude(quiet.dbfs) - 2) < .01f);
    for (unsigned mode = 0; mode < SOUND_MODE_COUNT; ++mode) {
        state.mode = mode;
        sound_curve(&state, &quiet, 1.5f, 0, upper);
        sound_curve(&state, &loud, 1.5f, 0, lower);
        float small_peak = 0, large_peak = 0;
        for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) {
            small_peak = fmaxf(small_peak, fabsf(upper[i]));
            large_peak = fmaxf(large_peak, fabsf(lower[i]));
        }
        assert(large_peak > small_peak && fabsf(large_peak / small_peak - 2) < .03f);
    }
    // Tone recovery remains useful at normal quiet-input levels with the fixed-point FFT.
    for (unsigned i = 0; i < 4; ++i) {
        frame = tone(frequencies[i], .004f, 0);
        assert(fabsf(frame.hz - frequencies[i]) < 15);
    }
    frame.valid = false;
    sound_curve(&state, &frame, 1, 0, points);
    for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) assert(points[i] == 0);
    puts("Sound Wave core: PASS (FFT, RMS, DC, clipping, controls, curve bounds)");
    return 0;
}
