#ifndef DICT_H
#define DICT_H

#include "elvm.h"

// A dictionary (the 'dict' type): text keys, each holding an int, float or
// str value. Keys keep the order they were first added in, like a Python
// dict. Lookup goes through a hash index, so it stays fast for big
// dictionaries too.

// One key and its value
typedef struct {
  char* key;
  
  // VAR_INT, VAR_FLOAT or VAR_STR
  VarType type;
  
  // Value of an int or float entry
  double number;
  
  // Value of a str entry, heap-allocated
  char* string;
} DictEntry;

typedef struct Dict {
  int count;
  int capacity;
  
  // Entries in the order they were added
  DictEntry* entries;
  
  // Hash index: slot -> entry index + 1, 0 for an empty slot. Its size is
  // always a power of 2 and always more than twice 'count'.
  int* table;
  int tableSize;
} Dict;

// New empty dictionary
Dict* DictCreate(void);

// Free a dictionary and everything in it (NULL is fine)
void DictFree(Dict* dict);

// Entry for 'key', or NULL if the key is not in the dictionary
DictEntry* DictFind(Dict* dict, const char* key);

// Set 'key' to a number ('type' is VAR_INT or VAR_FLOAT) or a text, adding the
// key if it is new and replacing the old value (of any type) if not
void DictSetNumber(Dict* dict, const char* key, VarType type, double value);
void DictSetString(Dict* dict, const char* key, const char* value);

// Remove 'key'. Returns 1 if it was there, 0 if not.
int DictRemove(Dict* dict, const char* key);

// Name of an entry's type for messages: "int", "float" or "str"
const char* DictTypeName(VarType type);

#endif
