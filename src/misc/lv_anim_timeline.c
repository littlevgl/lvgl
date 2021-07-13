/**
 * @file lv_anim_timeline.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_anim_timeline.h"
#include "lv_mem.h"
#include "../misc/lv_assert.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Data of anim_timeline_dsc*/
typedef struct{
    lv_anim_t anim;
    lv_anim_t * new_anim;
    uint32_t start_time;
}lv_anim_timeline_dsc_t;

/*Data of anim_timeline*/
struct _lv_anim_timeline_t{
    lv_anim_timeline_dsc_t * anim_dsc;  /**< Dynamically allocated anim dsc array*/
    uint32_t anim_dsc_cnt;              /**< The length of anim dsc array*/
    bool reverse;                       /**< Reverse playback*/
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_anim_timeline_t * lv_anim_timeline_create(void)
{
    lv_anim_timeline_t * at = (lv_anim_timeline_t *)lv_mem_alloc(sizeof(lv_anim_timeline_t));

    LV_ASSERT_MALLOC(at);

    if(at) lv_memset_00(at, sizeof(lv_anim_timeline_t));

    return at;
}

void lv_anim_timeline_del(lv_anim_timeline_t * at)
{
    LV_ASSERT_NULL(at);

    for(uint32_t i = 0; i < at->anim_dsc_cnt; i++) {
        lv_anim_t * a = &(at->anim_dsc[i].anim);
#if LV_ANIM_TIMELINE_CUSTOM_EXEC
        lv_anim_custom_del(at->anim_dsc[i].new_anim, (lv_anim_custom_exec_cb_t)a->exec_cb);
#else
        lv_anim_del(a->var, a->exec_cb);
#endif
    }

    lv_mem_free(at->anim_dsc);
    lv_mem_free(at);
}

void lv_anim_timeline_add(lv_anim_timeline_t * at, uint32_t start_time, lv_anim_t * a)
{
    LV_ASSERT_NULL(at);

    at->anim_dsc_cnt++;
    at->anim_dsc = lv_mem_realloc(at->anim_dsc, at->anim_dsc_cnt * sizeof(lv_anim_timeline_dsc_t));

    LV_ASSERT_MALLOC(at->anim_dsc);

    at->anim_dsc[at->anim_dsc_cnt - 1].anim = *a;
    at->anim_dsc[at->anim_dsc_cnt - 1].new_anim = NULL;
    at->anim_dsc[at->anim_dsc_cnt - 1].start_time = start_time;
}

uint32_t lv_anim_timeline_start(lv_anim_timeline_t * at)
{
    LV_ASSERT_NULL(at);

    const uint32_t playtime = lv_anim_timeline_get_playtime(at);
    bool reverse = at->reverse;

    for(uint32_t i = 0; i < at->anim_dsc_cnt; i++) {
        lv_anim_t a = at->anim_dsc[i].anim;
        uint32_t start_time = at->anim_dsc[i].start_time;

        if(reverse) {
            int32_t temp = a.start_value;
            a.start_value = a.end_value;
            a.end_value = temp;
            lv_anim_set_delay(&a, playtime - (start_time + a.time));
        }
        else {
            lv_anim_set_delay(&a, start_time);
        }

        at->anim_dsc[i].new_anim = lv_anim_start(&a);
    }

    return playtime;
}

void lv_anim_timeline_set_reverse(lv_anim_timeline_t * at, bool reverse)
{
    LV_ASSERT_NULL(at);
    at->reverse = reverse;
}

void lv_anim_timeline_set_progress(lv_anim_timeline_t * at, uint16_t progress)
{
    LV_ASSERT_NULL(at);

    const uint32_t playtime = lv_anim_timeline_get_playtime(at);
    const uint32_t act_time = progress * playtime / 0xFFFF;

    for(uint32_t i = 0; i < at->anim_dsc_cnt; i++) {
        lv_anim_t * a = &(at->anim_dsc[i].anim);
        uint32_t start_time = at->anim_dsc[i].start_time;
        int32_t value = 0;

        if(act_time < start_time) {
            value = a->start_value;
        }
        else if(act_time < (start_time + a->time)) {
            a->act_time = act_time - start_time;
            value = a->path_cb(a);
        }
        else {
            value = a->end_value;
        }

        a->exec_cb(a->var, value);
    }
}

uint32_t lv_anim_timeline_get_playtime(lv_anim_timeline_t * at)
{
    LV_ASSERT_NULL(at);

    uint32_t playtime = 0;
    for(uint32_t i = 0; i < at->anim_dsc_cnt; i++) {
        uint32_t end = at->anim_dsc[i].start_time + at->anim_dsc[i].anim.time;
        if(end > playtime) {
            playtime = end;
        }
    }

    return playtime;
}

bool lv_anim_timeline_get_reverse(lv_anim_timeline_t * at)
{
    LV_ASSERT_NULL(at);
    return at->reverse;
}
