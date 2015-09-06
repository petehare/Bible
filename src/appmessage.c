#include <pebble.h>
#include "appmessage.h"
#include "common.h"
#include "libs/pebble-assist.h"
#include "windows/testamentlist.h"
#include "windows/booklist.h"
#include "windows/verseslist.h"
#include "windows/favoriteslist.h"
#include "windows/viewer.h"

#define MAX_SEND_ATTEMPTS 3

typedef struct OutMessage {
    uint8_t request_type;
    unsigned int token;
    uint8_t testament;
    char* book_name;
    uint8_t chapter;
    char* range;
} OutMessage;

typedef struct OutMessageQueue OutMessageQueue;
struct OutMessageQueue {
  OutMessage* message;
  OutMessageQueue* next;
  uint8_t send_attempts;
};

static void in_received_handler(DictionaryIterator *iter, void *context);
static void in_dropped_handler(AppMessageResult reason, void *context);
static void out_sent_handler(DictionaryIterator *sent, void *context);
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);
static unsigned int enqueue_message(OutMessage *message);
static void process_next_message();
static void destroy_out_message(OutMessage *message);

static OutMessageQueue* out_message_queue = NULL;
static bool send_in_progress = false;
static bool pebble_js_initialized = false;

void appmessage_init(void) {
  app_message_open(128 /* inbound_size */, 128 /* outbound_size */);
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMessage initialised");
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming AppMessage from Pebble received");
	Tuple *type_tuple = dict_find(iter, KEY_MESSAGE_TYPE);

	if (type_tuple) {
        switch (type_tuple->value->int16) {
            case MessageTypeBook:
                booklist_in_received_handler(iter);
                break;
            case MessageTypeVerses:
                verseslist_in_received_handler(iter);
                break;
            case MessageTypeViewer:
                viewer_in_received_handler(iter);
                break;
            case MessageTypeFavorites:
                favoriteslist_in_received_handler(iter);
                break;
            case MessageTypeFavoritesDidChange:
                favoriteslist_mark_dirty();
                break;
            case MessageTypePebbleJSInitialized:
            	pebble_js_initialized = true;
            	process_next_message();
                break;
        }
    }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming AppMessage from Pebble dropped, %d", reason);
}

static void enqueue_next_message(bool sent_successfully) {
  // dequeue the message
  if (sent_successfully || (out_message_queue != NULL && out_message_queue->send_attempts >= MAX_SEND_ATTEMPTS)) {
      destroy_out_message(out_message_queue->message);
      out_message_queue = out_message_queue->next;
  }
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
    send_in_progress = false;
    enqueue_next_message(true);
    process_next_message();
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send AppMessage to Pebble, %d", reason);
    send_in_progress = false;
    enqueue_next_message(false);
    process_next_message();
}

static void message_to_iter(OutMessage *message, DictionaryIterator *iter) {
    Tuplet request_tuple = TupletInteger(KEY_REQUEST, message->request_type);
	dict_write_tuplet(iter, &request_tuple);

    if (message->book_name != NULL) {
      Tuplet book_tuple = TupletCString(KEY_BOOK, message->book_name);
      dict_write_tuplet(iter, &book_tuple);
    }

    if (message->range != NULL) {
      Tuplet range_tuple = TupletCString(KEY_RANGE, message->range);
      dict_write_tuplet(iter, &range_tuple);
    }

    Tuplet testament_tuple = TupletInteger(KEY_TESTAMENT, message->testament);
    dict_write_tuplet(iter, &testament_tuple);

    Tuplet chapter_tuple = TupletInteger(KEY_CHAPTER, message->chapter);
    dict_write_tuplet(iter, &chapter_tuple);

    Tuplet token_tuple = TupletInteger(KEY_TOKEN, message->token);
    dict_write_tuplet(iter, &token_tuple);

	dict_write_end(iter);
}

static void process_next_message() {
  if (pebble_js_initialized == false) {
    return;
  }

  OutMessageQueue* omq = out_message_queue;
  if (omq == NULL) {
    send_in_progress = false;
    return;
  }

  if (send_in_progress) {
    return;
  }

  DictionaryIterator* dict;
  app_message_outbox_begin(&dict);
  if (dict != NULL) {
    message_to_iter(omq->message, dict);
    omq->send_attempts = omq->send_attempts + 1;
    if (omq->send_attempts > 1) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "message being sent again: %d", omq->send_attempts);
    }
    AppMessageResult result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
      enqueue_next_message(false);
      process_next_message();
      return;
    }
    send_in_progress = true;
  }
}

static unsigned int enqueue_message(OutMessage *message) {
  if (message == NULL) {
    return 0;
  }

  OutMessageQueue* omq = malloc(sizeof(OutMessageQueue));
  omq->next = NULL;
  omq->message = message;
  omq->send_attempts = 0;

  if (out_message_queue == NULL) {
    out_message_queue = omq;
  }
  else {
    OutMessageQueue* eoq = out_message_queue;
    while (eoq->next != NULL) {
      eoq = eoq->next;
    }
    eoq->next = omq;
  }

  process_next_message();
  return message->token;
}

static void destroy_out_message(OutMessage *message) {
  if (message != NULL) {
    if (message->range != NULL) {
      free(message->range);
    }
    if (message->book_name != NULL) {
      free(message->book_name);
    }
    free(message);
  }
}

static OutMessage* create_out_message(uint8_t request_type, uint8_t *testament, char* book_name, uint8_t *chapter, char* range, unsigned int *token) {
  OutMessage *message = malloc(sizeof(OutMessage));
  memset(message, 0, sizeof(OutMessage));
  message->request_type = request_type;

  if (testament != NULL) {
    message->testament = *testament;
  }

  if (book_name != NULL) {
    message->book_name = malloc(strlen(book_name)+1);
    strcpy(message->book_name, book_name);
  }

  if (chapter != NULL) {
    message->chapter = *chapter;
  }

  if (range != NULL) {
    message->range = malloc(strlen(range)+1);
    strcpy(message->range, range);
  }
  
  message->token = token != NULL ? (unsigned int)*token : (unsigned int)time(NULL);
  return message;
}

// ---------------------------------------------------
unsigned int appmessage_cancel_request(unsigned int token) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "appmessage_cancel_request");
  return enqueue_message(create_out_message(RequestTypeCancel, NULL, NULL, NULL, NULL, &token));
}

unsigned int appmessage_viewer_toggle_favorite(char* book_name, uint8_t chapter, char* range) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "appmessage_viewer_toggle_favorite");
  return enqueue_message(create_out_message(RequestTypeToggleFavorite, NULL, book_name, &chapter, range, NULL));
}

unsigned int appmessage_viewer_request_data(char* book_name, uint8_t chapter, char* range) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "appmessage_viewer_request_data");
  return enqueue_message(create_out_message(RequestTypeViewer, NULL, book_name, &chapter, range, NULL));
}

unsigned int appmessage_verseslist_request_data(char *book_name, uint8_t chapter) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "appmessage_verseslist_request_data");
  return enqueue_message(create_out_message(RequestTypeVerses, NULL, book_name, &chapter, NULL, NULL));
}

unsigned int appmessage_favoriteslist_request_data(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "appmessage_favoriteslist_request_data");
  return enqueue_message(create_out_message(RequestTypeFavorites, NULL, NULL, NULL, NULL, NULL));
}

unsigned int appmessage_booklist_request_data(uint8_t testament) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "appmessage_booklist_request_data");
  return enqueue_message(create_out_message(RequestTypeBooks, &testament, NULL, NULL, NULL, NULL));
}

