/**
 * @file lv_obj_style.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_obj.h"
#include "lv_disp.h"
#include "../misc/lv_gc.h"

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_obj_class

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    lv_obj_t * obj;
    lv_style_prop_t prop;
    lv_style_selector_t selector;
    lv_style_value_t start_value;
    lv_style_value_t end_value;
} trans_t;

typedef enum {
    CACHE_ZERO = 0,
    CACHE_TRUE = 1,
    CACHE_UNSET = 2,
    CACHE_255 = 3,
    CACHE_NEED_CHECK = 4,
}cache_t;

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_style_t * get_local_style(lv_obj_t * obj, lv_style_selector_t selector);
static _lv_obj_style_t * get_trans_style(lv_obj_t * obj, uint32_t part);
static bool get_prop_core(const lv_obj_t * obj, lv_part_t part, lv_style_prop_t prop, lv_style_value_t * v);
static lv_style_value_t apply_color_filter(const lv_obj_t * obj, uint32_t part, lv_style_value_t v);
static void report_style_change_core(void * style, lv_obj_t * obj);
static void refresh_children_style(lv_obj_t * obj);
static bool trans_del(lv_obj_t * obj, lv_part_t part, lv_style_prop_t prop, trans_t * tr_limit);
static void trans_anim_cb(void * _tr, int32_t v);
static void trans_anim_start_cb(lv_anim_t * a);
static void trans_anim_ready_cb(lv_anim_t * a);
static void fade_anim_cb(void * obj, int32_t v);
static void fade_in_anim_ready(lv_anim_t * a);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool style_refr = true;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void _lv_obj_style_init(void)
{
    _lv_ll_init(&LV_GC_ROOT(_lv_obj_style_trans_ll), sizeof(trans_t));
}

void lv_obj_add_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector)
{
    trans_del(obj, selector, LV_STYLE_PROP_ANY, NULL);

    uint32_t i;
    /*Go after the transition and local styles*/
    for(i = 0; i < obj->style_cnt; i++) {
        if(obj->styles[i].is_trans) continue;
        if(obj->styles[i].is_local) continue;
        break;
    }

    /*Now `i` is at the first normal style. Insert the new style before this*/

    /*Allocate space for the new style and shift the rest of the style to the end*/
    obj->style_cnt++;
    obj->styles = lv_mem_realloc(obj->styles, obj->style_cnt * sizeof(_lv_obj_style_t));

    uint32_t j;
    for(j = obj->style_cnt - 1; j > i ; j--) {
        obj->styles[j] = obj->styles[j - 1];
    }

    lv_memset_00(&obj->styles[i], sizeof(_lv_obj_style_t));
    obj->styles[i].style = style;
    obj->styles[i].selector = selector;

    lv_obj_refresh_style(obj, selector, LV_STYLE_PROP_ANY);
}

void lv_obj_remove_style(lv_obj_t * obj, lv_style_t * style, lv_style_selector_t selector)
{
    lv_state_t state = lv_obj_style_get_selector_state(selector);
    lv_part_t part = lv_obj_style_get_selector_part(selector);
    lv_style_prop_t prop = LV_STYLE_PROP_ANY;
    if(style && style->prop_cnt == 0) prop = LV_STYLE_PROP_INV;

    uint32_t i = 0;
    bool deleted = false;
    while(i <  obj->style_cnt) {
        lv_state_t state_act = lv_obj_style_get_selector_state(obj->styles[i].selector);
        lv_part_t part_act = lv_obj_style_get_selector_part(obj->styles[i].selector);
        if((state != LV_STATE_ANY && state_act != state) ||
           (part != LV_PART_ANY && part_act != part) ||
           (style != NULL && style != obj->styles[i].style))
        {
            i++;
            continue;
        }

        if(obj->styles[i].is_trans) {
            trans_del(obj, part, LV_STYLE_PROP_ANY, NULL);
        }

        if(obj->styles[i].is_local || obj->styles[i].is_trans) {
            lv_style_reset(obj->styles[i].style);
            lv_mem_free(obj->styles[i].style);
            obj->styles[i].style = NULL;
        }

        /*Shift the styles after `i` by one*/
        uint32_t j;
        for(j = i; j < (uint32_t)obj->style_cnt - 1 ; j++) {
            obj->styles[j] = obj->styles[j + 1];
        }

        obj->style_cnt--;
        obj->styles = lv_mem_realloc(obj->styles, obj->style_cnt * sizeof(_lv_obj_style_t));

        deleted = true;
        /*The style from the current `i` index is removed, so `i` points to the next style.
         *Therefore it doesn't needs to be incremented*/
    }
    if(deleted && prop != LV_STYLE_PROP_INV) {
        lv_obj_refresh_style(obj, part, prop);
    }
}

