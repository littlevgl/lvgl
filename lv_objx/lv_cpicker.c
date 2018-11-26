/**
 * @file lv_cpicker.c
 *
 */

/* TODO Remove these instructions
 * Search an replace: color_picker -> object normal name with lower case (e.g. button, label etc.)
 *                    cpicker -> object short name with lower case(e.g. btn, label etc)
 *                    CPICKER -> object short name with upper case (e.g. BTN, LABEL etc.)
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_cpicker.h"
#include "../lv_misc/lv_math.h"
#include "../lv_draw/lv_draw_arc.h"
#include "../lv_themes/lv_theme.h"
#include "../lv_core/lv_indev.h"
#include "../lv_core/lv_refr.h"

#if USE_LV_CPICKER != 0

/*********************
 *      DEFINES
 *********************/
#ifndef LV_CPICKER_DEF_TYPE
#define LV_CPICKER_DEF_TYPE LV_CPICKER_TYPE_DISC
#endif

#ifndef LV_CPICKER_DEF_HUE
#define LV_CPICKER_DEF_HUE 0
#endif

#ifndef LV_CPICKER_DEF_SAT
#define LV_CPICKER_DEF_SAT 100
#endif

#ifndef LV_CPICKER_DEF_VAL
#define LV_CPICKER_DEF_VAL 100
#endif

#ifndef LV_CPICKER_DEF_IND_TYPE
#define LV_CPICKER_DEF_IND_TYPE LV_CPICKER_IND_CIRCLE
#endif

#ifndef LV_CPICKER_DEF_QF /*quantization factor*/
#define LV_CPICKER_DEF_QF 1
#endif

/*for rectangular mode the QF can be down to 1*/
/*
#define LV_CPICKER_MINIMUM_QF 4
#if LV_CPICKER_DEF_QF < LV_CPICKER_MINIMUM_QF
#undef LV_CPICKER_DEF_QF
#define LV_CPICKER_DEF_QF LV_CPICKER_MINIMUM_QF
#endif
 */

#ifndef LV_CPICKER_USE_TRI /*Use triangle approximation instead of arc*/
#define LV_CPICKER_USE_TRI 1
#endif

#define TRI_OFFSET 4

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool lv_cpicker_disc_design(lv_obj_t * cpicker, const lv_area_t * mask, lv_design_mode_t mode);
static lv_res_t lv_cpicker_disc_signal(lv_obj_t * cpicker, lv_signal_t sign, void * param);

static bool lv_cpicker_rect_design(lv_obj_t * cpicker, const lv_area_t * mask, lv_design_mode_t mode);
static lv_res_t lv_cpicker_rect_signal(lv_obj_t * cpicker, lv_signal_t sign, void * param);

static void lv_cpicker_invalidate_indicator(lv_obj_t * cpicker);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_signal_func_t ancestor_signal;
static lv_design_func_t ancestor_design;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Create a color_picker object
 * @param par pointer to an object, it will be the parent of the new color_picker
 * @param copy pointer to a color_picker object, if not NULL then the new object will be copied from it
 * @return pointer to the created color_picker
 */
lv_obj_t * lv_cpicker_create(lv_obj_t * par, const lv_obj_t * copy)
{
    LV_LOG_TRACE("color_picker create started");

    /*Create the ancestor of color_picker*/
    /*TODO modify it to the ancestor create function */
    lv_obj_t * new_cpicker = lv_obj_create(par, copy);
    lv_mem_assert(new_cpicker);
    if(new_cpicker == NULL) return NULL;

    /*Allocate the colorpicker type specific extended data*/
    lv_cpicker_ext_t * ext = lv_obj_allocate_ext_attr(new_cpicker, sizeof(lv_cpicker_ext_t));
    lv_mem_assert(ext);
    if(ext == NULL) return NULL;
    if(ancestor_signal == NULL) ancestor_signal = lv_obj_get_signal_func(new_cpicker);
    if(ancestor_design == NULL) ancestor_design = lv_obj_get_design_func(new_cpicker);

    /*Initialize the allocated 'ext' */
    ext->hue = LV_CPICKER_DEF_HUE;
    ext->prev_hue = ext->hue;
    ext->saturation = LV_CPICKER_DEF_SAT;
    ext->value = LV_CPICKER_DEF_VAL;
    ext->ind.style = &lv_style_plain;
    ext->ind.type = LV_CPICKER_DEF_IND_TYPE;
    ext->value_changed = NULL;
    ext->wheel_mode = LV_CPICKER_WHEEL_HUE;
    ext->wheel_fixed = 0;
    ext->last_clic = 0;
    ext->type = LV_CPICKER_DEF_TYPE;

    /*The signal and design functions are not copied so set them here*/
    if(ext->type == LV_CPICKER_TYPE_DISC)
    {
        lv_obj_set_signal_func(new_cpicker, lv_cpicker_disc_signal);
        lv_obj_set_design_func(new_cpicker, lv_cpicker_disc_design);
    }
    else if(ext->type == LV_CPICKER_TYPE_RECT)
    {
        lv_obj_set_signal_func(new_cpicker, lv_cpicker_rect_signal);
        lv_obj_set_design_func(new_cpicker, lv_cpicker_rect_design);
    }

    /*Init the new cpicker color_picker*/
    if(copy == NULL) {

        /*Set the default styles*/
        lv_theme_t * th = lv_theme_get_current();
        if(th) {
            lv_cpicker_set_style(new_cpicker, LV_CPICKER_STYLE_MAIN, th->bg);
        } else {
            lv_cpicker_set_style(new_cpicker, LV_CPICKER_STYLE_MAIN, &lv_style_plain);
        }
    }

    /*Copy an existing color_picker*/
    else {
        lv_cpicker_ext_t * copy_ext = lv_obj_get_ext_attr(copy);

        /*Refresh the style with new signal function*/
        lv_obj_refresh_style(new_cpicker);
    }

    LV_LOG_INFO("colorpicker created");

    return new_cpicker;
}

/*======================
 * Add/remove functions
 *=====================*/

/*
 * New object specific "add" or "remove" functions come here
 */


/*=====================
 * Setter functions
 *====================*/

/*
 * New object specific "set" functions come here
 */


/**
 * Set a style of a color_picker.
 * @param cpicker pointer to color_picker object
 * @param type which style should be set
 * @param style pointer to a style
 */
void lv_cpicker_set_style(lv_obj_t * cpicker, lv_cpicker_style_t type, lv_style_t * style)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    switch(type) {
    case LV_CPICKER_STYLE_MAIN:
        lv_obj_set_style(cpicker, style);
        break;
    case LV_CPICKER_STYLE_IND:
        ext->ind.style = style;
        lv_obj_invalidate(cpicker);
        break;
    }
}

/**
 * Set a type of a colorpicker indicator.
 * @param cpicker pointer to colorpicker object
 * @param type indicator type
 */
void lv_cpicker_set_ind_type(lv_obj_t * cpicker, lv_cpicker_ind_type_t type)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);
    ext->ind.type = type;
    lv_obj_invalidate(cpicker);
}

/**
 * Set the current hue of a colorpicker.
 * @param cpicker pointer to colorpicker object
 * @param hue current selected hue [0..360]
 */
