#include <stddef.h>

struct string {
  char *data;
  size_t capacity;
  size_t size;
};

void string_init(struct string* string);
void string_free(struct string* string);

void string_push_back(struct string* string, char value);
void string_append(struct string* string, char* cstring);