void lv_obj_report_style_change(lv_style_t * style)
{
    if(!style_refr) return;
    lv_disp_t * d = lv_disp_get_next(NULL);

    while(d) {
        uint32_t i;
        for(i = 0; i < d->screen_cnt; i++) {
            report_style_change_core(style, d->screens[i]);
        }
        d = lv_disp_get_next(d);
    }
}

void lv_obj_refresh_style(lv_obj_t * obj, lv_style_selector_t selector, lv_style_prop_t prop)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    if(!style_refr) return;

    lv_obj_invalidate(obj);

    lv_part_t part = lv_obj_style_get_selector_part(selector);

    if((part == LV_PART_ANY || part == LV_PART_MAIN) && (prop == LV_STYLE_PROP_ANY || (prop & LV_STYLE_PROP_LAYOUT_REFR))) {
        lv_event_send(obj, LV_EVENT_STYLE_CHANGED, NULL);
        lv_obj_mark_layout_as_dirty(obj);
    }
    if((part == LV_PART_ANY || part == LV_PART_MAIN) && (prop == LV_STYLE_PROP_ANY || (prop & LV_STYLE_PROP_PARENT_LAYOUT_REFR))) {
        lv_obj_t * parent = lv_obj_get_parent(obj);
        if(parent) lv_obj_mark_layout_as_dirty(parent);
    }

    if(prop == LV_STYLE_PROP_ANY || (prop & LV_STYLE_PROP_EXT_DRAW)) {
        lv_obj_refresh_ext_draw_size(obj);
    }
    lv_obj_invalidate(obj);

    if(prop == LV_STYLE_PROP_ANY ||
      ((prop & LV_STYLE_PROP_INHERIT) && ((prop & LV_STYLE_PROP_EXT_DRAW) || (prop & LV_STYLE_PROP_LAYOUT_REFR))))
    {
        if(part != LV_PART_SCROLLBAR) {
            refresh_children_style(obj);
        }
    }
}

void lv_obj_enable_style_refresh(bool en)
{
    style_refr = en;
}

lv_style_value_t lv_obj_get_style_prop(const lv_obj_t * obj, lv_part_t part, lv_style_prop_t prop)
{
    lv_style_value_t value_act;
    bool inherit = prop & LV_STYLE_PROP_INHERIT ? true : false;
    bool filter = prop & LV_STYLE_PROP_FILTER ? true : false;
    if(filter) {
        prop &= ~LV_STYLE_PROP_FILTER;
    }
    bool found = false;
    while(obj) {
        found = get_prop_core(obj, part, prop, &value_act);
        if(found) break;
        if(!inherit) break;

        /*If not found, check the `MAIN` style first*/
        if(part != LV_PART_MAIN) {
            part = LV_PART_MAIN;
            continue;
        }

        /*Check the parent too.*/
        obj = lv_obj_get_parent(obj);
    }

    if(!found) {
        if(part == LV_PART_MAIN && (prop == LV_STYLE_WIDTH || prop == LV_STYLE_HEIGHT)) {
            const lv_obj_class_t * cls = obj->class_p;
            while(cls) {
                if(prop == LV_STYLE_WIDTH) {
                    if(cls->width_def != 0) break;
                } else {
                    if(cls->height_def != 0) break;
                }
                cls = cls->base_class;
            }

            value_act.num = prop == LV_STYLE_WIDTH ? cls->width_def : cls->height_def;
        } else {
            value_act = lv_style_prop_get_default(prop);
        }
    }
    if(filter) value_act = apply_color_filter(obj, part, value_act);
    return value_act;
}