void lv_cpicker_set_hue(lv_obj_t * cpicker, uint16_t hue)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    ext->hue = hue % 360;

    lv_obj_invalidate(cpicker);
}

/**
 * Set the current saturation of a colorpicker.
 * @param cpicker pointer to colorpicker object
 * @param sat current selected saturation [0..100]
 */
void lv_cpicker_set_saturation(lv_obj_t * cpicker, uint16_t sat)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    ext->saturation = sat % 100;

    lv_obj_invalidate(cpicker);
}

/**
 * Set the current value of a colorpicker.
 * @param cpicker pointer to colorpicker object
 * @param val current selected value [0..100]
 */
void lv_cpicker_set_value(lv_obj_t * cpicker, uint16_t val)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    ext->value = val % 100;

    lv_obj_invalidate(cpicker);
}

/**
 * Set the current color of a colorpicker.
 * @param cpicker pointer to colorpicker object
 * @param color current selected color
 */
void lv_cpicker_set_color(lv_obj_t * cpicker, lv_color_t color)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    lv_color_hsv_t hsv = lv_color_rgb_to_hsv(color.red, color.green, color.blue);
    ext->hue = hsv.h;

    lv_obj_invalidate(cpicker);
}

/**
 * Set the action callback on value change event.
 * @param cpicker pointer to colorpicker object
 * @param action callback function
 */
void lv_cpicker_set_action(lv_obj_t * cpicker, lv_action_t action)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    ext->value_changed = action;
}

/**
 * Set the current wheel mode.
 * @param cpicker pointer to colorpicker object
 * @param mode wheel mode (hue/sat/val)
 */
void lv_cpicker_set_wheel_mode(lv_obj_t * cpicker, lv_cpicker_wheel_mode_t mode)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    ext->wheel_mode = mode;
}

/**
 * Set if the wheel mode is changed on long press on center
 * @param cpicker pointer to colorpicker object
 * @param fixed_mode mode cannot be changed if set
 */
void lv_cpicker_set_wheel_fixed(lv_obj_t * cpicker, bool fixed_mode)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    ext->wheel_fixed = fixed_mode;
}

/*=====================
 * Getter functions
 *====================*/

/*
 * New object specific "get" functions come here
 */

/**
 * Get style of a color_picker.
 * @param cpicker pointer to color_picker object
 * @param type which style should be get
 * @return style pointer to the style
 */
lv_style_t * lv_cpicker_get_style(const lv_obj_t * cpicker, lv_cpicker_style_t type)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    switch(type) {
    case LV_CPICKER_STYLE_MAIN:
        return lv_obj_get_style(cpicker);
    case LV_CPICKER_STYLE_IND:
        return ext->ind.style;
    default:
        return NULL;
    }

    /*To avoid warning*/
    return NULL;
}

/**
 * Get the current hue of a colorpicker.
 * @param cpicker pointer to colorpicker object
 * @return hue current selected hue
 */
uint16_t lv_cpicker_get_hue(lv_obj_t * cpicker)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    return ext->hue;
}

/**
 * Get the current selected color of a colorpicker.
 * @param cpicker pointer to colorpicker object
 * @return color current selected color
 */
lv_color_t lv_cpicker_get_color(lv_obj_t * cpicker)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    return lv_color_hsv_to_rgb(ext->hue, ext->saturation, ext->value);
}

/**
 * Get the action callback called on value change event.
 * @param cpicker pointer to colorpicker object
 * @return action callback function
 */
lv_action_t lv_cpicker_get_action(lv_obj_t * cpicker)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    return ext->value_changed;
}

/*=====================
 * Other functions
 *====================*/

/*
 * New object specific "other" functions come here
 */

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Handle the drawing related tasks of the color_pickerwhen when wheel type
 * @param cpicker pointer to an object
 * @param mask the object will be drawn only in this area
 * @param mode LV_DESIGN_COVER_CHK: only check if the object fully covers the 'mask_p' area
 *                                  (return 'true' if yes)
 *             LV_DESIGN_DRAW: draw the object (always return 'true')
 *             LV_DESIGN_DRAW_POST: drawing after every children are drawn
 * @param return true/false, depends on 'mode'
 */
