#pragma once

void appmessage_init(void);

unsigned int appmessage_cancel_request(unsigned int token);
unsigned int appmessage_verseslist_request_data(char *book_name, uint8_t current_chapter);
unsigned int appmessage_favoriteslist_request_data(void);
unsigned int appmessage_booklist_request_data(uint8_t testament);
unsigned int appmessage_viewer_request_data(char* book_name, uint8_t current_chapter, char* range);
unsigned int appmessage_viewer_toggle_favorite(char* book_name, uint8_t current_chapter, char* range);