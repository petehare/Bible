#include <pebble.h>
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "coachmark.h"

#define PADDING             5

static void window_load(Window *window);
static void window_unload(Window *window);

static Window *window;
static ScrollLayer *scroll_layer;
static TextLayer *text_layer;
static char *coach_text;

void coachmark_init(void) {
    int coachmark_version = persist_read_int(COACHMARK_VERSION_KEY);
    if (coachmark_version != COACHMARK_VERSION) {
        // versions don't match, lets write it to storage
        persist_write_int(COACHMARK_VERSION_KEY, COACHMARK_VERSION);
        // read it back, verify that it was stored properly
        coachmark_version = persist_read_int(COACHMARK_VERSION_KEY);
        if (coachmark_version != COACHMARK_VERSION) {
            // something went wrong, don't show coachmark
            return;
        }
    }
    else {
        // don't show coachmark
        return;
    }


	window = window_create();

    window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
        .unload = window_unload,
	});

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);
	scroll_layer = scroll_layer_create(bounds);

    text_layer = text_layer_create(GRect(PADDING, PADDING, bounds.size.w - PADDING*2, bounds.size.h - PADDING*2));
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_layer));
	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));

#if PBL_ROUND    
    text_layer_enable_screen_text_flow_and_paging(text_layer, 5);
#endif

	window_stack_push(window, true);
}

void coachmark_destroy(void) {
	layer_remove_from_parent(scroll_layer_get_layer(scroll_layer));
	scroll_layer_destroy(scroll_layer);
    
	window_destroy_safe(window);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
static void window_load(Window *window) {
    coach_text = COACHMARK_TEXT;
    text_layer_set_text(text_layer, coach_text);
}

static void window_unload(Window *window) {
}
