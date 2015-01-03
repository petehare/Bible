#include <pebble.h>
#include "favoriteslist.h"
#include "viewer.h"
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "../appmessage.h"

#define MAX_FAVORITES 20

static Favorite favorites[MAX_FAVORITES];

static int num_favorites;
static int request_token;

static void refresh_list();
static void request_data();
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

void favoriteslist_init() {
    window = window_create();
    
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

void favoriteslist_destroy(void) {
    viewer_destroy();
    layer_remove_from_parent(menu_layer_get_layer(menu_layer));
    menu_layer_destroy_safe(menu_layer);
    window_destroy_safe(window);
}

void favoriteslist_in_received_handler(DictionaryIterator *iter) {
    Tuple *token_tuple = dict_find(iter, KEY_TOKEN);
    Tuple *index_tuple = dict_find(iter, KEY_INDEX);
    Tuple *book_tuple = dict_find(iter, KEY_BOOK);
    Tuple *chapter_tuple = dict_find(iter, KEY_CHAPTER);
    Tuple *range_tuple = dict_find(iter, KEY_RANGE);
    
    if (index_tuple && token_tuple && book_tuple && chapter_tuple && range_tuple) {
        if (token_tuple->value->int32 != request_token) return;
        if (index_tuple->value->int16 > MAX_FAVORITES) return;
        
        Favorite favorite;
        favorite.chapter = chapter_tuple->value->int16;
        strncpy(favorite.range, range_tuple->value->cstring, sizeof(favorite.range));
        
        Book book;
        strncpy(book.name, book_tuple->value->cstring, sizeof(book.name));
        favorite.book = book;
        
        favorites[index_tuple->value->int16] = favorite;
        num_favorites++;
        menu_layer_reload_data(menu_layer);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void refresh_list() {
    memset(favorites, 0x0, sizeof(favorites));
    num_favorites = 0;
    menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
    menu_layer_reload_data(menu_layer);
    request_data();
    menu_layer_reload_data(menu_layer);
}

static void request_data() {
    Tuplet request_tuple = TupletInteger(KEY_REQUEST, RequestTypeFavorites);
    
    request_token = (int)time(NULL);
    Tuplet token_tuple = TupletInteger(KEY_TOKEN, request_token);
    
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    if (iter == NULL) {
        return;
    }
    
    dict_write_tuplet(iter, &request_tuple);
    dict_write_tuplet(iter, &token_tuple);
    dict_write_end(iter);
    
    app_message_outbox_send();
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
    return 34;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Favorites");
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Displaying favorite for a book");
    if (num_favorites == 0) {
        menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    } else {
        Favorite favorite = favorites[cell_index->row];

        static char title[40];
        snprintf(title, sizeof(title), "%s %d:%s", favorite.book.name, favorite.chapter, favorite.range);;
        
        graphics_context_set_text_color(ctx, GColorBlack);
        graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_24), (GRect) { .origin = { 8, 0 }, .size = { PEBBLE_WIDTH - 8, 28 } }, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
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

static void window_load(Window *window) {
    refresh_list();
}

static void window_unload(Window *window) {
    cancel_request_with_token(request_token);
}
