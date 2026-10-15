#include "sound_wave_raster.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sound_analyzer_t analyzer;
static int16_t pcm[SOUND_FFT_SIZE];
static uint16_t full[320 * 320], partial[320 * 320], clipped[64 * 8 + 2];
static sound_frame_t tone(float amplitude) {
    for (unsigned i = 0; i < SOUND_FFT_SIZE; ++i)
        pcm[i] = (int16_t)(32767 * amplitude * sinf(2 * SOUND_PI * 1000 * i / SOUND_SAMPLE_RATE));
    sound_frame_t frame;
    sound_analyze(&analyzer, pcm, &frame);
    return frame;
}
int main(void) {
    sound_analyzer_init(&analyzer);
    sound_frame_t quiet = tone(.02f), loud = tone(.04f), silent = tone(0);
    sound_state_t state = {0};
    sound_plot_t plot = {0}, small = {0};
    const int sizes[][2] = {{216,190},{312,196},{232,276},{296,110}};
    for (unsigned mode = 0; mode < SOUND_MODE_COUNT; ++mode) {
        state.mode = mode;
        for (unsigned size = 0; size < 4; ++size) {
            int w = sizes[size][0], h = sizes[size][1];
            state.landscape = w > h;
            sound_plot_prepare(&small, &state, &quiet, .5f, w, h);
            sound_plot_prepare(&plot, &state, &loud, .5f, w, h);
            assert(plot.background_count == small.background_count);
            assert(plot.background_count <= 320);
            assert(memcmp(plot.background, small.background,
                          plot.background_count * sizeof(plot.background[0])) == 0);
            if (mode != SOUND_RAW) assert(plot.background_count > 0);
            assert(small.peak_pixels > 0 && abs(plot.peak_pixels - 2 * small.peak_pixels) <= 2);
            sound_plot_strip(&plot, 0, 0, w, h, full, w);
            memset(partial, 0, sizeof(partial));
            for (int y = 0; y < h; y += 8)
                sound_plot_strip(&plot, 0, y, w, h-y < 8 ? h-y : 8, partial + y*w, w);
            assert(memcmp(full, partial, w*h*sizeof(uint16_t)) == 0);
            for (unsigned i = 0; i < sizeof(clipped)/sizeof(clipped[0]); ++i) clipped[i] = 0xface;
            sound_plot_strip(&plot, 11, 18, 51, 8, clipped+1, 64);
            assert(clipped[0] == 0xface && clipped[64*8+1] == 0xface);
            for (int y = 0; y < 8; ++y) {
                assert(memcmp(clipped+1+y*64, full+(18+y)*w+11, 51*sizeof(uint16_t)) == 0);
                for (int x = 51; x < 64; ++x) assert(clipped[1+y*64+x] == 0xface);
            }
            sound_plot_prepare(&plot, &state, &silent, .5f, w, h);
            assert(plot.peak_pixels == 0);
        }
    }
    assert(sound_plot_background(SOUND_SINE) != sound_plot_background(SOUND_MIRROR));
    assert(sound_plot_background(SOUND_MIRROR) != sound_plot_background(SOUND_RAW));
    puts("Sound Wave raster: PASS (pixel amplitude, strip equivalence, clip bounds, silence, both orientations)");
    return 0;
}
