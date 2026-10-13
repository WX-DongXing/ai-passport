/*******************************************************************************
 * Size: 8 px
 * Bpp: 4
 * Opts: --size 8 --bpp 4 --format lvgl --font assets/fonts/offduty/OffDutySans.otf --symbols POCKET COMPANION --no-compress --no-kerning --lv-include lvgl.h --lv-font-name od_boot_caption -o /Users/didi/VscodeProjects/ai-passport/assets/fonts/offduty/od_boot_caption.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef OD_BOOT_CAPTION
#define OD_BOOT_CAPTION 1
#endif

#if OD_BOOT_CAPTION

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0041 "A" */
    0x3, 0xf2, 0x0, 0x7a, 0x70, 0xb, 0x2b, 0x0,
    0xc0, 0xd0, 0x4e, 0xce, 0x49, 0x40, 0x58, 0xd1,
    0x2, 0xd0,

    /* U+0043 "C" */
    0x4, 0xdd, 0x61, 0xe2, 0x2, 0x68, 0x0, 0x8,
    0x60, 0x0, 0x68, 0x0, 0x1, 0xe2, 0x4, 0x5,
    0xdd, 0x60,

    /* U+0045 "E" */
    0x3f, 0xdd, 0x33, 0xb0, 0x0, 0x3b, 0x0, 0x3,
    0xfd, 0xc0, 0x3b, 0x0, 0x3, 0xb0, 0x0, 0x3f,
    0xdd, 0x40,

    /* U+0049 "I" */
    0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b,

    /* U+004B "K" */
    0x3b, 0x6, 0xa0, 0x3b, 0x1d, 0x10, 0x3b, 0xb5,
    0x0, 0x3f, 0xe9, 0x0, 0x3e, 0x1d, 0x10, 0x3b,
    0x7, 0x80, 0x3b, 0x0, 0xe1,

    /* U+004D "M" */
    0x3f, 0x0, 0x5d, 0x3e, 0x40, 0xac, 0x3b, 0x90,
    0xbc, 0x39, 0xc4, 0x7d, 0x39, 0x8b, 0x2d, 0x39,
    0x3d, 0xd, 0x39, 0x3, 0xd,

    /* U+004E "N" */
    0x3e, 0x0, 0xc1, 0x3e, 0x70, 0xc1, 0x3a, 0xd0,
    0xc1, 0x3a, 0x86, 0xc1, 0x3a, 0x1c, 0xb1, 0x3a,
    0x9, 0xe1, 0x3a, 0x1, 0xf1,

    /* U+004F "O" */
    0x5, 0xdd, 0x60, 0x2d, 0x11, 0xd2, 0x68, 0x0,
    0x87, 0x86, 0x0, 0x68, 0x68, 0x0, 0x87, 0x1e,
    0x11, 0xd2, 0x5, 0xdd, 0x50,

    /* U+0050 "P" */
    0x3f, 0xdc, 0x23, 0xb0, 0x5a, 0x3b, 0x2, 0xc3,
    0xb0, 0x89, 0x3f, 0xdb, 0x13, 0xb0, 0x0, 0x3b,
    0x0, 0x0,

    /* U+0054 "T" */
    0xae, 0xfd, 0x80, 0xe, 0x0, 0x0, 0xe0, 0x0,
    0xe, 0x0, 0x0, 0xe0, 0x0, 0xe, 0x0, 0x0,
    0xe0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 29, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 18, .adv_w = 83, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 40, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 85, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 106, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 94, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 83, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 78, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x21, 0x23, 0x25, 0x29, 0x2b, 0x2d, 0x2e,
    0x2f, 0x30, 0x34
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 53, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 11, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t od_boot_caption = {
#else
lv_font_t od_boot_caption = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 7,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if OD_BOOT_CAPTION*/