void lv_obj_set_local_style_prop(lv_obj_t * obj, lv_style_prop_t prop, lv_style_value_t value, lv_style_selector_t selector)
{
    lv_style_t * style = get_local_style(obj, selector);
    lv_style_set_prop(style, prop, value);
    lv_obj_refresh_style(obj, selector, prop);
}


lv_res_t lv_obj_get_local_style_prop(lv_obj_t * obj, lv_style_prop_t prop, lv_style_value_t * value, lv_style_selector_t selector)
{
    uint32_t i;
    for(i = 0; i < obj->style_cnt; i++) {
        if(obj->styles[i].is_local &&
           obj->styles[i].selector ==  selector)
        {
            return lv_style_get_prop(obj->styles[i].style, prop, value);
        }
    }

    return LV_RES_INV;
}

bool lv_obj_remove_local_style_prop(lv_obj_t * obj, lv_style_prop_t prop, lv_style_selector_t selector)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    uint32_t i;
    /*Find the style*/
    for(i = 0; i < obj->style_cnt; i++) {
        if(obj->styles[i].is_local &&
           obj->styles[i].selector == selector) {
            break;
        }
    }

    /*The style is not found*/
    if(i == obj->style_cnt) return false;

    return lv_style_remove_prop(obj->styles[i].style, prop);
}

void _lv_obj_style_create_transition(lv_obj_t * obj, lv_part_t part, lv_state_t prev_state, lv_state_t new_state, const _lv_obj_style_transition_dsc_t * tr_dsc)
{
    trans_t * tr;

    /*Get the previous and current values*/
    obj->skip_trans = 1;
    obj->state = prev_state;
    lv_style_value_t v1 = lv_obj_get_style_prop(obj, part, tr_dsc->prop);
    obj->state = new_state;
    lv_style_value_t v2 = lv_obj_get_style_prop(obj, part, tr_dsc->prop);
    obj->skip_trans = 0;

    if(v1.ptr == v2.ptr && v1.num == v2.num && v1.color.full == v2.color.full)  return;
    obj->state = prev_state;
    v1 = lv_obj_get_style_prop(obj, part, tr_dsc->prop);
    obj->state = new_state;

    _lv_obj_style_t * style_trans = get_trans_style(obj, part);
    lv_style_set_prop(style_trans->style, tr_dsc->prop, v1);   /*Be sure `trans_style` has a valid value*/

    if(tr_dsc->prop == LV_STYLE_RADIUS) {
        if(v1.num == LV_RADIUS_CIRCLE || v2.num == LV_RADIUS_CIRCLE) {
            lv_coord_t whalf = lv_obj_get_width(obj) / 2;
            lv_coord_t hhalf = lv_obj_get_width(obj) / 2;
            if(v1.num == LV_RADIUS_CIRCLE) v1.num = LV_MIN(whalf + 1, hhalf + 1);
            if(v2.num == LV_RADIUS_CIRCLE) v2.num = LV_MIN(whalf + 1, hhalf + 1);
        }
    }

    tr = _lv_ll_ins_head(&LV_GC_ROOT(_lv_obj_style_trans_ll));
    LV_ASSERT_MALLOC(tr);
    if(tr == NULL) return;
    tr->start_value = v1;
    tr->end_value = v2;

    if(tr) {
        tr->obj = obj;
        tr->prop = tr_dsc->prop;
        tr->selector = part;

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, tr);
        lv_anim_set_exec_cb(&a, trans_anim_cb);
        lv_anim_set_start_cb(&a, trans_anim_start_cb);
        lv_anim_set_ready_cb(&a, trans_anim_ready_cb);
        lv_anim_set_values(&a, 0x00, 0xFF);
        lv_anim_set_time(&a, tr_dsc->time);
        lv_anim_set_delay(&a, tr_dsc->delay);
        lv_anim_set_path_cb(&a, tr_dsc->path_cb);
        lv_anim_set_early_apply(&a, false);
#if LV_USE_USER_DATA
        a.user_data = tr_dsc->user_data;
#endif
        lv_anim_start(&a);
    }
}

