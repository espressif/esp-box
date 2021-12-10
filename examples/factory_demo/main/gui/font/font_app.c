/*******************************************************************************
 * Size: 20 px
 * Bpp: 4
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef FONT_APP
#define FONT_APP 1
#endif

#if FONT_APP

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+FA00 "切" */
    0x0, 0x1, 0x60, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x1, 0xdf, 0xb0, 0x0, 0x9a, 0xaa, 0xa1, 0x1,
    0xdf, 0xff, 0xb0, 0x3f, 0xff, 0xff, 0x60, 0xdf,
    0xff, 0xff, 0xb3, 0xff, 0xff, 0xf6, 0x1e, 0xff,
    0xff, 0xfd, 0x3f, 0xff, 0xff, 0x60, 0x3e, 0xff,
    0xfd, 0x13, 0xff, 0xff, 0xf6, 0x0, 0x3e, 0xfd,
    0x10, 0x1e, 0xff, 0xfe, 0x30, 0x0, 0x29, 0x10,
    0x0, 0x0, 0x0, 0x0, 0x3, 0xef, 0xff, 0xd1,
    0x1d, 0xff, 0xfe, 0x30, 0x6f, 0xff, 0xff, 0x33,
    0xff, 0xff, 0xf6, 0x6, 0xff, 0xff, 0xf3, 0x3f,
    0xff, 0xff, 0x60, 0x6f, 0xff, 0xff, 0x33, 0xff,
    0xff, 0xf6, 0x6, 0xff, 0xff, 0xf3, 0x3f, 0xff,
    0xff, 0x60, 0x1a, 0xaa, 0xa9, 0x0, 0x9a, 0xaa,
    0xa1
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 320, .box_w = 15, .box_h = 14, .ofs_x = 2, .ofs_y = 1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 64000, .range_length = 1, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
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
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t font_app = {
#else
lv_font_t font_app = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
    .base_line = -1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if FONT_APP*/

