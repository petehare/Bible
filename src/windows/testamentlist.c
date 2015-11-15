#include <pebble.h>
#include "testamentlist.h"
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "windows/booklist.h"
#include "windows/favoriteslist.h"
#include "windows/coachmark.h"

const char* testament_to_string(TestamentType testament);
static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *window;
static MenuLayer *menu_layer;

void testamentlist_init(void) {
    window = window_create();
    
    menu_layer = menu_layer_create_fullscreen(window);
    menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .get_cell_height = menu_get_cell_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
    });
    menu_layer_set_click_config_onto_window(menu_layer, window);
    menu_layer_add_to_window(menu_layer, window);
    
    window_stack_push(window, true);

    coachmark_init();
}

void testamentlist_destroy(void) {
    booklist_destroy();
    layer_remove_from_parent(menu_layer_get_layer(menu_layer));
    menu_layer_destroy_safe(menu_layer);
    window_destroy_safe(window);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

const char* testament_to_string(TestamentType testament) {
    switch (testament) {
        case TestamentTypeOld:
            return "Old Testament";
        case TestamentTypeNew:
            return "New Testament";
        default:
            return "";
    }
}

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
    return 2;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
    if (section_index == 0) return 2;
    return 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 39;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	const char *header = section_index == 0 ? "Testaments" : "More";
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
    const char *title;
    if (cell_index->section == 0) {
        title = testament_to_string((TestamentType)cell_index->row);
    } else {
        title = "Favorites";
    }
    if (menu_cell_layer_is_highlighted(cell_layer)) {
        graphics_context_set_text_color(ctx, GColorWhite);
    } else {
        graphics_context_set_text_color(ctx, GColorBlack);
    }
    graphics_draw_text(ctx, 
    	title, 
    	fonts_get_system_font(FONT_KEY_GOTHIC_24), 
    	(GRect) { .origin = { PBL_IF_ROUND_ELSE(0, 8), 2. }, .size = { PEBBLE_WIDTH - PBL_IF_ROUND_ELSE(0, 8), 30 }  }, 
    	GTextOverflowModeTrailingEllipsis, 
    	PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), 
    	NULL);
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    if (cell_index->section == 0) {
        booklist_init((TestamentType)cell_index->row);
    } else {
        favoriteslist_init();
    }
}