_lv_style_state_cmp_t _lv_obj_style_state_compare(lv_obj_t * obj, lv_state_t state1, lv_state_t state2)
{
    _lv_style_state_cmp_t res = _LV_STYLE_STATE_CMP_SAME;

    /*Are there any new styles for the new state?*/
    uint32_t i;
    for(i = 0; i < obj->style_cnt; i++) {
        if(obj->styles[i].is_trans) continue;

        lv_state_t state_act = lv_obj_style_get_selector_state(obj->styles[i].selector);
        lv_part_t part_act = lv_obj_style_get_selector_part(obj->styles[i].selector);
        /*The style is valid for a stat but not the other*/
        bool valid1 = state_act & (~state1) ? false : true;
        bool valid2 = state_act & (~state2) ? false : true;
        if(valid1 != valid2) {
            lv_style_t * style = obj->styles[i].style;
            lv_style_value_t v;
            /*If there is layout difference on the main part, return immediately. There is no more serious difference*/
            bool layout_diff = false;
            if(lv_style_get_prop(style, LV_STYLE_PAD_TOP, &v))layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_PAD_BOTTOM, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_PAD_LEFT, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_PAD_RIGHT, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_PAD_COLUMN, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_PAD_ROW, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_LAYOUT, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_TRANSLATE_X, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_TRANSLATE_Y, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_WIDTH, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_HEIGHT, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_MIN_WIDTH, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_MAX_WIDTH, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_MIN_HEIGHT, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_MAX_HEIGHT, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_BORDER_WIDTH, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_TRANSFORM_ANGLE, &v)) layout_diff = true;
            else if(lv_style_get_prop(style, LV_STYLE_TRANSFORM_ZOOM, &v)) layout_diff = true;

            if(layout_diff) {
                if(part_act == LV_PART_MAIN) {
                    return _LV_STYLE_STATE_CMP_DIFF_LAYOUT;
                }
                else {
                    res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
                    continue;
                }
            }

            /*Check for draw pad changes*/
            if(lv_style_get_prop(style, LV_STYLE_TRANSFORM_WIDTH, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_TRANSFORM_HEIGHT, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_TRANSFORM_ANGLE, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_TRANSFORM_ZOOM, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_OUTLINE_OPA, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_OUTLINE_PAD, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_OUTLINE_WIDTH, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_SHADOW_WIDTH, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_SHADOW_OPA, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_SHADOW_OFS_X, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_SHADOW_OFS_Y, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_SHADOW_SPREAD, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(lv_style_get_prop(style, LV_STYLE_LINE_WIDTH, &v)) res = _LV_STYLE_STATE_CMP_DIFF_DRAW_PAD;
            else if(res == _LV_STYLE_STATE_CMP_SAME) res = _LV_STYLE_STATE_CMP_DIFF_REDRAW;
        }
    }

    return res;
}

void lv_obj_fade_in(lv_obj_t * obj, uint32_t time, uint32_t delay)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, fade_anim_cb);
    lv_anim_set_ready_cb(&a, fade_in_anim_ready);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    lv_anim_start(&a);
}

void lv_obj_fade_out(lv_obj_t * obj, uint32_t time, uint32_t delay)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&a, fade_anim_cb);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, delay);
    lv_anim_start(&a);
}

lv_state_t lv_obj_style_get_selector_state(lv_style_selector_t selector)
{
    return selector & 0xFFFF;
}

