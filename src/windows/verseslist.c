#include <pebble.h>
#include "verseslist.h"
#include "viewer.h"
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "windows/chapterlist.h"
#include "../appmessage.h"

#define MAX_RANGES 6
#define MAX_RANGE_SIZE 8

static char ranges[MAX_RANGES][MAX_RANGE_SIZE];

static Book *current_book;
static int current_chapter;
static int num_ranges;
static int request_token;

static void refresh_list();
static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void window_load(Window *window);
static void window_unload(Window *window);

static Window *window;
static MenuLayer *menu_layer;

void verseslist_init(Book *book, int chapter) {
	window = window_create();
    current_book = book;
    current_chapter = chapter;

    window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
        .unload = window_unload,
	});

	menu_layer = menu_layer_create_fullscreen(window);
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = menu_get_num_sections_callback,
		.get_num_rows = menu_get_num_rows_callback,
		.get_header_height = menu_get_header_height_callback,
		.get_cell_height = menu_get_cell_height_callback,
		.draw_header = menu_draw_header_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,
		.select_long_click = menu_select_long_callback,
	});
	menu_layer_set_click_config_onto_window(menu_layer, window);
	menu_layer_add_to_window(menu_layer, window);

	window_stack_push(window, true);
}

void verseslist_destroy(void) {
	viewer_destroy();
	layer_remove_from_parent(menu_layer_get_layer(menu_layer));
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void verseslist_in_received_handler(DictionaryIterator *iter) {
    Tuple *content_tuple = dict_find(iter, KEY_CONTENT);
    Tuple *index_tuple = dict_find(iter, KEY_INDEX);
    Tuple *token_tuple = dict_find(iter, KEY_TOKEN);

	if (content_tuple && index_tuple && token_tuple) {
        if (token_tuple->value->int32 != request_token) return;

		strncpy(ranges[index_tuple->value->int16], content_tuple->value->cstring, MAX_RANGE_SIZE - 1);
		num_ranges++;
		menu_layer_reload_data(menu_layer);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received verse range %s", ranges[index_tuple->value->int16]);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void refresh_list() {
	memset(ranges, 0x0, sizeof(ranges));
	num_ranges = 0;
	menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data(menu_layer);
	request_token = appmessage_verseslist_request_data(current_book->name, current_chapter);
	menu_layer_reload_data(menu_layer);
}

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return (num_ranges) ? num_ranges : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return 34;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	menu_cell_basic_header_draw(ctx, cell_layer, "Verse Ranges");
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	if (num_ranges == 0) {
		menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
	} else {
	    if (menu_cell_layer_is_highlighted(cell_layer)) {
            graphics_context_set_text_color(ctx, GColorWhite);
        } else {
            graphics_context_set_text_color(ctx, GColorBlack);
	    }
		graphics_draw_text(ctx, ranges[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_24), (GRect) { .origin = { 8, 0 }, .size = { PEBBLE_WIDTH - 8, 28 } }, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (num_ranges == 0) {
		return;
	}
    viewer_init(current_book, current_chapter, ranges[cell_index->row]);
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	refresh_list();
}

static void window_load(Window *window) {
    refresh_list();
}

static void window_unload(Window *window) {
    appmessage_cancel_request(request_token);
}
