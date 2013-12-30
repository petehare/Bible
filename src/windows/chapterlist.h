#include "../common.h"

#pragma once

void chapterlist_init(Book *book);
void chapterlist_destroy(void);
void chapterlist_in_received_handler(DictionaryIterator *iter);
