#include "sound_wave_raster.h"
#include <stdlib.h>
#include <math.h>

static const uint32_t BACKGROUNDS[] = {0x201D35, 0x292133, 0x160F1E};
static const uint32_t FOREGROUNDS[] = {0xCEB5FB, 0xE8BDE2, 0xFFA997};
static const uint32_t GRIDS[] = {0x37324D, 0x50405E, 0x403048};
uint32_t sound_plot_background(sound_mode_t m) { return BACKGROUNDS[m]; }
uint32_t sound_plot_foreground(sound_mode_t m) { return FOREGROUNDS[m]; }
static uint16_t rgb565(uint32_t c) { return ((c >> 8) & 0xf800) | ((c >> 5) & 0x7e0) | ((c >> 3) & 0x1f); }

static void background_line(sound_plot_t *p, float x1, float y1, float x2, float y2) {
    if (p->background_count >= 320) return;
    int16_t *segment = p->background[p->background_count++];
    segment[0] = (int16_t)lroundf(x1 * p->width / 320);
    segment[1] = (int16_t)lroundf(y1 * p->height / 214);
    segment[2] = (int16_t)lroundf(x2 * p->width / 320);
    segment[3] = (int16_t)lroundf(y2 * p->height / 214);
}
static void background_circle(sound_plot_t *p, float cx, float cy, float radius) {
    float x = cx + radius, y = cy;
    for (unsigned i = 1; i <= 12; ++i) {
        float angle = i * (2 * SOUND_PI / 12);
        float nx = cx + radius * cosf(angle), ny = cy + radius * sinf(angle);
        background_line(p, x, y, nx, ny); x = nx; y = ny;
    }
}
static void background_prepare(sound_plot_t *p) {
    p->background_count = 0;
    if (p->mode == SOUND_SINE) {
        const float c = .92718385f, s = -.37460659f; // -22 degree orbit tilt
        for (unsigned ring = 0; ring < 3; ++ring) {
            float ry = 27 + 11 * ring, x = 160 + 185*c, y = 106 + 185*s;
            for (unsigned i = 1; i <= 64; ++i) {
                float a = i * (2 * SOUND_PI / 64), ex = 185*cosf(a), ey = ry*sinf(a);
                float nx = 160 + ex*c - ey*s, ny = 106 + ex*s + ey*c;
                background_line(p, x, y, nx, ny); x = nx; y = ny;
            }
        }
        const int stars[][2] = {{27,39},{286,49},{44,179},{259,172}};
        for (unsigned i = 0; i < 4; ++i) {
            int x = stars[i][0], y = stars[i][1];
            background_circle(p, x, y, 2);
            background_line(p, x-5, y, x+5, y);
            background_line(p, x, y-5, x, y+5);
        }
        background_circle(p, 238, 34, 5); background_circle(p, 78, 178, 4);
    } else if (p->mode == SOUND_MIRROR) {
        for (unsigned i = 0; i < 5; ++i) {
            float edge = 14 + 18*i, top = 12 + 7*i, control = 128 - 5*i;
            float x = edge, y = top;
            for (unsigned j = 1; j <= 24; ++j) {
                float t = j/24.0f, u = 1-t;
                float nx = u*u*edge + 2*u*t*160 + t*t*(320-edge);
                float ny = (u*u+t*t)*top + 2*u*t*control;
                background_line(p, x, y, nx, ny);
                background_line(p, x, 214-y, nx, 214-ny);
                x = nx; y = ny;
            }
        }
        background_line(p,14,23,14,54); background_line(p,306,23,306,54);
        background_line(p,14,160,14,191); background_line(p,306,160,306,191);
    }
}

void sound_plot_prepare(sound_plot_t *p, const sound_state_t *s, const sound_frame_t *f,
                        float phase, int width, int height) {
    bool layout = p->width != width || p->height != height || p->mode != s->mode;
    p->width = width; p->height = height; p->mode = s->mode; p->peak_pixels = 0;
    if (layout) background_prepare(p);
    for (unsigned layer = 0; layer < (s->mode == SOUND_MIRROR ? 2u : 1u); ++layer) {
        float curve[SOUND_CURVE_POINTS];
        sound_curve(s, f, phase, layer, curve);
        for (unsigned i = 0; i < SOUND_CURVE_POINTS; ++i) {
            const int offset = (int)(curve[i] * (height / 2 - 8));
            p->y[layer][i] = height / 2 + offset;
            if (abs(offset) > p->peak_pixels) p->peak_pixels = abs(offset);
        }
    }
}

typedef struct { int x, y, w, h, stride; uint16_t *pixels; } strip_t;
static void pixel(const strip_t *s, int x, int y, uint16_t color) {
    if (x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h)
        s->pixels[(y - s->y) * s->stride + x - s->x] = color;
}
static void line(const strip_t *s, int x1, int y1, int x2, int y2, uint16_t color, int thick) {
    if ((y1 < s->y - thick && y2 < s->y - thick) ||
        (y1 >= s->y + s->h && y2 >= s->y + s->h)) return;
    const int dx = x2 - x1, dy = y2 - y1;
    // Evaluate only pixels intersecting this strip; don't retrace an entire steep segment per strip.
    bool horizontal = abs(dx) >= abs(dy);
    int a = horizontal ? x1 : y1, b = horizontal ? x2 : y2;
    int low = a < b ? a : b, high = a > b ? a : b;
    int clip_low = (horizontal ? s->x : s->y) - thick + 1;
    int clip_high = horizontal ? s->x + s->w - 1 : s->y + s->h - 1;
    if (low < clip_low) low = clip_low;
    if (high > clip_high) high = clip_high;
    for (int at = low; at <= high; ++at) {
        int x = horizontal ? at : x1 + (dy ? (at-y1)*dx/dy : 0);
        int y = horizontal ? y1 + (dx ? (at-x1)*dy/dx : 0) : at;
        pixel(s, x, y, color);
        if (thick == 2) { pixel(s, x + 1, y, color); pixel(s, x, y + 1, color); }
    }
}

void sound_plot_strip(const sound_plot_t *p, int x, int y, int width, int height,
                      uint16_t *pixels, int stride) {
    const strip_t s = {x, y, width, height, stride, pixels};
    const uint16_t bg = rgb565(BACKGROUNDS[p->mode]), grid = rgb565(GRIDS[p->mode]);
    const uint16_t wave = rgb565(FOREGROUNDS[p->mode]);
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col) pixels[row * stride + col] = bg;
    if (p->mode != SOUND_RAW) {
        for (unsigned i = 0; i < p->background_count; ++i) {
            const int16_t *b = p->background[i];
            line(&s, b[0], b[1], b[2], b[3], grid, 1);
        }
    } else {
        int horizon = p->height / 4;
        for (int col = -p->width; col <= p->width * 2; col += p->width / 6)
            line(&s, p->width / 2, horizon, col, p->height - 8, grid, 1);
        const int fractions[] = {5, 14, 29, 52, 85};
        for (unsigned i = 0; i < 5; ++i) {
            int row = horizon + (p->height - 8 - horizon) * fractions[i] / 100;
            line(&s, 4, row, p->width - 5, row, grid, 1);
        }
    }
    for (unsigned layer = 0; layer < (p->mode == SOUND_MIRROR ? 2u : 1u); ++layer)
        for (unsigned i = 1; i < SOUND_CURVE_POINTS; ++i)
            line(&s, (i-1) * (p->width-1) / (SOUND_CURVE_POINTS-1), p->y[layer][i-1],
                     i * (p->width-1) / (SOUND_CURVE_POINTS-1), p->y[layer][i], wave, 2);
}
