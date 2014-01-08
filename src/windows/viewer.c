#include <pebble.h>
#include "booklist.h"
#include "../libs/pebble-assist.h"
#include "../common.h"

#define LOADING_TEXT "Loading..."

static Book *current_book;
static int current_chapter;
static int current_index;
static char *current_text;

static void request_data();
static void window_load(Window *window);
static void window_unload(Window *window);

static Window *window;
static ScrollLayer *scroll_layer;
static TextLayer *text_layer;

void viewer_init(Book *book, int chapter) {
	window = window_create();

  current_book = book;
  current_chapter = chapter;

  window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
    .unload = window_unload,
	});

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
	scroll_layer = scroll_layer_create(bounds);

	scroll_layer_set_click_config_onto_window(scroll_layer, window);
  text_layer = text_layer_create(bounds);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

  scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_layer));
	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));

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
//  Tuple *book_tuple = dict_find(iter, KEY_BOOK);
//  Tuple *chapter_tuple = dict_find(iter, KEY_CHAPTER);

	if (content_tuple && index_tuple /* && book_tuple && chapter_tuple*/) {
//    if (strcmp(book_tuple->value->cstring, current_book->name) != 0) return;
//    if (chapter_tuple->value->int16 != current_chapter) return;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "received content, checking index [%d] -> [%d]", index_tuple->value->int16, current_index);
    if (index_tuple->value->int16 <= current_index) return;
    current_index = index_tuple->value->int16;

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);
    text_layer_set_size(text_layer, GSize(bounds.size.w, 5000));
    char *additional_text = content_tuple->value->cstring;
    char *new_text = malloc((strlen(current_text) + strlen(additional_text) + 1));
    if (strcmp(current_text, LOADING_TEXT))
    {
      strcpy(new_text, current_text);
    }
    strcat(new_text, additional_text);
    free(current_text);
    current_text = new_text;
    text_layer_set_text(text_layer, current_text);
    GSize max_size = text_layer_get_content_size(text_layer);
    text_layer_set_size(text_layer, max_size);
    scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, max_size.h));

		APP_LOG(APP_LOG_LEVEL_DEBUG, "received content for chapter[%d] %s", current_chapter, current_book->name);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void request_data() {
  Tuplet request_tuple = TupletCString(KEY_REQUEST, "viewer");
	Tuplet book_tuple = TupletCString(KEY_BOOK, current_book->name);
  Tuplet chapter_tuple = TupletInteger(KEY_CHAPTER, current_chapter);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) {
		return;
	}

	dict_write_tuplet(iter, &request_tuple);
	dict_write_tuplet(iter, &book_tuple);
  dict_write_tuplet(iter, &chapter_tuple);
	dict_write_end(iter);

	app_message_outbox_send();
}

static void window_load(Window *window) {
  current_text = LOADING_TEXT;
  current_index = -1;
  text_layer_set_text(text_layer, current_text);
  request_data();
}

static void window_unload(Window *window) {

}
