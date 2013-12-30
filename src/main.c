#include <pebble.h>
#include "appmessage.h"
#include "windows/testamentlist.h"

static void init(void) {
	appmessage_init();
	testamentlist_init();
}

static void deinit(void) {
	testamentlist_destroy();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
