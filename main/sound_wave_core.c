#include "sound_wave_core.h"
#include <math.h>
#include <string.h>

static float clamp(float value, float low, float high) {
    return fminf(high, fmaxf(low, value));
}

void sound_state_action(sound_state_t *state, sound_action_t action) {
    switch (action) {
    case SOUND_NEXT: state->mode = (state->mode + 1) % SOUND_MODE_COUNT; break;
    case SOUND_DIRECTION: state->landscape = !state->landscape; break;
    case SOUND_PAUSE: state->paused = !state->paused; break;
    case SOUND_FULLSCREEN:
        state->fullscreen = !state->fullscreen;
        state->landscape = state->fullscreen;
        break;
    }
}

void sound_analyzer_init(sound_analyzer_t *a) {
    memset(a, 0, sizeof(*a));
    for (unsigned i = 0; i < SOUND_FFT_SIZE; ++i)
        a->window[i] = (int16_t)lroundf((.5f - .5f * cosf(2 * SOUND_PI * i / (SOUND_FFT_SIZE - 1))) * 32767);
    for (unsigned i = 0; i < SOUND_FFT_SIZE / 2; ++i) {
        a->twiddle_real[i] = (int16_t)lroundf(cosf(2 * SOUND_PI * i / SOUND_FFT_SIZE) * 32767);
        a->twiddle_imag[i] = (int16_t)lroundf(-sinf(2 * SOUND_PI * i / SOUND_FFT_SIZE) * 32767);
    }
}

void sound_analyze(sound_analyzer_t *a, const int16_t *pcm, sound_frame_t *f) {
    memset(f, 0, sizeof(*f));
    int64_t sum = 0;
    uint64_t energy = 0;
    for (unsigned i = 0; i < SOUND_FFT_SIZE; ++i) sum += pcm[i];
    const int32_t mean = (int32_t)(sum / SOUND_FFT_SIZE);
    for (unsigned i = 0; i < SOUND_FFT_SIZE; ++i) {
        const int32_t sample = pcm[i] - mean;
        energy += (int64_t)sample * sample;
        f->clipped |= pcm[i] >= 32760 || pcm[i] <= -32760;
        a->real[i] = (int32_t)(((int64_t)sample * a->window[i]) >> 15);
        a->imag[i] = 0;
    }
    // RMS relative to the maximum PCM code; a full-scale sine is -3.01 dBFS.
    const float mean_energy = (float)energy / SOUND_FFT_SIZE;
    f->dbfs = fmaxf(-96, 10 * log10f(fmaxf(mean_energy / (32768.0f * 32768.0f), 1e-12f)));
    f->valid = true;
    for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) {
        unsigned start = i * SOUND_FFT_SIZE / SOUND_CURVE_POINTS;
        unsigned end = (i + 1) * SOUND_FFT_SIZE / SOUND_CURVE_POINTS;
        uint64_t local = 0;
        int32_t signed_peak = 0;
        for (unsigned j = start; j < end; ++j) {
            const int32_t sample = pcm[j] - mean;
            local += (int64_t)sample * sample;
            if (sample * (int64_t)sample > signed_peak * (int64_t)signed_peak) signed_peak = sample;
        }
        // Absolute local RMS: never normalize a quiet block into a loud-looking envelope.
        f->envelope[i] = sqrtf((float)local / (end - start)) / 32768.0f;
        f->waveform[i] = signed_peak;
    }
    for (unsigned i = 1, j = 0; i < SOUND_FFT_SIZE; ++i) {
        unsigned bit = SOUND_FFT_SIZE >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) {
            int32_t temp = a->real[i]; a->real[i] = a->real[j]; a->real[j] = temp;
        }
    }
    for (unsigned length = 2; length <= SOUND_FFT_SIZE; length <<= 1) {
        for (unsigned start = 0; start < SOUND_FFT_SIZE; start += length) {
            for (unsigned j = 0; j < length / 2; ++j) {
                const unsigned k = j * SOUND_FFT_SIZE / length;
                const unsigned even = start + j, odd = even + length / 2;
                int32_t re = (int32_t)(((int64_t)a->real[odd] * a->twiddle_real[k] -
                                         (int64_t)a->imag[odd] * a->twiddle_imag[k]) >> 15);
                int32_t im = (int32_t)(((int64_t)a->real[odd] * a->twiddle_imag[k] +
                                         (int64_t)a->imag[odd] * a->twiddle_real[k]) >> 15);
                // Divide each radix-2 stage by two to retain bounded fixed-point range.
                a->real[odd] = (a->real[even] - re) / 2;
                a->imag[odd] = (a->imag[even] - im) / 2;
                a->real[even] = (a->real[even] + re) / 2;
                a->imag[even] = (a->imag[even] + im) / 2;
            }
        }
    }
    uint64_t peak = 0, total = 0, bands[3] = {0};
    unsigned bin = 0;
    for (unsigned i = 1; i < SOUND_FFT_SIZE / 2; ++i) {
        const uint64_t power = (int64_t)a->real[i] * a->real[i] + (int64_t)a->imag[i] * a->imag[i];
        total += power;
        unsigned band = i < 16 ? 0 : i < 64 ? 1 : 2;
        bands[band] += power;
        if (i >= 2 && power > peak) { peak = power; bin = i; }
    }
    for (unsigned i = 0; i < 3; ++i) f->bands[i] = total ? (float)bands[i] / (float)total : 0;
    // Reject silence and a spectrum without a pronounced peak instead of inventing a pitch.
    if (f->dbfs > -60 && bin && peak * 1000 > total * 35) {
        const float left = (float)((int64_t)a->real[bin-1] * a->real[bin-1] + (int64_t)a->imag[bin-1] * a->imag[bin-1]);
        const float right = (float)((int64_t)a->real[bin+1] * a->real[bin+1] + (int64_t)a->imag[bin+1] * a->imag[bin+1]);
        const float denominator = left - 2 * (float)peak + right;
        float offset = fabsf(denominator) > 1e-12f ? .5f * (left - right) / denominator : 0;
        f->hz = (bin + clamp(offset, -.5f, .5f)) * SOUND_SAMPLE_RATE / SOUND_FFT_SIZE;
    }
}