static bool lv_cpicker_disc_design(lv_obj_t * cpicker, const lv_area_t * mask, lv_design_mode_t mode)
{
    /*Return false if the object is not covers the mask_p area*/
    if(mode == LV_DESIGN_COVER_CHK) {
        return false;
    }
    /*Draw the object*/
    else if(mode == LV_DESIGN_DRAW_MAIN) {

        lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);
        lv_style_t * style = lv_cpicker_get_style(cpicker, LV_CPICKER_STYLE_MAIN);

        static lv_style_t styleCopy;
        lv_style_copy(&styleCopy, style);

        lv_coord_t r = (LV_MATH_MIN(lv_obj_get_width(cpicker), lv_obj_get_height(cpicker))) / 2;
        lv_coord_t x = cpicker->coords.x1 + lv_obj_get_width(cpicker) / 2;
        lv_coord_t y = cpicker->coords.y1 + lv_obj_get_height(cpicker) / 2;
        lv_opa_t opa_scale = lv_obj_get_opa_scale(cpicker);

        uint8_t redraw_wheel = 0;

        lv_area_t center_ind_area;

        uint32_t rin = r - styleCopy.line.width;
        //the square area (a and b being sides) should fit into the center of diameter d
        //we have:
        //a^2+b^2<=d^2
        //2a^2 <= d^2
        //a^2<=(d^2)/2
        //a <= sqrt((d^2)/2)
        uint16_t radius = lv_sqrt((4*rin*rin)/2)/2 - style->body.padding.inner;

        center_ind_area.x1 = x - radius;
        center_ind_area.y1 = y - radius;
        center_ind_area.x2 = x + radius;
        center_ind_area.y2 = y + radius;

        /*redraw the wheel only if the mask intersect with the wheel*/
        if(mask->x1 < center_ind_area.x1 || mask->x2 > center_ind_area.x2
                || mask->y1 < center_ind_area.y1 || mask->y2 > center_ind_area.y2)
        {
            redraw_wheel = 1;
        }

        lv_point_t triangle_points[3];

        int16_t start_angle, end_angle;
        start_angle = 0; //Default
        end_angle = 360 - LV_CPICKER_DEF_QF; //Default

        if(redraw_wheel)
        {
            /*if the mask does not include the center of the object
             * redrawing all the wheel is not necessary;
             * only a given angular range
             */
            lv_point_t center = {x, y};
            if(!lv_area_is_point_on(mask, &center)
                    /*
              && (mask->x1 != cpicker->coords.x1 || mask->x2 != cpicker->coords.x2
              ||  mask->y1 != cpicker->coords.y1 || mask->y2 != cpicker->coords.y2)
                     */
            )
            {
                /*get angle from center of object to each corners of the area*/
                int16_t dr, ur, ul, dl;
                dr = lv_atan2(mask->x2 - x, mask->y2 - y);
                ur = lv_atan2(mask->x2 - x, mask->y1 - y);
                ul = lv_atan2(mask->x1 - x, mask->y1 - y);
                dl = lv_atan2(mask->x1 - x, mask->y2 - y);

                /* check area position from object axis*/
                uint8_t left = (mask->x2 < x && mask->x1 < x);
                uint8_t onYaxis = (mask->x2 > x && mask->x1 < x);
                uint8_t right = (mask->x2 > x && mask->x1 > x);
                uint8_t top = (mask->y2 < y && mask->y1 < y);
                uint8_t onXaxis = (mask->y2 > y && mask->y1 < y);
                uint8_t bottom = (mask->y2 > y && mask->y1 > x);

                /*store angular range*/
                if(right && bottom)
                {
                    start_angle = dl;
                    end_angle = ur;
                }
                else if(right && onXaxis)
                {
                    start_angle = dl;
                    end_angle = ul;
                }
                else if(right && top)
                {
                    start_angle = dr;
                    end_angle = ul;
                }
                else if(onYaxis && top)
                {
                    start_angle = dr;
                    end_angle = dl;
                }
                else if(left && top)
                {
                    start_angle = ur;
                    end_angle = dl;
                }
                else if(left && onXaxis)
                {
                    start_angle = ur;
                    end_angle = dr;
                }
                else if(left && bottom)
                {
                    start_angle = ul;
                    end_angle = dr;
                }
                else if(onYaxis && bottom)
                {
                    start_angle = ul;
                    end_angle = ur;
                }

                /*rollover angle*/
                if(start_angle > end_angle)
                {
                    end_angle +=  360;
                }

                /*round to QF factor*/
                start_angle = start_angle/LV_CPICKER_DEF_QF*LV_CPICKER_DEF_QF;
                end_angle = end_angle/LV_CPICKER_DEF_QF*LV_CPICKER_DEF_QF;;

                /*shift angle if necessary before adding offset*/
                if((start_angle - LV_CPICKER_DEF_QF) < 0)
                {
                    start_angle += 360;
                    end_angle += 360;
                }

                /*ensure overlapping by adding offset*/
                start_angle -= LV_CPICKER_DEF_QF;
                end_angle += LV_CPICKER_DEF_QF;
            }

            if(ext->wheel_mode == LV_CPICKER_WHEEL_HUE)
            {
                for(uint16_t i = start_angle; i <= end_angle; i+= LV_CPICKER_DEF_QF)
                {
                    styleCopy.line.color = lv_color_hsv_to_rgb(i%360, ext->saturation, ext->value);

                    triangle_points[0].x = x;
                    triangle_points[0].y = y;

                    triangle_points[1].x = x + (r * lv_trigo_sin(i) >> LV_TRIGO_SHIFT);
                    triangle_points[1].y = y + (r * lv_trigo_sin(i + 90) >> LV_TRIGO_SHIFT);


                    if(i == end_angle || i == (360 - LV_CPICKER_DEF_QF))
                    {
                        /*the last triangle is drawn without additional overlapping pixels*/
                        triangle_points[2].x = x + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF) >> LV_TRIGO_SHIFT);
                        triangle_points[2].y = y + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + 90) >> LV_TRIGO_SHIFT);

                    }
                    else
                    {
                        triangle_points[2].x = x + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + TRI_OFFSET) >> LV_TRIGO_SHIFT);
                        triangle_points[2].y = y + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + TRI_OFFSET + 90) >> LV_TRIGO_SHIFT);

                    }

                    lv_draw_triangle(triangle_points, mask, styleCopy.line.color);
                }
            }
            else if(ext->wheel_mode == LV_CPICKER_WHEEL_SAT)
            {
                for(uint16_t i = start_angle; i <= end_angle; i += LV_CPICKER_DEF_QF)
                {
                    styleCopy.line.color = lv_color_hsv_to_rgb(ext->hue, (i%360)*100/360, ext->value);

                    triangle_points[0].x = x;
                    triangle_points[0].y = y;

                    triangle_points[1].x = x + (r * lv_trigo_sin(i) >> LV_TRIGO_SHIFT);
                    triangle_points[1].y = y + (r * lv_trigo_sin(i + 90) >> LV_TRIGO_SHIFT);

                    if(i == end_angle || i == (360 - LV_CPICKER_DEF_QF))
                    {
                        /*the last triangle is drawn without additional overlapping pixels*/
                        triangle_points[2].x = x + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF) >> LV_TRIGO_SHIFT);
                        triangle_points[2].y = y + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + 90) >> LV_TRIGO_SHIFT);
                    }
                    else
                    {
                        triangle_points[2].x = x + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + TRI_OFFSET) >> LV_TRIGO_SHIFT);
                        triangle_points[2].y = y + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + TRI_OFFSET + 90) >> LV_TRIGO_SHIFT);
                    }

                    lv_draw_triangle(triangle_points, mask, styleCopy.line.color);
                }
            }
            else if(ext->wheel_mode == LV_CPICKER_WHEEL_VAL)
            {
                for(uint16_t i = start_angle; i <= end_angle; i += LV_CPICKER_DEF_QF)
                {
                    styleCopy.line.color = lv_color_hsv_to_rgb(ext->hue, ext->saturation, (i%360)*100/360);

                    triangle_points[0].x = x;
                    triangle_points[0].y = y;

                    triangle_points[1].x = x + (r * lv_trigo_sin(i) >> LV_TRIGO_SHIFT);
                    triangle_points[1].y = y + (r * lv_trigo_sin(i + 90) >> LV_TRIGO_SHIFT);

                    if(i == end_angle || i == (360 - LV_CPICKER_DEF_QF))
                    {
                        /*the last triangle is drawn without additional overlapping pixels*/
                        triangle_points[2].x = x + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF) >> LV_TRIGO_SHIFT);
                        triangle_points[2].y = y + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + 90) >> LV_TRIGO_SHIFT);

                    }
                    else
                    {
                        triangle_points[2].x = x + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + TRI_OFFSET) >> LV_TRIGO_SHIFT);
                        triangle_points[2].y = y + (r * lv_trigo_sin(i + LV_CPICKER_DEF_QF + TRI_OFFSET + 90) >> LV_TRIGO_SHIFT);
                    }

                    lv_draw_triangle(triangle_points, mask, styleCopy.line.color);
                }
            }
        }

        //draw center background
        lv_area_t center_area;
        uint16_t wradius = r - styleCopy.line.width;
        center_area.x1 = x - wradius;
        center_area.y1 = y - wradius;
        center_area.x2 = x + wradius;
        center_area.y2 = y + wradius;
	styleCopy.body.grad_color = styleCopy.body.main_color;
        styleCopy.body.radius = LV_RADIUS_CIRCLE;
        lv_draw_rect(&center_area, mask, &styleCopy, opa_scale);

        //draw the center color indicator
        styleCopy.body.main_color = lv_color_hsv_to_rgb(ext->hue, ext->saturation, ext->value);
        styleCopy.body.grad_color = styleCopy.body.main_color;
        styleCopy.body.radius = LV_RADIUS_CIRCLE;
        lv_draw_rect(&center_ind_area, mask, &styleCopy, opa_scale);

        //Draw the current hue indicator
        switch(ext->ind.type)
        {
        case LV_CPICKER_IND_NONE:
            break;
        case LV_CPICKER_IND_LINE:
        {
            lv_point_t start;
            lv_point_t end;

            uint16_t angle;

            switch(ext->wheel_mode)
            {
            default:
            case LV_CPICKER_WHEEL_HUE:
                angle = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                angle = ext->saturation * 360 / 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                angle = ext->value * 360 / 100;
                break;
            }

            /*save the angle to refresh the area later*/
            ext->prev_pos = angle;

            start.x = x + ((r - style->line.width + ext->ind.style->body.padding.inner + ext->ind.style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            start.y = y + ((r - style->line.width + ext->ind.style->body.padding.inner + ext->ind.style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            end.x = x + ((r - ext->ind.style->body.padding.inner - ext->ind.style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            end.y = y + ((r - ext->ind.style->body.padding.inner - ext->ind.style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            lv_draw_line(&start, &end, mask, ext->ind.style, opa_scale);
            if(ext->ind.style->line.rounded)
            {
                lv_area_t circle_area;
                circle_area.x1 = start.x - ((ext->ind.style->line.width - 1) >> 1) - ((ext->ind.style->line.width - 1) & 0x1);
                circle_area.y1 = start.y - ((ext->ind.style->line.width - 1) >> 1) - ((ext->ind.style->line.width - 1) & 0x1);
                circle_area.x2 = start.x + ((ext->ind.style->line.width - 1) >> 1);
                circle_area.y2 = start.y + ((ext->ind.style->line.width - 1) >> 1);
                lv_draw_rect(&circle_area, mask, ext->ind.style, opa_scale);

                circle_area.x1 = end.x - ((ext->ind.style->line.width - 1) >> 1) - ((ext->ind.style->line.width - 1) & 0x1);
                circle_area.y1 = end.y - ((ext->ind.style->line.width - 1) >> 1) - ((ext->ind.style->line.width - 1) & 0x1);
                circle_area.x2 = end.x + ((ext->ind.style->line.width - 1) >> 1);
                circle_area.y2 = end.y + ((ext->ind.style->line.width - 1) >> 1);
                lv_draw_rect(&circle_area, mask, ext->ind.style, opa_scale);
            }
            break;
        }
        case LV_CPICKER_IND_CIRCLE:
        {
            lv_area_t circle_area;
            uint32_t cx, cy;

            uint16_t angle;

            switch(ext->wheel_mode)
            {
            default:
            case LV_CPICKER_WHEEL_HUE:
                angle = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                angle = ext->saturation * 360 / 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                angle = ext->value * 360 / 100;
                break;
            }

            /*save the angle to refresh the area later*/
            ext->prev_pos = angle;

            cx = x + ((r - style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            cy = y + ((r - style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            circle_area.x1 = cx - style->line.width/2;
            circle_area.y1 = cy - style->line.width/2;
            circle_area.x2 = cx + style->line.width/2;
            circle_area.y2 = cy + style->line.width/2;

            ext->ind.style->body.radius = LV_RADIUS_CIRCLE;
            lv_draw_rect(&circle_area, mask, ext->ind.style, opa_scale);
            break;
        }
        case LV_CPICKER_IND_IN:
        {
            lv_area_t circle_area;
            uint32_t cx, cy;

            uint16_t angle;

            switch(ext->wheel_mode)
            {
            default:
            case LV_CPICKER_WHEEL_HUE:
                angle = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                angle = ext->saturation * 360 / 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                angle = ext->value * 360 / 100;
                break;
            }

            /*save the angle to refresh the area later*/
            ext->prev_pos = angle;

            uint16_t ind_radius = lv_sqrt((4*rin*rin)/2)/2 + 1 - style->body.padding.inner;
            ind_radius = (ind_radius + rin) / 2;

            cx = x + ((ind_radius) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            cy = y + ((ind_radius) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            circle_area.x1 = cx - ext->ind.style->line.width/2;
            circle_area.y1 = cy - ext->ind.style->line.width/2;
            circle_area.x2 = cx + ext->ind.style->line.width/2;
            circle_area.y2 = cy + ext->ind.style->line.width/2;

            ext->ind.style->body.radius = LV_RADIUS_CIRCLE;
            lv_draw_rect(&circle_area, mask, ext->ind.style, opa_scale);
            break;
        }
        }


        /*
        //code to color the drawn area
        static uint32_t c = 0;
        lv_style_t style2;
        lv_style_copy(&style2, &lv_style_plain);
        style2.body.main_color.full = c;
        style2.body.grad_color.full = c;
        c += 0x123445678;
        lv_draw_rect(mask, mask, &style2, opa_scale);
         */
    }
    /*Post draw when the children are drawn*/
    else if(mode == LV_DESIGN_DRAW_POST) {

    }

    return true;
}

/**
 * Handle the drawing related tasks of the color_pickerwhen of rectangle type
 * @param cpicker pointer to an object
 * @param mask the object will be drawn only in this area
 * @param mode LV_DESIGN_COVER_CHK: only check if the object fully covers the 'mask_p' area
 *                                  (return 'true' if yes)
 *             LV_DESIGN_DRAW: draw the object (always return 'true')
 *             LV_DESIGN_DRAW_POST: drawing after every children are drawn
 * @param return true/false, depends on 'mode'
 */
static bool lv_cpicker_rect_design(lv_obj_t * cpicker, const lv_area_t * mask, lv_design_mode_t mode)
{
    /*Return false if the object is not covers the mask_p area*/
    if(mode == LV_DESIGN_COVER_CHK) {
        return false;
    }
    /*Draw the object*/
    else if(mode == LV_DESIGN_DRAW_MAIN) {

        lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);
        lv_style_t * style = lv_cpicker_get_style(cpicker, LV_CPICKER_STYLE_MAIN);

        static lv_style_t styleCopy;
        lv_style_copy(&styleCopy, style);

        lv_coord_t w = lv_obj_get_width(cpicker);
        lv_coord_t h = lv_obj_get_height(cpicker);

        lv_coord_t gradient_w, gradient_h;
        lv_area_t gradient_area;

        lv_coord_t x1 = cpicker->coords.x1;
        lv_coord_t y1 = cpicker->coords.y1;
        lv_coord_t x2 = cpicker->coords.x2;
        lv_coord_t y2 = cpicker->coords.y2;
        lv_opa_t opa_scale = lv_obj_get_opa_scale(cpicker);


        /* prepare the color preview area */
        uint16_t preview_offset = style->line.width;
        lv_area_t preview_area;
        if(style->body.padding.ver == 0)
        {
            /* draw the color preview rect to the side of the gradient*/
            if(style->body.padding.hor >= 0)
            {
                /*draw the preview to the right*/
                gradient_w = w - preview_offset - (LV_MATH_ABS(style->body.padding.hor) - 1);
                gradient_h = y2 - y1;
                gradient_area.x1 = x1;
                gradient_area.x2 = gradient_area.x1 + gradient_w;
                gradient_area.y1 = y1;
                gradient_area.y2 = y2;

                preview_area.x1 = x2 - preview_offset;
                preview_area.y1 = y1;
                preview_area.x2 = x2 ;
                preview_area.y2 = y2;
            }
            else
            {
                /*draw the preview to the left*/
                gradient_w = w - preview_offset - (LV_MATH_ABS(style->body.padding.hor) - 1);
                gradient_h = y2 - y1;
                gradient_area.x1 = x2 - gradient_w;
                gradient_area.x2 = x2;
                gradient_area.y1 = y1;
                gradient_area.y2 = y2;

                preview_area.x1 = x1;
                preview_area.y1 = y1;
                preview_area.x2 = x1 + preview_offset;
                preview_area.y2 = y2;
            }
        }
        else
        {
            /* draw the color preview rect on top or below the gradient*/
            if(style->body.padding.ver >= 0)
            {
                /*draw the preview on top*/
                gradient_w = w;
                gradient_h = (y2 - y1) - preview_offset - (LV_MATH_ABS(style->body.padding.ver) - 1);
                gradient_area.x1 = x1;
                gradient_area.x2 = x2;
                gradient_area.y1 = y2 - gradient_h;
                gradient_area.y2 = y2;

                preview_area.x1 = x1;
                preview_area.y1 = y1;
                preview_area.x2 = x2;
                preview_area.y2 = y1 + preview_offset;
            }
            else
            {
                /*draw the preview below the gradient*/
                gradient_w = w;
                gradient_h = (y2 - y1) - preview_offset - (LV_MATH_ABS(style->body.padding.ver) - 1);
                gradient_area.x1 = x1;
                gradient_area.x2 = x2;
                gradient_area.y1 = y1;
                gradient_area.y2 = y1 + gradient_h;

                preview_area.x1 = x1;
                preview_area.y1 = y2 - preview_offset;
                preview_area.x2 = x2;
                preview_area.y2 = y2;
            }
        }

        for(uint16_t i = 0; i < 360; i += LV_MATH_MAX(LV_CPICKER_DEF_QF, 360/gradient_w))
        {
            switch(ext->wheel_mode)
            {
            default:
            case LV_CPICKER_WHEEL_HUE:
                styleCopy.body.main_color = lv_color_hsv_to_rgb(i%360, ext->saturation, ext->value);
                break;
            case LV_CPICKER_WHEEL_SAT:
                styleCopy.body.main_color = lv_color_hsv_to_rgb(ext->hue, (i%360)*100/360, ext->value);
                break;
            case LV_CPICKER_WHEEL_VAL:
                styleCopy.body.main_color = lv_color_hsv_to_rgb(i%360, ext->saturation, (i%360)*100/360);
                break;
            }

            styleCopy.body.grad_color = styleCopy.body.main_color;

            /*the following attribute might need changing between index to add border, shadow, radius etc*/
            styleCopy.body.radius = 0;
            styleCopy.body.border.width = 0;
            styleCopy.body.border.width = 0;

            lv_area_t rect_area;

            /*scale angle (hue/sat/val) to linear coordinate*/
            lv_coord_t xi = i*gradient_w/360;

            rect_area.x1 = LV_MATH_MIN(gradient_area.x1 + xi, gradient_area.x1 + gradient_w - LV_MATH_MAX(LV_CPICKER_DEF_QF, 360/gradient_w));
            rect_area.y1 = gradient_area.y1;
            rect_area.x2 = rect_area.x1 + LV_MATH_MAX(LV_CPICKER_DEF_QF, 360/gradient_w);
            rect_area.y2 = gradient_area.y2;

            lv_draw_rect(&rect_area, mask, &styleCopy, opa_scale);
        }

        /*draw the color preview indicator*/
        styleCopy.body.main_color = lv_cpicker_get_color(cpicker);
        styleCopy.body.grad_color = styleCopy.body.main_color;
        lv_draw_rect(&preview_area, mask, &styleCopy, opa_scale);


        /*draw the color position indicator*/
        lv_coord_t ind_pos;
        switch(ext->wheel_mode)
        {
        default:
        case LV_CPICKER_WHEEL_HUE:
            ind_pos = ext->hue * gradient_w /360;
            break;
        case LV_CPICKER_WHEEL_SAT:
            ind_pos = ext->saturation * 360 / 100 * gradient_w/360;
            break;
        case LV_CPICKER_WHEEL_VAL:
            ind_pos = ext->value * 360 / 100 * gradient_w/360;
            break;
        }

        switch(ext->ind.type)
        {
        case LV_CPICKER_IND_NONE:
            /*no indicator*/
            break;
        case LV_CPICKER_IND_LINE:
        {
            lv_point_t p1, p2;
            p1.x = gradient_area.x1 + ind_pos;
            p2.x = p1.x;
            p1.y = gradient_area.y1;
            p2.y = gradient_area.y2;

            lv_draw_line(&p1, &p2, &gradient_area, ext->ind.style, opa_scale);
            break;
        }
        case LV_CPICKER_IND_CIRCLE:
        {
            lv_area_t circle_ind_area;
            circle_ind_area.x1 = gradient_area.x1 + ind_pos - gradient_h/2;
            circle_ind_area.x2 = circle_ind_area.x1 + gradient_h;
            circle_ind_area.y1 = gradient_area.y1;
            circle_ind_area.y2 = gradient_area.y2;

            lv_style_copy(&styleCopy, ext->ind.style);
            styleCopy.body.radius = LV_RADIUS_CIRCLE;

            lv_draw_rect(&circle_ind_area, &gradient_area, &styleCopy, opa_scale);
            break;
        }
        case LV_CPICKER_IND_IN:
        {
            /*draw triangle under the gradient*/
            lv_point_t triangle_points[3];

            triangle_points[0].x = ind_pos + gradient_area.x1;
            triangle_points[0].y = gradient_area.y2 - gradient_h/2;

            triangle_points[1].x = triangle_points[0].x - ext->ind.style->line.width / 3;
            triangle_points[1].y = gradient_area.y2;

            triangle_points[2].x = triangle_points[0].x + ext->ind.style->line.width / 3;
            triangle_points[2].y = gradient_area.y2;

            lv_draw_triangle(triangle_points, &gradient_area, ext->ind.style->line.color);

            triangle_points[1].y = gradient_area.y1 - 1;
            triangle_points[2].y = gradient_area.y1 - 1;
            lv_draw_triangle(triangle_points, &gradient_area, ext->ind.style->line.color);
            break;
        }
        default:
            break;
        }
    }
    /*Post draw when the children are drawn*/
    else if(mode == LV_DESIGN_DRAW_POST) {

    }

    return true;
}

/**
 * Signal function of the color_picker of wheel type
 * @param cpicker pointer to a color_picker object
 * @param sign a signal type from lv_signal_t enum
 * @param param pointer to a signal specific variable
 * @return LV_RES_OK: the object is not deleted in the function; LV_RES_INV: the object is deleted
 */
static lv_res_t lv_cpicker_disc_signal(lv_obj_t * cpicker, lv_signal_t sign, void * param)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    lv_res_t res;

    /* Include the ancient signal function */
    res = ancestor_signal(cpicker, sign, param);
    if(res != LV_RES_OK) return res;

    lv_style_t * style = lv_cpicker_get_style(cpicker, LV_CPICKER_STYLE_MAIN);

    lv_coord_t r_out = (LV_MATH_MIN(lv_obj_get_width(cpicker), lv_obj_get_height(cpicker))) / 2;
    lv_coord_t r_in = r_out - style->line.width - style->body.padding.inner;

    lv_coord_t x = cpicker->coords.x1 + lv_obj_get_width(cpicker) / 2;
    lv_coord_t y = cpicker->coords.y1 + lv_obj_get_height(cpicker) / 2;



    if(sign == LV_SIGNAL_CLEANUP) {
        /*Nothing to cleanup. (No dynamically allocated memory in 'ext')*/
    } else if(sign == LV_SIGNAL_GET_TYPE) {
        lv_obj_type_t * buf = param;
        uint8_t i;
        for(i = 0; i < LV_MAX_ANCESTOR_NUM - 1; i++) {  /*Find the last set data*/
            if(buf->type[i] == NULL) break;
        }
        buf->type[i] = "lv_cpicker";
    }
    else if(sign == LV_SIGNAL_PRESSED)
    {
        switch(ext->wheel_mode)
        {
        case LV_CPICKER_WHEEL_HUE:
            ext->prev_hue = ext->hue;
            break;
        case LV_CPICKER_WHEEL_SAT:
            ext->prev_saturation = ext->saturation;
            break;
        case LV_CPICKER_WHEEL_VAL:
            ext->prev_value = ext->value;
            break;
        }

        lv_indev_t * indev = param;
        lv_coord_t xp = indev->proc.act_point.x - x;
        lv_coord_t yp = indev->proc.act_point.y - y;

        if((xp*xp + yp*yp) < (r_in*r_in))
        {
            if(lv_tick_elaps(ext->last_clic) < 400)
            {
                switch(ext->wheel_mode)
                {
                case LV_CPICKER_WHEEL_HUE:
                    ext->hue = 0;
                    ext->prev_hue = ext->hue;
                    break;
                case LV_CPICKER_WHEEL_SAT:
                    ext->saturation = 100;
                    ext->prev_saturation = ext->saturation;
                    break;
                case LV_CPICKER_WHEEL_VAL:
                    ext->value = 100;
                    ext->prev_value = ext->value;
                    break;
                }
                //lv_cpicker_invalidate_indicator(cpicker);
            }
            ext->last_clic = lv_tick_get();
        }
    }
    else if(sign == LV_SIGNAL_PRESSING)
    {
        lv_indev_t * indev = param;
        lv_coord_t xp = indev->proc.act_point.x - x;
        lv_coord_t yp = indev->proc.act_point.y - y;

        if((xp*xp + yp*yp) < (r_out*r_out) && (xp*xp + yp*yp) >= (r_in*r_in))
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = lv_atan2(xp, yp);
                ext->prev_hue = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_saturation = ext->saturation;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_value = ext->value;
                break;
            }

            lv_cpicker_invalidate_indicator(cpicker);

        }
    }
    else if(sign == LV_SIGNAL_PRESS_LOST)
    {
        switch(ext->wheel_mode)
        {
        case LV_CPICKER_WHEEL_HUE:
            ext->prev_hue = ext->hue;
            break;
        case LV_CPICKER_WHEEL_SAT:
            ext->prev_saturation = ext->saturation;
            break;
        case LV_CPICKER_WHEEL_VAL:
            ext->prev_value = ext->value;
            break;
        }
        lv_cpicker_invalidate_indicator(cpicker);
    }
    else if(sign == LV_SIGNAL_RELEASED)
    {
        lv_indev_t * indev = param;
        lv_coord_t xp = indev->proc.act_point.x - x;
        lv_coord_t yp = indev->proc.act_point.y - y;

        if((xp*xp + yp*yp) < (r_out*r_out) && (xp*xp + yp*yp) >= (r_in*r_in))
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = lv_atan2(xp, yp);
                ext->prev_hue = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_saturation = ext->saturation;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_value = ext->value;
                break;
            }

            lv_cpicker_invalidate_indicator(cpicker);

            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
    }
    else if(sign == LV_SIGNAL_LONG_PRESS)
    {
        if(!ext->wheel_fixed)
        {
            lv_indev_t * indev = param;
            lv_coord_t xp = indev->proc.act_point.x - x;
            lv_coord_t yp = indev->proc.act_point.y - y;

            if((xp*xp + yp*yp) < (r_in*r_in))
            {
                switch(ext->wheel_mode)
                {
                case LV_CPICKER_WHEEL_HUE:
                    ext->prev_hue = ext->hue;
                    break;
                case LV_CPICKER_WHEEL_SAT:
                    ext->prev_saturation = ext->saturation;
                    break;
                case LV_CPICKER_WHEEL_VAL:
                    ext->prev_value = ext->value;
                    break;
                }

                ext->wheel_mode = (ext->wheel_mode + 1) % 3;

                lv_obj_invalidate(cpicker);
            }
        }
    }
    else if(sign == LV_SIGNAL_CONTROLL)
    {
        uint32_t c = *((uint32_t *)param);      /*uint32_t because can be UTF-8*/
        if(c == LV_GROUP_KEY_RIGHT)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = (ext->hue + 1) % 360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = (ext->saturation + 1) % 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = (ext->value + 1) % 100;
                break;
            }

            lv_cpicker_invalidate_indicator(cpicker);

            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
        else if(c == LV_GROUP_KEY_LEFT)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = ext->hue > 0?(ext->hue - 1):360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = ext->saturation > 0?(ext->saturation - 1):100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = ext->value > 0?(ext->value - 1):100;
                break;
            }

            lv_cpicker_invalidate_indicator(cpicker);

            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
        else if(c == LV_GROUP_KEY_UP)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = (ext->hue + 1) % 360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = (ext->saturation + 1) % 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = (ext->value + 1) % 100;
                break;
            }

            lv_cpicker_invalidate_indicator(cpicker);

            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
        else if(c == LV_GROUP_KEY_DOWN)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = ext->hue > 0?(ext->hue - 1):360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = ext->saturation > 0?(ext->saturation - 1):100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = ext->value > 0?(ext->value - 1):100;
                break;
            }

            lv_cpicker_invalidate_indicator(cpicker);

            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
    }

    return LV_RES_OK;
}

/**
 * Signal function of the color_picker of rectangle type
 * @param cpicker pointer to a color_picker object
 * @param sign a signal type from lv_signal_t enum
 * @param param pointer to a signal specific variable
 * @return LV_RES_OK: the object is not deleted in the function; LV_RES_INV: the object is deleted
 */
static lv_res_t lv_cpicker_rect_signal(lv_obj_t * cpicker, lv_signal_t sign, void * param)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);

    lv_res_t res;

    /* Include the ancient signal function */
    res = ancestor_signal(cpicker, sign, param);
    if(res != LV_RES_OK) return res;

    lv_style_t * style = lv_cpicker_get_style(cpicker, LV_CPICKER_STYLE_MAIN);

    lv_coord_t r_out = (LV_MATH_MIN(lv_obj_get_width(cpicker), lv_obj_get_height(cpicker))) / 2;
    lv_coord_t r_in = r_out - style->line.width - style->body.padding.inner;

    lv_coord_t x = cpicker->coords.x1 + lv_obj_get_width(cpicker) / 2;
    lv_coord_t y = cpicker->coords.y1 + lv_obj_get_height(cpicker) / 2;



    if(sign == LV_SIGNAL_CLEANUP) {
        /*Nothing to cleanup. (No dynamically allocated memory in 'ext')*/
    } else if(sign == LV_SIGNAL_GET_TYPE) {
        lv_obj_type_t * buf = param;
        uint8_t i;
        for(i = 0; i < LV_MAX_ANCESTOR_NUM - 1; i++) {  /*Find the last set data*/
            if(buf->type[i] == NULL) break;
        }
        buf->type[i] = "lv_cpicker";
    }
    else if(sign == LV_SIGNAL_PRESSED)
    {
        switch(ext->wheel_mode)
        {
        case LV_CPICKER_WHEEL_HUE:
            ext->prev_hue = ext->hue;
            break;
        case LV_CPICKER_WHEEL_SAT:
            ext->prev_saturation = ext->saturation;
            break;
        case LV_CPICKER_WHEEL_VAL:
            ext->prev_value = ext->value;
            break;
        }

        lv_indev_t * indev = param;
        lv_coord_t xp = indev->proc.act_point.x - x;
        lv_coord_t yp = indev->proc.act_point.y - y;

        lv_area_t colorIndArea;
        //todo : set the area to the color indicator area
        if(lv_area_is_point_on(&colorIndArea, &indev->proc.act_point))
        {
            if(lv_tick_elaps(ext->last_clic) < 400)
            {
                switch(ext->wheel_mode)
                {
                case LV_CPICKER_WHEEL_HUE:
                    ext->hue = 0;
                    ext->prev_hue = ext->hue;
                    break;
                case LV_CPICKER_WHEEL_SAT:
                    ext->saturation = 100;
                    ext->prev_saturation = ext->saturation;
                    break;
                case LV_CPICKER_WHEEL_VAL:
                    ext->value = 100;
                    ext->prev_value = ext->value;
                    break;
                }
                lv_obj_invalidate(cpicker);
            }
            ext->last_clic = lv_tick_get();
        }
    }
    else if(sign == LV_SIGNAL_PRESSING)
    {
        lv_indev_t * indev = param;
        lv_coord_t xp = indev->proc.act_point.x - x;
        lv_coord_t yp = indev->proc.act_point.y - y;

        lv_area_t colorGradientArea;
        //todo : set the area to the color gradient area
        if(lv_area_is_point_on(&colorGradientArea, &indev->proc.act_point))
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = lv_atan2(xp, yp);
                ext->prev_hue = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_saturation = ext->saturation;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_value = ext->value;
                break;
            }
            lv_obj_invalidate(cpicker);
        }
    }
    else if(sign == LV_SIGNAL_PRESS_LOST)
    {
        switch(ext->wheel_mode)
        {
        case LV_CPICKER_WHEEL_HUE:
            ext->prev_hue = ext->hue;
            break;
        case LV_CPICKER_WHEEL_SAT:
            ext->prev_saturation = ext->saturation;
            break;
        case LV_CPICKER_WHEEL_VAL:
            ext->prev_value = ext->value;
            break;
        }
        lv_obj_invalidate(cpicker);
    }
    else if(sign == LV_SIGNAL_RELEASED)
    {
        lv_indev_t * indev = param;
        lv_coord_t xp = indev->proc.act_point.x - x;
        lv_coord_t yp = indev->proc.act_point.y - y;

        lv_area_t colorGradientArea;
        //todo : set th area to the color gradient area
        if(lv_area_is_point_on(&colorGradientArea, &indev->proc.act_point))
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = lv_atan2(xp, yp);
                ext->prev_hue = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_saturation = ext->saturation;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = lv_atan2(xp, yp) * 100.0 / 360.0;
                ext->prev_value = ext->value;
                break;
            }

            lv_obj_invalidate(cpicker);

            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
    }
    else if(sign == LV_SIGNAL_LONG_PRESS)
    {
        if(!ext->wheel_fixed)
        {
            lv_indev_t * indev = param;
            lv_coord_t xp = indev->proc.act_point.x - x;
            lv_coord_t yp = indev->proc.act_point.y - y;

            lv_area_t colorIndArea;
            //todo : set the area to the color indicator area
            if(lv_area_is_point_on(&colorIndArea, &indev->proc.act_point))
            {
                switch(ext->wheel_mode)
                {
                case LV_CPICKER_WHEEL_HUE:
                    ext->prev_hue = ext->hue;
                    break;
                case LV_CPICKER_WHEEL_SAT:
                    ext->prev_saturation = ext->saturation;
                    break;
                case LV_CPICKER_WHEEL_VAL:
                    ext->prev_value = ext->value;
                    break;
                }

                ext->wheel_mode = (ext->wheel_mode + 1) % 3;
                lv_obj_invalidate(cpicker);
            }
        }
    }
    else if(sign == LV_SIGNAL_CONTROLL)
    {
        uint32_t c = *((uint32_t *)param);      /*uint32_t because can be UTF-8*/
        if(c == LV_GROUP_KEY_RIGHT)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = (ext->hue + 1) % 360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = (ext->saturation + 1) % 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = (ext->value + 1) % 100;
                break;
            }
            lv_obj_invalidate(cpicker);
            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
        else if(c == LV_GROUP_KEY_LEFT)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = ext->hue > 0?(ext->hue - 1):360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = ext->saturation > 0?(ext->saturation - 1):100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = ext->value > 0?(ext->value - 1):100;
                break;
            }
            lv_obj_invalidate(cpicker);
            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
        else if(c == LV_GROUP_KEY_UP)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = (ext->hue + 1) % 360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = (ext->saturation + 1) % 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = (ext->value + 1) % 100;
                break;
            }
            lv_obj_invalidate(cpicker);
            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
        else if(c == LV_GROUP_KEY_DOWN)
        {
            switch(ext->wheel_mode)
            {
            case LV_CPICKER_WHEEL_HUE:
                ext->hue = ext->hue > 0?(ext->hue - 1):360;
                break;
            case LV_CPICKER_WHEEL_SAT:
                ext->saturation = ext->saturation > 0?(ext->saturation - 1):100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                ext->value = ext->value > 0?(ext->value - 1):100;
                break;
            }
            lv_obj_invalidate(cpicker);
            if(ext->value_changed != NULL)
                ext->value_changed(cpicker);
        }
    }

    return LV_RES_OK;
}

