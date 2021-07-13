CSRCS += lv_anim.c
CSRCS += lv_anim_timeline.c
CSRCS += lv_area.c
CSRCS += lv_async.c
CSRCS += lv_bidi.c
CSRCS += lv_color.c
CSRCS += lv_fs.c
CSRCS += lv_gc.c
CSRCS += lv_ll.c
CSRCS += lv_log.c
CSRCS += lv_math.c
CSRCS += lv_mem.c
CSRCS += lv_printf.c
CSRCS += lv_style.c
CSRCS += lv_style_gen.c
CSRCS += lv_timer.c
CSRCS += lv_tlsf.c
CSRCS += lv_txt.c
CSRCS += lv_txt_ap.c
CSRCS += lv_utils.c

DEPPATH += --dep-path $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/misc
VPATH += :$(LVGL_DIR)/$(LVGL_DIR_NAME)/src/misc

CFLAGS += "-I$(LVGL_DIR)/$(LVGL_DIR_NAME)/src/misc"
