#pragma once

typedef enum {
	ListTypeBook = 0x0,
  ListTypeViewer = 0x1
} ListType;

typedef enum {
	TestamentTypeOld = 0x0,
	TestamentTypeNew = 0x1,
} TestamentType;

typedef struct {
  int index;
	char name[24];
  int chapters;
} Book;

enum {
	KEY_LIST,
  KEY_REQUEST,
  KEY_INDEX,
  KEY_TESTAMENT,
	KEY_BOOK,
	KEY_CHAPTER,
  KEY_CONTENT,
  KEY_TOKEN
};
