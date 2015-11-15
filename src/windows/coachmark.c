#include <pebble.h>
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "coachmark.h"

#define PADDING             5
#define SCROLL_UP_JUMP      100
#define SCROLL_DOWN_JUMP    -(SCROLL_UP_JUMP)

static void set_current_text(char *text);
static void window_load(Window *window);
static void window_unload(Window *window);
static void click_config_provider(Window *window);

static Window *window;
static ScrollLayer *scroll_layer;
static TextLayer *text_layer;
static char *current_text;

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

    scroll_layer_set_click_config_onto_window(scroll_layer, window);
    scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
        .click_config_provider = (ClickConfigProvider) click_config_provider
    });

    text_layer = text_layer_create(GRect(PADDING, PADDING, bounds.size.w - PADDING*2, bounds.size.h - PADDING*2));
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_layer));
	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));

#if PBL_ROUND    
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_enable_screen_text_flow_and_paging(text_layer, 5);
#endif

	window_stack_push(window, true);
}

void coachmark_destroy(void) {
	layer_remove_from_parent(scroll_layer_get_layer(scroll_layer));
	scroll_layer_destroy(scroll_layer);
    
	window_destroy_safe(window);
}

static void set_current_text(char *text) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);
    text_layer_set_size(text_layer, GSize(bounds.size.w - PADDING*2, 9999));
    
    current_text = text;
    text_layer_set_text(text_layer, current_text);
    GSize max_size = text_layer_get_content_size(text_layer);
    text_layer_set_size(text_layer, max_size);
    scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, max_size.h + PADDING*2));
}

static void scroll_text_by(int16_t amount, ScrollLayer *layer) {
    GPoint point = scroll_layer_get_content_offset(layer);
    point.y += amount;
    layer_mark_dirty(scroll_layer_get_layer(layer));
    scroll_layer_set_content_offset(layer, point, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //
static void select_single_down_click_handler(ClickRecognizerRef recognizer, void *context) {
    scroll_text_by(SCROLL_DOWN_JUMP, (ScrollLayer *)context);
}

static void select_single_up_click_handler(ClickRecognizerRef recognizer, void *context) {
    scroll_text_by(SCROLL_UP_JUMP, (ScrollLayer *)context);
}

static void click_config_provider(Window *window) {
    window_single_click_subscribe(BUTTON_ID_DOWN, select_single_down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, select_single_up_click_handler);
}

static void window_load(Window *window) {
    set_current_text(COACHMARK_TEXT);
}

static void window_unload(Window *window) {
}