static void lv_cpicker_invalidate_indicator(lv_obj_t * cpicker)
{
    lv_cpicker_ext_t * ext = lv_obj_get_ext_attr(cpicker);
    lv_style_t * style = lv_cpicker_get_style(cpicker, LV_CPICKER_STYLE_MAIN);

    static lv_style_t styleCopy;
    lv_style_copy(&styleCopy, style);

    lv_coord_t r = (LV_MATH_MIN(lv_obj_get_width(cpicker), lv_obj_get_height(cpicker))) / 2;
    lv_coord_t x = cpicker->coords.x1 + lv_obj_get_width(cpicker) / 2;
    lv_coord_t y = cpicker->coords.y1 + lv_obj_get_height(cpicker) / 2;

    if(ext->type == LV_CPICKER_TYPE_DISC)
    {
        /*invalidate center*/
        lv_area_t center_col_area;

        uint32_t rin = r - styleCopy.line.width;

        uint16_t radius = lv_sqrt((4*rin*rin)/2)/2 + 1 - style->body.padding.inner;

        center_col_area.x1 = x - radius;
        center_col_area.y1 = y - radius;
        center_col_area.x2 = x + radius;
        center_col_area.y2 = y + radius;

        lv_inv_area(&center_col_area);

        switch(ext->ind.type)
        {
        case LV_CPICKER_IND_LINE:
        {
            lv_area_t line_area;
            lv_point_t point1, point2;
            lv_coord_t x1, y1, x2, y2;
            uint16_t angle;

            switch(ext->wheel_mode)
            {
            default:
            case LV_CPICKER_WHEEL_HUE:
                angle = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                angle = ext->saturation * 360 / 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                angle = ext->value * 360 / 100;
                break;
            }

            x1 = x + ((r - style->line.width + ext->ind.style->body.padding.inner + ext->ind.style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            y1 = y + ((r - style->line.width + ext->ind.style->body.padding.inner + ext->ind.style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);
            x2 = x + ((r - ext->ind.style->body.padding.inner - ext->ind.style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            y2 = y + ((r - ext->ind.style->body.padding.inner - ext->ind.style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            point1.x = x1;
            point1.y = y1;
            point2.x = x2;
            point2.y = y2;

            //if(LV_MATH_ABS(point1.x - point2.x) > LV_MATH_ABS(point1.y - point2.y))
            //{
            /*Steps less in y then x -> rather horizontal*/
            if(point1.x < point2.x) {
                line_area.x1 = point1.x;
                //line_area.y1 = point1.y;
                line_area.x2 = point2.x;
                //line_area.y2 = point2.y;
            } else {
                line_area.x1 = point2.x;
                //line_area.y1 = point2.y;
                line_area.x2 = point1.x;
                //line_area.y2 = point1.y;
            }
            //} else {
            /*Steps less in x then y -> rather vertical*/
            if(point1.y < point2.y) {
                //line_area.x1 = point1.x;
                line_area.y1 = point1.y;
                //line_area.x2 = point2.x;
                line_area.y2 = point2.y;
            } else {
                //line_area.x1 = point2.x;
                line_area.y1 = point2.y;
                line_area.x2 = point1.x;
                //line_area.y2 = point1.y;
            }
            //}

            line_area.x1 -= 2*ext->ind.style->line.width;
            line_area.y1 -= 2*ext->ind.style->line.width;
            line_area.x2 += 2*ext->ind.style->line.width;
            line_area.y2 += 2*ext->ind.style->line.width;

            lv_inv_area(&line_area);


            angle = ext->prev_pos;

            x1 = x + ((r - style->line.width + ext->ind.style->body.padding.inner + ext->ind.style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            y1 = y + ((r - style->line.width + ext->ind.style->body.padding.inner + ext->ind.style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            x2 = x + ((r - ext->ind.style->body.padding.inner - ext->ind.style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            y2 = y + ((r - ext->ind.style->body.padding.inner - ext->ind.style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            point1.x = x1;
            point1.y = y1;
            point2.x = x2;
            point2.y = y2;

            //if(LV_MATH_ABS(point1.x - point2.x) > LV_MATH_ABS(point1.y - point2.y))
            //{
            /*rather horizontal*/
            if(point1.x < point2.x) {
                line_area.x1 = point1.x;
                //line_area.y1 = point1.y;
                line_area.x2 = point2.x;
                //line_area.y2 = point2.y;
            } else {
                line_area.x1 = point2.x;
                //line_area.y1 = point2.y;
                line_area.x2 = point1.x;
                //line_area.y2 = point1.y;
            }
            //} else {
            /*rather vertical*/
            if(point1.y < point2.y) {
                //line_area.x1 = point1.x;
                line_area.y1 = point1.y;
                //line_area.x2 = point2.x;
                line_area.y2 = point2.y;
            } else {
                //line_area.x1 = point2.x;
                line_area.y1 = point2.y;
                //line_area.x2 = point1.x;
                line_area.y2 = point1.y;
            }
            //}

            line_area.x1 -= 2*ext->ind.style->line.width;
            line_area.y1 -= 2*ext->ind.style->line.width;
            line_area.x2 += 2*ext->ind.style->line.width;
            line_area.y2 += 2*ext->ind.style->line.width;

            lv_inv_area(&line_area);

            break;
        }
        case LV_CPICKER_IND_CIRCLE:
        {
            lv_area_t circle_ind_area;
            uint32_t cx, cy;

            uint16_t angle;

            switch(ext->wheel_mode)
            {
            default:
            case LV_CPICKER_WHEEL_HUE:
                angle = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                angle = ext->saturation * 360 / 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                angle = ext->value * 360 / 100;
                break;
            }

            cx = x + ((r - style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            cy = y + ((r - style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            circle_ind_area.x1 = cx - style->line.width/2;
            circle_ind_area.y1 = cy - style->line.width/2;
            circle_ind_area.x2 = cx + style->line.width/2;
            circle_ind_area.y2 = cy + style->line.width/2;

            lv_inv_area(&circle_ind_area);


            /* invalidate last position*/
            angle = ext->prev_pos;

            cx = x + ((r - style->line.width/2) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            cy = y + ((r - style->line.width/2) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            circle_ind_area.x1 = cx - style->line.width/2;
            circle_ind_area.y1 = cy - style->line.width/2;
            circle_ind_area.x2 = cx + style->line.width/2;
            circle_ind_area.y2 = cy + style->line.width/2;

            lv_inv_area(&circle_ind_area);
            break;
        }
        case LV_CPICKER_IND_IN:
        {
            lv_area_t circle_ind_area;
            uint32_t cx, cy;

            uint16_t angle;

            switch(ext->wheel_mode)
            {
            default:
            case LV_CPICKER_WHEEL_HUE:
                angle = ext->hue;
                break;
            case LV_CPICKER_WHEEL_SAT:
                angle = ext->saturation * 360 / 100;
                break;
            case LV_CPICKER_WHEEL_VAL:
                angle = ext->value * 360 / 100;
                break;
            }

            uint16_t ind_radius = lv_sqrt((4*rin*rin)/2)/2 + 1 - style->body.padding.inner;
            ind_radius = (ind_radius + rin) / 2;

            cx = x + ((ind_radius) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            cy = y + ((ind_radius) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            circle_ind_area.x1 = cx - ext->ind.style->line.width/2;
            circle_ind_area.y1 = cy - ext->ind.style->line.width/2;
            circle_ind_area.x2 = cx + ext->ind.style->line.width/2;
            circle_ind_area.y2 = cy + ext->ind.style->line.width/2;

            lv_inv_area(&circle_ind_area);


            /* invalidate last position*/
            angle = ext->prev_pos;

            cx = x + ((ind_radius) * lv_trigo_sin(angle) >> LV_TRIGO_SHIFT);
            cy = y + ((ind_radius) * lv_trigo_sin(angle + 90) >> LV_TRIGO_SHIFT);

            circle_ind_area.x1 = cx - ext->ind.style->line.width/2;
            circle_ind_area.y1 = cy - ext->ind.style->line.width/2;
            circle_ind_area.x2 = cx + ext->ind.style->line.width/2;
            circle_ind_area.y2 = cy + ext->ind.style->line.width/2;

            lv_inv_area(&circle_ind_area);
            break;
        }
        }
    }
    else if(ext->type == LV_CPICKER_TYPE_RECT)
    {

    }
}


#endif
