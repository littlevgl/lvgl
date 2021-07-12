/**
 * @file lv_img.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_img.h"
#if LV_USE_IMG != 0

#include "../misc/lv_assert.h"
#include "../draw/lv_img_decoder.h"
#include "../misc/lv_fs.h"
#include "../misc/lv_txt.h"
#include "../misc/lv_math.h"
#include "../misc/lv_log.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_img_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_img_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_img(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_img_class = {
         .constructor_cb = lv_img_constructor,
         .destructor_cb = lv_img_destructor,
         .event_cb = lv_img_event,
         .width_def = LV_SIZE_CONTENT,
         .height_def = LV_SIZE_CONTENT,
         .instance_size = sizeof(lv_img_t),
         .base_class = &lv_obj_class
     };

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_img_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin")
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_img_set_src(lv_obj_t * obj, const void * src)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_obj_invalidate(obj);

    lv_img_src_t src_type = lv_img_src_get_type(src);
    lv_img_t * img = (lv_img_t *)obj;

#if LV_USE_LOG && LV_LOG_LEVEL >= LV_LOG_LEVEL_INFO
    switch(src_type) {
        case LV_IMG_SRC_FILE:
            LV_LOG_TRACE("lv_img_set_src: `LV_IMG_SRC_FILE` type found");
            break;
        case LV_IMG_SRC_VARIABLE:
            LV_LOG_TRACE("lv_img_set_src: `LV_IMG_SRC_VARIABLE` type found");
            break;
        case LV_IMG_SRC_SYMBOL:
            LV_LOG_TRACE("lv_img_set_src: `LV_IMG_SRC_SYMBOL` type found");
            break;
        default:
            LV_LOG_WARN("lv_img_set_src: unknown type");
    }
#endif

    /*If the new source type is unknown free the memories of the old source*/
    if(src_type == LV_IMG_SRC_UNKNOWN) {
        LV_LOG_WARN("lv_img_set_src: unknown image type");
        if(img->src_type == LV_IMG_SRC_SYMBOL || img->src_type == LV_IMG_SRC_FILE) {
            lv_mem_free((void*)img->src);
        }
        img->src      = NULL;
        img->src_type = LV_IMG_SRC_UNKNOWN;
        return;
    }

    lv_img_header_t header;
    lv_img_decoder_get_info(src, &header);

    /*Save the source*/
    if(src_type == LV_IMG_SRC_VARIABLE) {
        /*If memory was allocated because of the previous `src_type` then free it*/
        if(img->src_type == LV_IMG_SRC_FILE || img->src_type == LV_IMG_SRC_SYMBOL) {
            lv_mem_free((void*)img->src);
        }
        img->src = src;
    }
    else if(src_type == LV_IMG_SRC_FILE || src_type == LV_IMG_SRC_SYMBOL) {
        /*If the new and the old src are the same then it was only a refresh.*/
        if(img->src != src) {
            const void * old_src = NULL;
            /*If memory was allocated because of the previous `src_type` then save its pointer and free after allocation.
             *It's important to allocate first to be sure the new data will be on a new address.
             *Else `img_cache` wouldn't see the change in source.*/
            if(img->src_type == LV_IMG_SRC_FILE || img->src_type == LV_IMG_SRC_SYMBOL) {
                old_src = img->src;
            }
            char * new_str = lv_mem_alloc(strlen(src) + 1);
            LV_ASSERT_MALLOC(new_str);
            if(new_str == NULL) return;
            strcpy(new_str, src);
            img->src = new_str;

            if(old_src) lv_mem_free((void*)old_src);
        }
    }

    if(src_type == LV_IMG_SRC_SYMBOL) {
        /*`lv_img_dsc_get_info` couldn't set the with and height of a font so set it here*/
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_coord_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);
        lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
        lv_point_t size;
        lv_txt_get_size(&size, src, font, letter_space, line_space, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        header.w = size.x;
        header.h = size.y;
    }

    img->src_type = src_type;
    img->w        = header.w;
    img->h        = header.h;
    img->cf       = header.cf;
    img->pivot.x = header.w / 2;
    img->pivot.y = header.h / 2;

    lv_obj_refresh_self_size(obj);

    /*Provide enough room for the rotated corners*/
    if(img->angle || img->zoom != LV_IMG_ZOOM_NONE) lv_obj_refresh_ext_draw_size(obj);

    lv_obj_invalidate(obj);
}

void lv_img_set_offset_x(lv_obj_t * obj, lv_coord_t x)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    x = x % img->w;

    img->offset.x = x;
    lv_obj_invalidate(obj);
}

