#include <pebble.h>
#include "appmessage.h"
#include "common.h"
#include "libs/pebble-assist.h"
#include "windows/testamentlist.h"
#include "windows/booklist.h"
#include "windows/viewer.h"

static void in_received_handler(DictionaryIterator *iter, void *context);
static void in_dropped_handler(AppMessageResult reason, void *context);
static void out_sent_handler(DictionaryIterator *sent, void *context);
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);

void appmessage_init(void) {
	app_message_open(128 /* inbound_size */, 64 /* outbound_size */);
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMessage initialised");
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming AppMessage from Pebble received");
	Tuple *list_tuple = dict_find(iter, KEY_LIST);

	if (list_tuple) {
		switch (list_tuple->value->int16) {
			case ListTypeBook:
				booklist_in_received_handler(iter);
				break;
      case ListTypeViewer:
				viewer_in_received_handler(iter);
				break;
		}
	}
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming AppMessage from Pebble dropped, %d", reason);
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send AppMessage to Pebble");
}
