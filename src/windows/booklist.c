#include <pebble.h>
#include "booklist.h"
#include "testamentlist.h"
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "chapterlist.h"
#include "../appmessage.h"

#define MAX_BOOKS 39

static Book books[MAX_BOOKS];

static TestamentType current_testament;
static int num_books;
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

void booklist_init(TestamentType testament) {
	window = window_create();
  current_testament = testament;

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

void booklist_destroy(void) {
	chapterlist_destroy();
	layer_remove_from_parent(menu_layer_get_layer(menu_layer));
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void booklist_in_received_handler(DictionaryIterator *iter) {
  Tuple *index_tuple = dict_find(iter, KEY_INDEX);
	Tuple *book_tuple = dict_find(iter, KEY_BOOK);
	Tuple *chapter_tuple = dict_find(iter, KEY_CHAPTER);
  Tuple *token_tuple = dict_find(iter, KEY_TOKEN);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %s", book_tuple->value->cstring);

	if (book_tuple && chapter_tuple && token_tuple) {
    if (token_tuple->value->int32 != request_token) return;

		Book book;
    book.index = index_tuple->value->int16;
		strncpy(book.name, book_tuple->value->cstring, sizeof(book.name));
		book.chapters = chapter_tuple->value->int16;
		books[book.index] = book;
		num_books++;
		menu_layer_reload_data(menu_layer);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received book [%d] %s", book.index, book.name);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void refresh_list() {
	memset(books, 0x0, sizeof(books));
	num_books = 0;
	menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data(menu_layer);
	request_token = appmessage_booklist_request_data(current_testament);
	menu_layer_reload_data(menu_layer);
}

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return (num_books) ? num_books : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return 34;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
#if PBL_ROUND
	graphics_draw_text(ctx, 
		testament_to_string(current_testament), 
		fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
		(GRect) { .origin = { 0, 0 }, .size = { PEBBLE_WIDTH, 16 } }, 
		GTextOverflowModeTrailingEllipsis, 
		PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), 
		NULL);
#else
	menu_cell_basic_header_draw(ctx, cell_layer, testament_to_string(current_testament));
#endif
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	if (num_books == 0) {
		menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
	} else {
	    if (menu_cell_layer_is_highlighted(cell_layer)) {
            graphics_context_set_text_color(ctx, GColorWhite);
        } else {
            graphics_context_set_text_color(ctx, GColorBlack);
	    }
		graphics_draw_text(ctx, 
			books[cell_index->row].name, 
			fonts_get_system_font(FONT_KEY_GOTHIC_24), 
			(GRect) { .origin = { PBL_IF_ROUND_ELSE(0, 8), 0 }, .size = { PEBBLE_WIDTH - PBL_IF_ROUND_ELSE(0, 8), 28 } }, 
			GTextOverflowModeTrailingEllipsis, 
			PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), 
			NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (num_books == 0) {
		return;
	}
  chapterlist_init(&books[cell_index->row]);
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	refresh_list();
}

static void window_load(Window *window) {
  refresh_list();
}

static void window_unload(Window *window) {
    APP_LOG(APP_LOG_LEVEL_INFO, "booklist.window_unload");
  appmessage_cancel_request(request_token);
}
