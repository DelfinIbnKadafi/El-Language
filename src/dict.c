#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dict.h"

#define DICT_START_TABLE_SIZE 8

static unsigned int HashKey(const char* key) {
  unsigned int hash = 2166136261u;
  
  while(*key != '\0') {
    hash ^= (unsigned char) *key++;
    hash *= 16777619u;
  }
  
  return hash;
}

// Fill the hash index again from the entries (after the table grew, or an
// entry was removed)
static void RebuildTable(Dict* dict, int tableSize) {
  free(dict->table);
  
  dict->tableSize = tableSize;
  dict->table = calloc(tableSize, sizeof(int));
  
  if(dict->table == NULL) {
    printf("Out of memory\n");
    exit(1);
  }
  
  for(int i = 0; i < dict->count; i++) {
    unsigned int slot = HashKey(dict->entries[i].key) & (tableSize - 1);
    
    while(dict->table[slot] != 0) {
      slot = (slot + 1) & (tableSize - 1);
    }
    
    dict->table[slot] = i + 1;
  }
}

Dict* DictCreate(void) {
  Dict* dict = calloc(1, sizeof(Dict));
  
  if(dict == NULL) {
    printf("Out of memory\n");
    exit(1);
  }
  
  RebuildTable(dict, DICT_START_TABLE_SIZE);
  
  return dict;
}

static void FreeEntry(DictEntry* entry) {
  free(entry->key);
  free(entry->string);
}

void DictFree(Dict* dict) {
  if(dict == NULL) {
    return;
  }
  
  for(int i = 0; i < dict->count; i++) {
    FreeEntry(&dict->entries[i]);
  }
  
  free(dict->entries);
  free(dict->table);
  free(dict);
}

DictEntry* DictFind(Dict* dict, const char* key) {
  unsigned int slot = HashKey(key) & (dict->tableSize - 1);
  
  while(dict->table[slot] != 0) {
    DictEntry* entry = &dict->entries[dict->table[slot] - 1];
    
    if(strcmp(entry->key, key) == 0) {
      return entry;
    }
    
    slot = (slot + 1) & (dict->tableSize - 1);
  }
  
  return NULL;
}

// Entry for 'key', adding an empty one at the end if it is not there yet
static DictEntry* FindOrAdd(Dict* dict, const char* key) {
  DictEntry* found = DictFind(dict, key);
  
  if(found != NULL) {
    free(found->string);
    found->string = NULL;
    
    return found;
  }
  
  if(dict->count >= dict->capacity) {
    dict->capacity = dict->capacity == 0 ? 8 : dict->capacity * 2;
    dict->entries = realloc(dict->entries, dict->capacity * sizeof(DictEntry));
    
    if(dict->entries == NULL) {
      printf("Out of memory\n");
      exit(1);
    }
  }
  
  DictEntry* entry = &dict->entries[dict->count];
  
  memset(entry, 0, sizeof(DictEntry));
  entry->key = DupString(key);
  
  dict->count++;
  
  if(dict->count * 2 >= dict->tableSize) {
    RebuildTable(dict, dict->tableSize * 2);
  } else {
    unsigned int slot = HashKey(key) & (dict->tableSize - 1);
    
    while(dict->table[slot] != 0) {
      slot = (slot + 1) & (dict->tableSize - 1);
    }
    
    dict->table[slot] = dict->count;
  }
  
  return entry;
}

void DictSetNumber(Dict* dict, const char* key, VarType type, double value) {
  DictEntry* entry = FindOrAdd(dict, key);
  
  entry->type = type;
  entry->number = value;
}

void DictSetString(Dict* dict, const char* key, const char* value) {
  // 'value' may point into the entry being replaced, so copy it first
  char* copy = DupString(value);
  DictEntry* entry = FindOrAdd(dict, key);
  
  entry->type = VAR_STR;
  entry->number = 0;
  entry->string = copy;
}

int DictRemove(Dict* dict, const char* key) {
  DictEntry* entry = DictFind(dict, key);
  
  if(entry == NULL) {
    return 0;
  }
  
  int index = (int) (entry - dict->entries);
  
  FreeEntry(entry);
  
  for(int i = index; i < dict->count - 1; i++) {
    dict->entries[i] = dict->entries[i + 1];
  }
  
  dict->count--;
  
  RebuildTable(dict, dict->tableSize);
  
  return 1;
}

const char* DictTypeName(VarType type) {
  if(type == VAR_INT) {
    return "int";
  }
  
  if(type == VAR_FLOAT) {
    return "float";
  }
  
  return "str";
}