void lv_img_set_offset_y(lv_obj_t * obj, lv_coord_t y)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    y = y % img->h;

    img->offset.y = y;
    lv_obj_invalidate(obj);
}

void lv_img_set_angle(lv_obj_t * obj, int16_t angle)
{
    if(angle < 0 || angle >= 3600) angle = angle % 3600;

   lv_img_t * img = (lv_img_t *)obj;
    if(angle == img->angle) return;

    lv_coord_t transf_zoom = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);
    transf_zoom = (transf_zoom * img->zoom) >> 8;

    lv_coord_t transf_angle = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);

    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
    lv_area_t a;
    _lv_img_buf_get_transformed_area(&a, w, h, transf_angle + img->angle, transf_zoom, &img->pivot);
    a.x1 += obj->coords.x1;
    a.y1 += obj->coords.y1;
    a.x2 += obj->coords.x1;
    a.y2 += obj->coords.y1;
    lv_obj_invalidate_area(obj, &a);

    img->angle = angle;
    lv_obj_refresh_ext_draw_size(obj);

    _lv_img_buf_get_transformed_area(&a, w, h, transf_angle + img->angle, transf_zoom, &img->pivot);
    a.x1 += obj->coords.x1;
    a.y1 += obj->coords.y1;
    a.x2 += obj->coords.x1;
    a.y2 += obj->coords.y1;
    lv_obj_invalidate_area(obj, &a);
}

void lv_img_set_pivot(lv_obj_t * obj, lv_coord_t x, lv_coord_t y)
{
    lv_img_t * img = (lv_img_t *)obj;
    if(img->pivot.x == x && img->pivot.y == y) return;

    lv_coord_t transf_zoom = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);
    transf_zoom = (transf_zoom * img->zoom) >> 8;

    lv_coord_t transf_angle = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
    transf_angle += img->angle;

    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
    lv_area_t a;
    _lv_img_buf_get_transformed_area(&a, w, h, transf_angle, transf_zoom, &img->pivot);
    a.x1 += obj->coords.x1;
    a.y1 += obj->coords.y1;
    a.x2 += obj->coords.x1;
    a.y2 += obj->coords.y1;
    lv_obj_invalidate_area(obj, &a);

    img->pivot.x = x;
    img->pivot.y = y;
    lv_obj_refresh_ext_draw_size(obj);

    _lv_img_buf_get_transformed_area(&a, w, h, transf_angle, transf_zoom, &img->pivot);
    a.x1 += obj->coords.x1;
    a.y1 += obj->coords.y1;
    a.x2 += obj->coords.x1;
    a.y2 += obj->coords.y1;
    lv_obj_invalidate_area(obj, &a);
}

void lv_img_set_zoom(lv_obj_t * obj, uint16_t zoom)
{
   lv_img_t * img = (lv_img_t *)obj;
    if(zoom == img->zoom) return;

    if(zoom == 0) zoom = 1;

    lv_coord_t transf_zoom = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);

    lv_coord_t transf_angle = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
    transf_angle += img->angle;

    lv_coord_t w = lv_obj_get_width(obj);
    lv_coord_t h = lv_obj_get_height(obj);
    lv_area_t a;
    _lv_img_buf_get_transformed_area(&a, w, h, transf_angle, (transf_zoom * img->zoom) >> 8, &img->pivot);
    a.x1 += obj->coords.x1 - 1;
    a.y1 += obj->coords.y1 - 1;
    a.x2 += obj->coords.x1 + 1;
    a.y2 += obj->coords.y1 + 1;
    lv_obj_invalidate_area(obj, &a);

    img->zoom = zoom;
    lv_obj_refresh_ext_draw_size(obj);

    _lv_img_buf_get_transformed_area(&a, w, h, transf_angle, (transf_zoom * img->zoom) >> 8, &img->pivot);
    a.x1 += obj->coords.x1 - 1;
    a.y1 += obj->coords.y1 - 1;
    a.x2 += obj->coords.x1 + 1;
    a.y2 += obj->coords.y1 + 1;
    lv_obj_invalidate_area(obj, &a);
}

void lv_img_set_antialias(lv_obj_t * obj, bool antialias)
{
   lv_img_t * img = (lv_img_t *)obj;
    if(antialias == img->antialias) return;

    img->antialias = antialias;
    lv_obj_invalidate(obj);
}

