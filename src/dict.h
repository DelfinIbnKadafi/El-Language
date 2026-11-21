#ifndef DICT_H
#define DICT_H

#include "elvm.h"

// A dictionary (the 'dict' type): text keys, each holding an int, float, bool,
// str or NONE value, or another dictionary. Keys keep the order they were first added in, like a Python
// dict. Lookup goes through a hash index, so it stays fast for big
// dictionaries too.

// One key and its value
typedef struct {
  char* key;
  
  // VAR_INT, VAR_FLOAT, VAR_BOOL, VAR_STR, VAR_NONE or VAR_DICT
  VarType type;
  
  // Value of an int, float or bool (0 or 1) entry
  double number;
  
  // Value of a str entry, heap-allocated
  char* string;
  
  // Value of a dictionary entry (a dictionary inside this one), owned by the
  // entry
  struct Dict* child;
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
  
  // How many levels of dictionaries are inside this one (0 if none). Only an
  // upper bound: it is not lowered when a nested dictionary is removed.
  int depth;
} Dict;

// Dictionaries inside dictionaries can go this deep. Programs that copy a
// dictionary into itself again and again would otherwise build structures
// that are too deep to free or print.
#define MAX_DICT_DEPTH 100

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

// Set 'key' to NONE
void DictSetNone(Dict* dict, const char* key);

// Set 'key' to a dictionary. The entry takes over 'child' (it is freed with
// the dictionary), pass a dictionary nobody else uses. Returns 0 and does
// nothing (the caller still owns 'child') if that would nest dictionaries
// deeper than MAX_DICT_DEPTH.
int DictSetChild(Dict* dict, const char* key, Dict* child);

// A copy of a dictionary, including every dictionary inside it
Dict* DictClone(Dict* dict);

// Remove every key
void DictClear(Dict* dict);

// Make 'dest' hold a copy of everything in 'source' (what it held before is
// gone). Does nothing if they are the same dictionary.
void DictCopyInto(Dict* dest, Dict* source);

// Remove 'key'. Returns 1 if it was there, 0 if not.
int DictRemove(Dict* dict, const char* key);

// Name of an entry's type for messages: "int", "float", "bool", "str",
// "NONE" or "dict"
const char* DictTypeName(VarType type);

// The same with its article in front: "an int", "a float", "a str", ...
const char* DictTypeNameWithArticle(VarType type);

#endif
