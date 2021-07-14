/**
 * @file lvgl.h
 * Include all LVGL related headers
 */

#ifndef LVGL_H
#define LVGL_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************
 * CURRENT VERSION OF LVGL
 ***************************/
#define LVGL_VERSION_MAJOR 8
#define LVGL_VERSION_MINOR 1
#define LVGL_VERSION_PATCH 0
#define LVGL_VERSION_INFO "dev"

/*********************
 *      INCLUDES
 *********************/

#include "src/misc/lv_log.h"
#include "src/misc/lv_timer.h"
#include "src/misc/lv_math.h"
#include "src/misc/lv_async.h"
#include "src/misc/lv_snapshot.h"

#include "src/hal/lv_hal.h"

#include "src/core/lv_obj.h"
#include "src/core/lv_group.h"
#include "src/core/lv_indev.h"

#include "src/core/lv_refr.h"
#include "src/core/lv_disp.h"
#include "src/core/lv_theme.h"

#include "src/font/lv_font.h"
#include "src/font/lv_font_loader.h"
#include "src/font/lv_font_fmt_txt.h"
#include "src/misc/lv_printf.h"

#include "src/widgets/lv_arc.h"
#include "src/widgets/lv_btn.h"
#include "src/widgets/lv_img.h"
#include "src/widgets/lv_label.h"
#include "src/widgets/lv_line.h"
#include "src/widgets/lv_table.h"
#include "src/widgets/lv_checkbox.h"
#include "src/widgets/lv_bar.h"
#include "src/widgets/lv_slider.h"
#include "src/widgets/lv_btnmatrix.h"
#include "src/widgets/lv_dropdown.h"
#include "src/widgets/lv_roller.h"
#include "src/widgets/lv_textarea.h"
#include "src/widgets/lv_canvas.h"
#include "src/widgets/lv_switch.h"

#include "src/draw/lv_draw.h"

#include "src/lv_api_map.h"

/*-----------------
 * EXTRAS
 *----------------*/
#include "src/extra/lv_extra.h"
#include "src/extra/widgets/lv_widgets.h"
#include "src/extra/layouts/lv_layouts.h"
#include "src/extra/themes/lv_themes.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/** Gives 1 if the x.y.z version is supported in the current version
 * Usage:
 *
 * - Require v6
 * #if LV_VERSION_CHECK(6,0,0)
 *   new_func_in_v6();
 * #endif
 *
 *
 * - Require at least v5.3
 * #if LV_VERSION_CHECK(5,3,0)
 *   new_feature_from_v5_3();
 * #endif
 *
 *
 * - Require v5.3.2 bugfixes
 * #if LV_VERSION_CHECK(5,3,2)
 *   bugfix_in_v5_3_2();
 * #endif
 *
 */
#define LV_VERSION_CHECK(x,y,z) (x == LVGL_VERSION_MAJOR && (y < LVGL_VERSION_MINOR || (y == LVGL_VERSION_MINOR && z <= LVGL_VERSION_PATCH)))

/**
 * Wrapper functions for VERSION macros
 */

static inline int lv_version_major(void)
{
    return LVGL_VERSION_MAJOR;
}

static inline int lv_version_minor(void)
{
    return LVGL_VERSION_MINOR;
}

static inline int lv_version_patch(void)
{
    return LVGL_VERSION_PATCH;
}

static inline const char *lv_version_info(void)
{
    return LVGL_VERSION_INFO;
}

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_H*/