void lv_img_set_content_size_mode(lv_obj_t * obj, lv_img_content_size_mode_t mode)
{
    lv_img_t * img = (lv_img_t *)obj;
    if(mode == img->obj_size_mode) return;

    img->obj_size_mode = mode;
    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

const void * lv_img_get_src(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    return img->src;
}

lv_coord_t lv_img_get_offset_x(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    return img->offset.x;
}

lv_coord_t lv_img_get_offset_y(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    return img->offset.y;
}

uint16_t lv_img_get_angle(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    return img->angle;
}

void lv_img_get_pivot(lv_obj_t * obj, lv_point_t * pivot)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    *pivot = img->pivot;
}

uint16_t lv_img_get_zoom(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    return img->zoom;
}

bool lv_img_get_antialias(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

   lv_img_t * img = (lv_img_t *)obj;

    return img->antialias ? true : false;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_img_t * img = (lv_img_t *)obj;

    img->src       = NULL;
    img->src_type  = LV_IMG_SRC_UNKNOWN;
    img->cf        = LV_IMG_CF_UNKNOWN;
    img->w         = lv_obj_get_width(obj);
    img->h         = lv_obj_get_height(obj);
    img->angle = 0;
    img->zoom = LV_IMG_ZOOM_NONE;
    img->antialias = LV_COLOR_DEPTH > 8 ? 1 : 0;
    img->offset.x  = 0;
    img->offset.y  = 0;
    img->pivot.x = 0;
    img->pivot.y = 0;
    img->obj_size_mode = LV_IMG_OBJ_SIZE_MODE_ORIGINAL;

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_ADV_HITTEST);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_img_t * img = (lv_img_t *)obj;
    if(img->src_type == LV_IMG_SRC_FILE || img->src_type == LV_IMG_SRC_SYMBOL) {
        lv_mem_free((void*)img->src);
        img->src      = NULL;
        img->src_type = LV_IMG_SRC_UNKNOWN;
    }
}

static lv_point_t lv_img_get_tranformed_size(lv_obj_t* obj)
{
    lv_img_t * img = (lv_img_t *)obj;

    int32_t zoom_final = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);
    zoom_final = (zoom_final * img->zoom) >> 8;
    int32_t angle_final = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
    angle_final += img->angle;

    lv_area_t area_transform;
    _lv_img_buf_get_transformed_area(&area_transform, img->w, img->h ,
            angle_final, zoom_final, &img->pivot);

    return (lv_point_t){lv_area_get_width(&area_transform), lv_area_get_height(&area_transform)};
}

static void lv_img_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    LV_UNUSED(class_p);

    lv_event_code_t code = lv_event_get_code(e);

    /*Ancestor events will be called during drawing*/
    if(code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
        /*Call the ancestor's event handler*/
        lv_res_t res = lv_obj_event_base(MY_CLASS, e);
        if(res != LV_RES_OK) return;
    }

    lv_obj_t * obj = lv_event_get_target(e);
    lv_img_t * img = (lv_img_t *)obj;

    if(code == LV_EVENT_STYLE_CHANGED) {
        /*Refresh the file name to refresh the symbol text size*/
        if(img->src_type == LV_IMG_SRC_SYMBOL) {
            lv_img_set_src(obj, img->src);
        } else {
            /*With transformation it might change*/
            lv_obj_refresh_ext_draw_size(obj);
        }
    }
    else if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {

        lv_coord_t * s = lv_event_get_param(e);
        lv_coord_t transf_zoom = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);
        transf_zoom = (transf_zoom * img->zoom) >> 8;

        lv_coord_t transf_angle = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
        transf_angle += img->angle;

        /*If the image has angle provide enough room for the rotated corners*/
        if(transf_angle || transf_zoom != LV_IMG_ZOOM_NONE) {
            lv_area_t a;
            lv_coord_t w = lv_obj_get_width(obj);
            lv_coord_t h = lv_obj_get_height(obj);
            _lv_img_buf_get_transformed_area(&a, w, h, transf_angle, transf_zoom, &img->pivot);
            lv_coord_t pad_ori = *s;
            *s = LV_MAX(*s, pad_ori - a.x1);
            *s = LV_MAX(*s, pad_ori - a.y1);
            *s = LV_MAX(*s, pad_ori + a.x2 - w);
            *s = LV_MAX(*s, pad_ori + a.y2 - h);
        }
    }
    else if(code == LV_EVENT_HIT_TEST) {
        lv_hit_test_info_t * info = lv_event_get_param(e);
        lv_coord_t zoom = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);
        zoom = (zoom * img->zoom) >> 8;

        lv_coord_t angle = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
        angle += img->angle;

        /*If the object is exactly image sized (not cropped, not mosaic) and transformed
         *perform hit test on it's transformed area*/
        if(img->w == lv_obj_get_width(obj) && img->h == lv_obj_get_height(obj) &&
                (zoom != LV_IMG_ZOOM_NONE || angle != 0 || img->pivot.x != img->w / 2 || img->pivot.y != img->h / 2)) {

            lv_coord_t w = lv_obj_get_width(obj);
            lv_coord_t h = lv_obj_get_height(obj);
            lv_area_t coords;
            _lv_img_buf_get_transformed_area(&coords, w, h, angle, zoom, &img->pivot);
            coords.x1 += obj->coords.x1;
            coords.y1 += obj->coords.y1;
            coords.x2 += obj->coords.x1;
            coords.y2 += obj->coords.y1;

            info->res = _lv_area_is_point_on(&coords, info->point, 0);
        }
        else {
            lv_area_t a;
            lv_obj_get_click_area(obj, &a);
            info->res = _lv_area_is_point_on(&a, info->point, 0);
        }
    }
    else if(code == LV_EVENT_GET_SELF_SIZE) {
        lv_point_t * p = lv_event_get_param(e);
        if (img->obj_size_mode == LV_IMG_OBJ_SIZE_MODE_ZOOMED){
            *p = lv_img_get_tranformed_size(obj);
        }
        else {
            p->x = img->w;
            p->y = img->h;
        }
    }
    else if(code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST || code == LV_EVENT_COVER_CHECK) {
        draw_img(e);
    }
}

