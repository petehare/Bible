#include "../common.h"

#pragma once

void verseslist_init(Book *book, int chapter);
void verseslist_destroy(void);
void verseslist_in_received_handler(DictionaryIterator *iter);