float sound_amplitude(float dbfs) {
    // Fixed 16x visual gain: +6.02 dB doubles height before screen clipping, never frame AGC.
    return clamp(16 * powf(10, dbfs / 20), 0, 1);
}

void sound_curve(const sound_state_t *s, const sound_frame_t *f,
                 float phase, unsigned layer, float *points) {
    if (s->mode != SOUND_SINE) {
        for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) {
            float value = 0;
            if (f->valid) value = s->mode == SOUND_RAW ? f->waveform[i] / 32768.0f * 16 : f->envelope[i] * 16;
            if (s->mode == SOUND_MIRROR && layer == 1) value = -value;
            points[i] = clamp(value, -.9f, .9f);
        }
        return;
    }
    const float amplitude = f->valid ? sound_amplitude(f->dbfs) : 0;
    float cycles = f->hz > 0 ? clamp(2 + log2f(f->hz / 125 + 1) * 1.8f, 3, 12) : 3;
    if (s->landscape) cycles *= 1.6f;
    const float step = 2 * SOUND_PI * cycles / (SOUND_CURVE_POINTS - 1);
    float wave = sinf(phase), wave_cos = cosf(phase);
    const float ws = sinf(step), wc = cosf(step);
    for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) {
        float value = wave * amplitude * .8f;
        points[i] = clamp(value, -.9f, .9f);
        // Recurrence avoids hundreds of software trig calls per refresh on the C3.
        float next = wave * wc + wave_cos * ws;
        wave_cos = wave_cos * wc - wave * ws; wave = next;
    }
}