static void draw_img(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    lv_img_t * img = (lv_img_t *)obj;
    if(code == LV_EVENT_COVER_CHECK) {
        lv_cover_check_info_t * info = lv_event_get_param(e);
        if(info->res == LV_COVER_RES_MASKED) return;
        if(img->src_type == LV_IMG_SRC_UNKNOWN || img->src_type == LV_IMG_SRC_SYMBOL) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        /*Non true color format might have "holes"*/
        if(img->cf != LV_IMG_CF_TRUE_COLOR && img->cf != LV_IMG_CF_RAW) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        /*With not LV_OPA_COVER images can't cover an area */
        if(lv_obj_get_style_img_opa(obj, LV_PART_MAIN) != LV_OPA_COVER) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        int32_t angle_final = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
        angle_final += img->angle;

        if(angle_final != 0) {
            info->res = LV_COVER_RES_NOT_COVER;
            return;
        }

        int32_t zoom_final = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);
        zoom_final = (zoom_final * img->zoom) >> 8;

        const lv_area_t * clip_area = lv_event_get_param(e);
        if(zoom_final == LV_IMG_ZOOM_NONE) {
            if(_lv_area_is_in(clip_area, &obj->coords, 0) == false) {
                info->res = LV_COVER_RES_NOT_COVER;
                return;
            }
        }
        else {
            lv_area_t a;
            _lv_img_buf_get_transformed_area(&a, lv_obj_get_width(obj), lv_obj_get_height(obj), 0, zoom_final, &img->pivot);
            a.x1 += obj->coords.x1;
            a.y1 += obj->coords.y1;
            a.x2 += obj->coords.x1;
            a.y2 += obj->coords.y1;

            if(_lv_area_is_in(clip_area, &a, 0) == false) {
                info->res = LV_COVER_RES_NOT_COVER;
                return;
            }
        }
    }
    else if(code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST) {

        int32_t zoom_final = lv_obj_get_style_transform_zoom(obj, LV_PART_MAIN);
        zoom_final = (zoom_final * img->zoom) >> 8;

        int32_t angle_final = lv_obj_get_style_transform_angle(obj, LV_PART_MAIN);
        angle_final += img->angle;

        lv_coord_t obj_w = lv_obj_get_width(obj);
        lv_coord_t obj_h = lv_obj_get_height(obj);

        lv_coord_t border_width = lv_obj_get_style_border_width(obj, LV_PART_MAIN);
        lv_coord_t pleft = lv_obj_get_style_pad_left(obj, LV_PART_MAIN) + border_width;
        lv_coord_t pright = lv_obj_get_style_pad_right(obj, LV_PART_MAIN) + border_width;
        lv_coord_t ptop = lv_obj_get_style_pad_top(obj, LV_PART_MAIN) + border_width;
        lv_coord_t pbottom = lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN) + border_width;

        lv_point_t bg_pivot;
        bg_pivot.x = img->pivot.x + pleft;
        bg_pivot.y = img->pivot.y + ptop;
        lv_area_t bg_coords;

        if (img->obj_size_mode == LV_IMG_OBJ_SIZE_MODE_ZOOMED) {
            /*Object size equals to transformed image size*/
            lv_obj_get_coords(obj, &bg_coords);
        }
        else {
            _lv_img_buf_get_transformed_area(&bg_coords, obj_w, obj_h,
                    angle_final, zoom_final, &bg_pivot);

            /*Modify the coordinates to draw the background for the rotated and scaled coordinates*/
            bg_coords.x1 += obj->coords.x1;
            bg_coords.y1 += obj->coords.y1;
            bg_coords.x2 += obj->coords.x1;
            bg_coords.y2 += obj->coords.y1;
        }

        lv_area_t ori_coords;
        lv_area_copy(&ori_coords, &obj->coords);
        lv_area_copy(&obj->coords, &bg_coords);

        lv_res_t res = lv_obj_event_base(MY_CLASS, e);
        if(res != LV_RES_OK) return;

        lv_area_copy(&obj->coords, &ori_coords);

        if(code == LV_EVENT_DRAW_MAIN) {
            if(img->h == 0 || img->w == 0) return;
            if(zoom_final == 0) return;

            const lv_area_t * clip_area = lv_event_get_param(e);

            lv_area_t img_max_area;
            lv_area_copy(&img_max_area, &obj->coords);

            lv_point_t img_size_final = lv_img_get_tranformed_size(obj);

            if(img->obj_size_mode == LV_IMG_OBJ_SIZE_MODE_ZOOMED){
                img_max_area.x1 -= ((img->w - img_size_final.x) + 1) / 2;
                img_max_area.x2 -= ((img->w - img_size_final.x) + 1) / 2;
                img_max_area.y1 -= ((img->h - img_size_final.y) + 1) / 2;
                img_max_area.y2 -= ((img->h - img_size_final.y) + 1) / 2;
            }
            else {
                img_max_area.x2 = img_max_area.x1 + lv_area_get_width(&bg_coords) - 1;
                img_max_area.y2 = img_max_area.y1 + lv_area_get_height(&bg_coords) - 1;
            }

            img_max_area.x1 += pleft;
            img_max_area.y1 += ptop;
            img_max_area.x2 -= pright;
            img_max_area.y2 -= pbottom;

            if(img->src_type == LV_IMG_SRC_FILE || img->src_type == LV_IMG_SRC_VARIABLE) {
                lv_draw_img_dsc_t img_dsc;
                lv_draw_img_dsc_init(&img_dsc);
                lv_obj_init_draw_img_dsc(obj, LV_PART_MAIN, &img_dsc);

                img_dsc.zoom = zoom_final;
                img_dsc.angle = angle_final;
                img_dsc.pivot.x = img->pivot.x;
                img_dsc.pivot.y = img->pivot.y;
                img_dsc.antialias = img->antialias;

                lv_area_t img_clip_area;
                img_clip_area.x1 = bg_coords.x1 + pleft;
                img_clip_area.y1 = bg_coords.y1 + ptop;
                img_clip_area.x2 = bg_coords.x2 - pright;
                img_clip_area.y2 = bg_coords.y2 - pbottom;

                _lv_area_intersect(&img_clip_area, clip_area, &img_clip_area);

                lv_area_t coords_tmp;
                coords_tmp.y1 = img_max_area.y1 + img->offset.y;
                if(coords_tmp.y1 > img_max_area.y1) coords_tmp.y1 -= img->h;
                coords_tmp.y2 = coords_tmp.y1 + img->h - 1;

                for(; coords_tmp.y1 < img_max_area.y2; coords_tmp.y1 += img_size_final.y, coords_tmp.y2 += img_size_final.y) {
                    coords_tmp.x1 = img_max_area.x1 + img->offset.x;
                    if(coords_tmp.x1 > img_max_area.x1) coords_tmp.x1 -= img->w;
                    coords_tmp.x2 = coords_tmp.x1 + img->w - 1;

                    for(; coords_tmp.x1 < img_max_area.x2; coords_tmp.x1 += img_size_final.x, coords_tmp.x2 += img_size_final.x) {
                        lv_draw_img(&coords_tmp, &img_clip_area, img->src, &img_dsc);
                    }
                }
            }
            else if(img->src_type == LV_IMG_SRC_SYMBOL) {
                lv_draw_label_dsc_t label_dsc;
                lv_draw_label_dsc_init(&label_dsc);
                lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &label_dsc);

                lv_draw_label(&obj->coords, clip_area, &label_dsc, img->src, NULL);
            }
            else {
                /*Trigger the error handler of image draw*/
                LV_LOG_WARN("draw_img: image source type is unknown");
                lv_draw_img(&obj->coords, clip_area, NULL, NULL);
            }
        }
    }
}

#endif
