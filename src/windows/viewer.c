#include <pebble.h>
#include "booklist.h"
#include "../libs/pebble-assist.h"
#include "../common.h"

#define LOADING_TEXT "Loading..."

static Book *current_book;
static int current_chapter;
static char *current_text = LOADING_TEXT;

static void request_data();

static Window *window;
static ScrollLayer *scroll_layer;
static TextLayer *text_layer;

void viewer_init(Book *book, int chapter) {
	window = window_create();
  current_book = book;
  current_chapter = chapter;

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
	scroll_layer = scroll_layer_create(bounds);

	scroll_layer_set_click_config_onto_window(scroll_layer, window);

  text_layer = text_layer_create(bounds);
  text_layer_set_text(text_layer, current_text);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));

  scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_layer));
	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));

	window_stack_push(window, true);

  request_data();
}

void viewer_destroy(void) {
	layer_remove_from_parent(scroll_layer_get_layer(scroll_layer));
	scroll_layer_destroy(scroll_layer);
	window_destroy_safe(window);
}

void viewer_in_received_handler(DictionaryIterator *iter) {
	Tuple *content_tuple = dict_find(iter, KEY_CONTENT);

	if (content_tuple) {
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

//static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
//	if (num_books == 0) {
//		menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
//	} else {
//    menu_cell_basic_draw(ctx, cell_layer, books[cell_index->row].name, NULL, NULL);
//	}
//}

