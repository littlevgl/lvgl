/**
 * @file lv_anim_timeline.h
 *
 */

#ifndef LV_ANIM_TIMELINE_H
#define LV_ANIM_TIMELINE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_anim.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_anim_timeline_t;

typedef struct _lv_anim_timeline_t lv_anim_timeline_t;

/**********************
* GLOBAL PROTOTYPES
**********************/

/**
 * Create a animation timeline.
 * @return pointer to the animation timeline.
 */
lv_anim_timeline_t * lv_anim_timeline_create(void);

/**
 * Delete animation timeline.
 * @param at    pointer to the animation timeline.
 */
void lv_anim_timeline_del(lv_anim_timeline_t * at);

/**
 * Add animation to the animation timeline.
 * @param at            pointer to the animation timeline.
 * @param start_time    the time the animation started on the timeline, note that start_time will override the value of delay.
 * @param a             pointer to an animation.
 */
void lv_anim_timeline_add(lv_anim_timeline_t * at, uint32_t start_time, lv_anim_t * a);

/**
 * Start the animation timeline.
 * @param at    pointer to the animation timeline.
 * @return total time spent in animation timeline.
 */
uint32_t lv_anim_timeline_start(lv_anim_timeline_t * at);

/**
 * Set the playback direction of the animation timeline.
 * @param at        pointer to the animation timeline.
 * @param reverse   whether to play in reverse.
 */
void lv_anim_timeline_set_reverse(lv_anim_timeline_t * at, bool reverse);

/**
 * Set the progress of the animation timeline.
 * @param at        pointer to the animation timeline.
 * @param progress  set value 0~65535 to map 0~100% animation progress.
 */
void lv_anim_timeline_set_progress(lv_anim_timeline_t * at, uint16_t progress);

/**
 * Get the time used to play the animation timeline.
 * @param at    pointer to the animation timeline.
 * @return total time spent in animation timeline.
 */
uint32_t lv_anim_timeline_get_playtime(lv_anim_timeline_t * at);

/**
 * Get whether the animation timeline is played in reverse.
 * @param at    pointer to the animation timeline.
 * @return return true if it is reverse playback.
 */
bool lv_anim_timeline_get_reverse(lv_anim_timeline_t * at);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_ANIM_TIMELINE_H*/
