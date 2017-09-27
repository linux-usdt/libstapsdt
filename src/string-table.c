#include <stdio.h>
#include <stdlib.h>

#include "string-table.h"

StringTable *stringTableInit() {
  StringTable *stringTable = (StringTable *)malloc(sizeof(StringTable));
  stringTable->count = 1;
  stringTable->size = 1;

  stringTable->first = (StringTableNode *)malloc(sizeof(StringTableNode));

  stringTable->first->index = 0;
  stringTable->first->size = 1;
  stringTable->first->str = (char *)malloc(sizeof(char));
  stringTable->first->str[0] = '\0';
  stringTable->first->next = NULL;

  return stringTable;
}

StringTableNode *stringTableAdd(StringTable *stringTable, char *str) {
  StringTableNode *current;

  for (current = stringTable->first; current->next != NULL;
       current = current->next) {
  }

  current->next = (StringTableNode *)malloc(sizeof(StringTableNode));
  current->next->index = current->index + current->size;

  current = current->next;
  current->size = strlen(str) + 1;

  current->str = (char *)malloc(current->size);
  memcpy(current->str, str, current->size);
  current->next = NULL;

  stringTable->count += 1;
  stringTable->size += current->size;

  return current;
}

char *stringTableToBuffer(StringTable *stringTable) {
  int offset;
  StringTableNode *current;
  char *buffer = (char *)malloc(stringTable->size);

  for (current = stringTable->first, offset = 0; current != NULL;
       offset += current->size, current = current->next) {
    memcpy(&buffer[offset], current->str, current->size);
  }

  return buffer;
}