lv_part_t lv_obj_style_get_selector_part(lv_style_selector_t selector)
{
    return selector & 0xFF0000;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Get the local style of an object for a given part and for a given state.
 * If the local style for the part-state pair doesn't exist allocate and return it.
 * @param obj   pointer to an object
 * @param part  the part in whose local style to get
 * @param state the state in whose local style to get
 * @return pointer to the local style
 */
static lv_style_t * get_local_style(lv_obj_t * obj,  lv_style_selector_t selector)
{
    uint32_t i;
    for(i = 0; i < obj->style_cnt; i++) {
        if(obj->styles[i].is_local &&
           obj->styles[i].selector == selector)
        {
            return obj->styles[i].style;
        }
    }

    obj->style_cnt++;
    obj->styles = lv_mem_realloc(obj->styles, obj->style_cnt * sizeof(_lv_obj_style_t));
    LV_ASSERT_MALLOC(obj->styles);

    for(i = obj->style_cnt - 1; i > 0 ; i--) {
        /*Copy only normal styles (not local and transition).
         *The new local style will be added as the last local style*/
        if(obj->styles[i - 1].is_local || obj->styles[i - 1].is_trans) break;
        obj->styles[i] = obj->styles[i - 1];
    }

    lv_memset_00(&obj->styles[i], sizeof(_lv_obj_style_t));
    obj->styles[i].style = lv_mem_alloc(sizeof(lv_style_t));
    lv_style_init(obj->styles[i].style);
    obj->styles[i].is_local = 1;
    obj->styles[i].selector = selector;
    return obj->styles[i].style;
}

/**
 * Get the transition style of an object for a given part and for a given state.
 * If the transition style for the part-state pair doesn't exist allocate and return it.
 * @param obj   pointer to an object
 * @param part  the part in whose local style to get
 * @param state the state in whose local style to get
 * @return pointer to the transition style
 */
static _lv_obj_style_t * get_trans_style(lv_obj_t * obj,  lv_style_selector_t selector)
{
    uint32_t i;
    for(i = 0; i < obj->style_cnt; i++) {
        if(obj->styles[i].is_trans && obj->styles[i].selector == selector) break;
    }

    /*Already have a transition style for it*/
    if(i != obj->style_cnt) return &obj->styles[i];

    obj->style_cnt++;
    obj->styles = lv_mem_realloc(obj->styles, obj->style_cnt * sizeof(_lv_obj_style_t));

    for(i = obj->style_cnt - 1; i > 0 ; i--) {
        obj->styles[i] = obj->styles[i - 1];
    }

    lv_memset_00(&obj->styles[0], sizeof(_lv_obj_style_t));
    obj->styles[0].style = lv_mem_alloc(sizeof(lv_style_t));
    lv_style_init(obj->styles[0].style);
    obj->styles[0].is_trans = 1;
    obj->styles[0].selector = selector;
    return &obj->styles[0];
}


static bool get_prop_core(const lv_obj_t * obj, lv_part_t part, lv_style_prop_t prop, lv_style_value_t * v)
{
    uint8_t group = 1 << _lv_style_get_prop_group(prop);
    int32_t weight = -1;
    lv_state_t state = obj->state;
    lv_state_t state_inv = ~state;
    lv_style_value_t value_tmp;
    bool skip_trans = obj->skip_trans;
    uint32_t i;
    bool found;
    for(i = 0; i < obj->style_cnt; i++) {
        _lv_obj_style_t * obj_style = &obj->styles[i];
        if(obj_style->is_trans == false) break;
        if(skip_trans) continue;

        lv_part_t part_act = lv_obj_style_get_selector_part(obj->styles[i].selector);

        if(part_act != part) continue;
        if((obj_style->style->has_group & group) == 0) continue;
        found = lv_style_get_prop(obj_style->style, prop, &value_tmp);
        if(found) {
            *v = value_tmp;
            return true;
        }
    }

    for(; i < obj->style_cnt; i++) {
        _lv_obj_style_t * obj_style = &obj->styles[i];
        lv_part_t part_act = lv_obj_style_get_selector_part(obj->styles[i].selector);
        lv_state_t state_act = lv_obj_style_get_selector_state(obj->styles[i].selector);
        if(part_act != part) continue;

        if((obj_style->style->has_group & group) == 0) continue;

        /*Be sure the style not specifies other state than the requested.
         *E.g. For HOVER+PRESS object state, HOVER style only is OK, but HOVER+FOCUS style is not*/
        if((state_act & state_inv)) continue;

        /*Check only better candidates*/
        if(state_act <= weight) continue;

        found = lv_style_get_prop(obj_style->style, prop, &value_tmp);

        if(found) {
            if(state_act == state) {
                *v = value_tmp;
                return true;
            }
            if(weight < state_act) {
                weight = state_act;
                *v = value_tmp;
            }
        }
    }

    if(weight >= 0) {
        *v = value_tmp;
        return true;
    }
    else return false;
}

static lv_style_value_t apply_color_filter(const lv_obj_t * obj, uint32_t part, lv_style_value_t v)
{
    if(obj == NULL) return v;
    const lv_color_filter_dsc_t * f = lv_obj_get_style_color_filter_dsc(obj, part);
    if(f && f->filter_cb) {
        lv_opa_t f_opa = lv_obj_get_style_color_filter_opa(obj, part);
        if(f_opa != 0) v.color = f->filter_cb(f, v.color, f_opa);
    }
    return v;
}

/**
 * Refresh the style of all children of an object. (Called recursively)
 * @param style refresh objects only with this
 * @param obj pointer to an object
 */
static void report_style_change_core(void * style, lv_obj_t * obj)
{
    uint32_t i;
    for(i = 0; i < obj->style_cnt; i++) {
        if(style == NULL || obj->styles[i].style == style) {
            lv_obj_refresh_style(obj, LV_PART_ANY, LV_STYLE_PROP_ANY);
            break;
        }
    }

    uint32_t child_cnt = lv_obj_get_child_cnt(obj);
    for(i = 0; i < child_cnt; i++) {
        report_style_change_core(style, lv_obj_get_child(obj, i));
    }
}

/**
 * Recursively refresh the style of the children. Go deeper until a not NULL style is found
 * because the NULL styles are inherited from the parent
 * @param obj pointer to an object
 */
static void refresh_children_style(lv_obj_t * obj)
{
    uint32_t i;
    uint32_t child_cnt = lv_obj_get_child_cnt(obj);
    for(i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(obj, i);
        lv_obj_invalidate(child);
        lv_event_send(child, LV_EVENT_STYLE_CHANGED, NULL);
        lv_obj_invalidate(child);

        refresh_children_style(child); /*Check children too*/
    }
}

/**
 * Remove the transition from object's part's property.
 * - Remove the transition from `_lv_obj_style_trans_ll` and free it
 * - Delete pending transitions
 * @param obj pointer to an object which transition(s) should be removed
 * @param part a part of object or 0xFF to remove from all parts
 * @param prop a property or 0xFF to remove all properties
 * @param tr_limit delete transitions only "older" than this. `NULL` if not used
 */
static bool trans_del(lv_obj_t * obj, lv_part_t part, lv_style_prop_t prop, trans_t * tr_limit)
{
    trans_t * tr;
    trans_t * tr_prev;
    bool removed = false;
    tr = _lv_ll_get_tail(&LV_GC_ROOT(_lv_obj_style_trans_ll));
    while(tr != NULL) {
        if(tr == tr_limit) break;

        /*'tr' might be deleted, so get the next object while 'tr' is valid*/
        tr_prev = _lv_ll_get_prev(&LV_GC_ROOT(_lv_obj_style_trans_ll), tr);

        if(tr->obj == obj && (part == tr->selector || part == LV_PART_ANY) && (prop == tr->prop || prop == LV_STYLE_PROP_ANY)) {
            /*Remove the transitioned property from trans. style
             *to allow changing it by normal styles*/
            uint32_t i;
            for(i = 0; i < obj->style_cnt; i++) {
                if(obj->styles[i].is_trans && (part == LV_PART_ANY || obj->styles[i].selector == part)) {
                    lv_style_remove_prop(obj->styles[i].style, tr->prop);
                    lv_anim_del(tr, NULL);
                    _lv_ll_remove(&LV_GC_ROOT(_lv_obj_style_trans_ll), tr);
                    lv_mem_free(tr);
                    removed = true;
                }
            }

        }
        tr = tr_prev;
    }
    return removed;
}

static void trans_anim_cb(void * _tr, int32_t v)
{
    trans_t * tr = _tr;
    lv_obj_t * obj = tr->obj;

    uint32_t i;
    for(i = 0; i < obj->style_cnt; i++) {
        if(obj->styles[i].is_trans == 0 || obj->styles[i].selector != tr->selector) continue;

        lv_style_value_t value_final;
        switch (tr->prop) {

            case LV_STYLE_BORDER_SIDE:
            case LV_STYLE_BORDER_POST:
            case LV_STYLE_BLEND_MODE:
                if(v < 255) value_final.num = tr->start_value.num;
                else value_final.num = tr->end_value.num;
                break;
            case LV_STYLE_TRANSITION:
            case LV_STYLE_TEXT_FONT:
                if(v < 255) value_final.ptr = tr->start_value.ptr;
                else value_final.ptr = tr->end_value.ptr;
                break;
            case LV_STYLE_COLOR_FILTER_DSC:
                if(tr->start_value.ptr == NULL) value_final.ptr = tr->end_value.ptr;
                else if(tr->end_value.ptr == NULL) value_final.ptr = tr->start_value.ptr;
                else if(v < 128) value_final.ptr = tr->start_value.ptr;
                else value_final.ptr = tr->end_value.ptr;
                break;
            case LV_STYLE_BG_COLOR:
            case LV_STYLE_BORDER_COLOR:
            case LV_STYLE_TEXT_COLOR:
            case LV_STYLE_SHADOW_COLOR:
            case LV_STYLE_OUTLINE_COLOR:
            case LV_STYLE_IMG_RECOLOR:
                if(v <= 0) value_final.color = tr->start_value.color;
                else if(v >= 255) value_final.color = tr->end_value.color;
                else value_final.color = lv_color_mix(tr->end_value.color, tr->start_value.color, v);
                break;

            default:
                if(v == 0) value_final.num = tr->start_value.num;
                else if(v == 255) value_final.num = tr->end_value.num;
                else value_final.num = tr->start_value.num + ((int32_t)((int32_t)(tr->end_value.num - tr->start_value.num) * v) >> 8);
                break;
        }

        lv_style_value_t old_value;
        bool refr = true;
        if(lv_style_get_prop(obj->styles[i].style, tr->prop, &old_value)) {
            if(value_final.ptr == old_value.ptr && value_final.color.full == old_value.color.full && value_final.num == old_value.num) {
                refr = false;
            }
        }
        lv_style_set_prop(obj->styles[i].style, tr->prop, value_final);
        if (refr) lv_obj_refresh_style(tr->obj, tr->selector, tr->prop);
        break;

    }

}

static void trans_anim_start_cb(lv_anim_t * a)
{
    trans_t * tr = a->var;

    lv_part_t part = lv_obj_style_get_selector_part(tr->selector);
    tr->start_value = lv_obj_get_style_prop(tr->obj, part, tr->prop);

    /*Init prop to an invalid values to be sure `trans_del` won't delete this added `tr`*/
    lv_style_prop_t prop_tmp = tr->prop;
    tr->prop = LV_STYLE_PROP_INV;

    /*Delete the related transitions if any*/
    trans_del(tr->obj, part, prop_tmp, tr);

    tr->prop = prop_tmp;

    _lv_obj_style_t * style_trans = get_trans_style(tr->obj, tr->selector);
    lv_style_set_prop(style_trans->style, tr->prop, tr->start_value);   /*Be sure `trans_style` has a valid value*/

}

static void trans_anim_ready_cb(lv_anim_t * a)
{
    trans_t * tr = a->var;
    lv_obj_t * obj = tr->obj;
    lv_style_prop_t prop = tr->prop;

    /*Remove the transitioned property from trans. style
     *if there no more transitions for this property
     *It allows changing it by normal styles*/
    bool running = false;
    trans_t * tr_i;
    _LV_LL_READ(&LV_GC_ROOT(_lv_obj_style_trans_ll), tr_i) {
        if(tr_i != tr && tr_i->obj == tr->obj && tr_i->selector == tr->selector && tr_i->prop == tr->prop) {
            running = true;
            break;
        }
    }

    if(!running) {
        uint32_t i;
        for(i = 0; i < obj->style_cnt; i++) {
            if(obj->styles[i].is_trans && obj->styles[i].selector == tr->selector) {
                _lv_ll_remove(&LV_GC_ROOT(_lv_obj_style_trans_ll), tr);
                lv_mem_free(tr);

                _lv_obj_style_t * obj_style = &obj->styles[i];
                lv_style_remove_prop(obj_style->style, prop);

                if(lv_style_is_empty(obj->styles[i].style)) {
                    lv_obj_remove_style(obj, obj_style->style, obj_style->selector);

                }
                break;
            }
        }
    }
}

static void fade_anim_cb(void * obj, int32_t v)
{
    lv_obj_set_style_opa(obj, v, 0);
}

static void fade_in_anim_ready(lv_anim_t * a)
{
    lv_obj_remove_local_style_prop(a->var, LV_STYLE_OPA, 0);
}


