#include "fsc/internal/string.h"

#include <stdlib.h>

void string_init(struct string* string) {
  string->capacity = 1;
  string->size = 1;
  string->data = malloc(1);
  string->data[0] = '\0';
}

void string_free(struct string* string) {
  free(string->data);
}

static void push_back(struct string* string, char value) {
  if (string->size == string->capacity) {
    string->capacity *= 2;

    string->data = realloc(string->data, string->capacity);
  }

  string->data[string->size++] = value;
}

void string_push_back(struct string* string, char value) {
  string->data[string->size] = value;
  push_back(string, '\0');
}

void string_append(struct string* string, char* cstring) {
  while (*cstring) {
    string_push_back(string, *cstring);
    ++cstring;
  }
}
