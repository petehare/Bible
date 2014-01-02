#include "../common.h"

#pragma once

void viewer_init(Book *book, int chapter);
void viewer_destroy(void);
void viewer_in_received_handler(DictionaryIterator *iter);
