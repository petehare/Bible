#include "../common.h"

#pragma once

void favoriteslist_init();
void favoriteslist_destroy(void);
void favoriteslist_in_received_handler(DictionaryIterator *iter);
