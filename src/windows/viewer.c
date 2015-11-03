#include <pebble.h>
#include "booklist.h"
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "../appmessage.h"

#define LOADING_TEXT        "Loading..."
#define PADDING             5
#define SCROLL_UP_JUMP      110
#define SCROLL_DOWN_JUMP    -(SCROLL_UP_JUMP)

static Book current_book;
static int current_chapter;
static char *current_range;
static int current_index;
static int request_token;
static char *current_text;

static void set_current_text(char *text);
static void click_config_provider(Window *window);
static void select_multi_click_handler(ClickRecognizerRef recognizer, void *context);
static void window_load(Window *window);
static void window_unload(Window *window);

static Window *window;
static ScrollLayer *scroll_layer;
static TextLayer *text_layer;

void viewer_init(Book *book, int chapter, char *range) {
	window = window_create();

    current_chapter = chapter;
    strncpy(current_book.name, book->name, sizeof(book->name));
    current_range = malloc((strlen(range) + 1));
    strcpy(current_range, range);

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
    text_layer_enable_screen_text_flow_and_paging(text_layer, 5);
#endif

	window_stack_push(window, true);
}

void viewer_destroy(void) {
	layer_remove_from_parent(scroll_layer_get_layer(scroll_layer));
	scroll_layer_destroy(scroll_layer);
    
	window_destroy_safe(window);
}

void viewer_in_received_handler(DictionaryIterator *iter) {

	Tuple *content_tuple = dict_find(iter, KEY_CONTENT);
    Tuple *index_tuple = dict_find(iter, KEY_INDEX);
    Tuple *token_tuple = dict_find(iter, KEY_TOKEN);

	if (content_tuple && index_tuple && token_tuple) {
        if (token_tuple->value->int32 != request_token) return;
        if (index_tuple->value->int16 <= current_index) return;
        current_index = index_tuple->value->int16;

        char *additional_text = content_tuple->value->cstring;
        char *new_text;
        if (strcmp(current_text, LOADING_TEXT) == 0)
        {
            new_text = malloc(strlen(additional_text) + 1);
            strcpy(new_text, additional_text);
            current_text = NULL;
        }
        else
        {
            new_text = malloc((strlen(current_text) + strlen(additional_text) + 1));
            strcpy(new_text, current_text);
            strcat(new_text, additional_text);
        }
        if (NULL != current_text && strcmp(current_text, LOADING_TEXT) != 0) {
            free(current_text);
            current_text = NULL;
        }
        set_current_text(new_text);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "received content for chapter [%d] %s", current_chapter, current_book.name);
	}
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
    window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 0, 0, true, select_multi_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, select_single_down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, select_single_up_click_handler);
}

static void select_multi_click_handler(ClickRecognizerRef recognizer, void *context) {
    request_token = appmessage_viewer_toggle_favorite(current_book.name, current_chapter, current_range);
}

static void window_load(Window *window) {
    current_text = LOADING_TEXT;
    current_index = -1;
    text_layer_set_text(text_layer, current_text);
    request_token = appmessage_viewer_request_data(current_book.name, current_chapter, current_range);
}

static void window_unload(Window *window) {
    appmessage_cancel_request(request_token);
    request_token = 0;
    if (current_text != NULL && strcmp(current_text, LOADING_TEXT) != 0) {
        free(current_text);
        current_text = NULL;
    }
    if (current_range != NULL) {
        free(current_range);
        current_range = NULL;
    }
}
