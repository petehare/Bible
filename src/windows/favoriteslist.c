#include <pebble.h>
#include "favoriteslist.h"
#include "viewer.h"
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "../appmessage.h"

#define MAX_FAVORITES 20

static Favorite favorites[MAX_FAVORITES];

static int num_favorites;
static int favorites_request_token;

static void refresh_list();
static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void window_appear(Window *window);
static void window_unload(Window *window);

static Window *window;
static MenuLayer *menu_layer;
static bool favorites_is_dirty = true;

void favoriteslist_init() {
    window = window_create();
    
    window_set_window_handlers(window, (WindowHandlers) {
        .appear = window_appear,
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

void favoriteslist_destroy(void) {
    viewer_destroy();
    layer_remove_from_parent(menu_layer_get_layer(menu_layer));
    menu_layer_destroy_safe(menu_layer);
    window_destroy_safe(window);
}

void favoriteslist_mark_dirty(void) {
    favorites_is_dirty = true;
}

void favoriteslist_in_received_handler(DictionaryIterator *iter) {
    Tuple *token_tuple = dict_find(iter, KEY_TOKEN);
    Tuple *index_tuple = dict_find(iter, KEY_INDEX);
    Tuple *book_tuple = dict_find(iter, KEY_BOOK);
    Tuple *chapter_tuple = dict_find(iter, KEY_CHAPTER);
    Tuple *range_tuple = dict_find(iter, KEY_RANGE);
    
    if (!token_tuple || token_tuple->value->int32 != favorites_request_token) {
        return;
    }

    favorites_is_dirty = false;
    if (index_tuple && token_tuple && book_tuple && chapter_tuple && range_tuple) {
        if (index_tuple->value->int16 > MAX_FAVORITES) return;
        
        Favorite favorite;
        favorite.chapter = chapter_tuple->value->int16;
        strncpy(favorite.range, range_tuple->value->cstring, sizeof(favorite.range));
        
        Book book;
        strncpy(book.name, book_tuple->value->cstring, sizeof(book.name));
        favorite.book = book;
        
        favorites[index_tuple->value->int16] = favorite;
        num_favorites++;
    }
    menu_layer_reload_data(menu_layer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void refresh_list() {
    if (!favorites_is_dirty) {
        return;
    }
    memset(favorites, 0x0, sizeof(favorites));
    num_favorites = 0;
    menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
    menu_layer_reload_data(menu_layer);
    favorites_request_token = appmessage_favoriteslist_request_data();
    menu_layer_reload_data(menu_layer);
}

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
    return 1;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
    return (num_favorites) ? num_favorites : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    if (num_favorites == 0 && !favorites_is_dirty) return 86;
    return 34;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	const char *header = "Favorites";
#if PBL_ROUND
	graphics_draw_text(ctx, 
		header, 
		fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
		(GRect) { .origin = { 0, 0 }, .size = { PEBBLE_WIDTH, 16 } }, 
		GTextOverflowModeTrailingEllipsis, 
		PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), 
		NULL);
#else
	menu_cell_basic_header_draw(ctx, cell_layer, header);
#endif
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    if (menu_cell_layer_is_highlighted(cell_layer)) {
        graphics_context_set_text_color(ctx, GColorWhite);
    } else {
        graphics_context_set_text_color(ctx, GColorBlack);
    }

	const char *row_text = NULL;
	const char *font = FONT_KEY_GOTHIC_24;
	int height = 28;
	int margin = PBL_IF_ROUND_ELSE(0, 8);
    if (favorites_is_dirty) {
    	row_text = "Loading...";
    } else if (num_favorites == 0) {
		row_text = "Double tap the “Select” button while reading to add or remove favorites";
		font = FONT_KEY_GOTHIC_18;
		height = 80;
		margin = 8;
    } else {
        Favorite favorite = favorites[cell_index->row];
        static char title[40];
        snprintf(title, sizeof(title), "%s %d:%s", favorite.book.name, favorite.chapter, favorite.range);
        row_text = title;
    }
    
    if (row_text != NULL) {
		graphics_draw_text(ctx, 
			row_text, 
			fonts_get_system_font(font), 
			(GRect) { .origin = { margin, 0 }, .size = { PEBBLE_WIDTH - (margin * PBL_IF_ROUND_ELSE(2, 1)), height } }, 
			GTextOverflowModeTrailingEllipsis, 
			PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), 
			NULL);
    }
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    if (num_favorites == 0) {
        return;
    }
    Favorite favorite = favorites[cell_index->row];
    viewer_init(&favorite.book, favorite.chapter, favorite.range);
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    refresh_list();
}

static void window_appear(Window *window) {
    refresh_list();
}

static void window_unload(Window *window) {
    appmessage_cancel_request(favorites_request_token);
}
