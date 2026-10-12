#ifndef JSON_H
#define JSON_H

// Minimal JSON reader, used for library.json. Reads the whole JSON format
// into a tree of values.

typedef enum {
  JSON_NULL,
  JSON_BOOL,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT
} JsonType;

// One JSON value
typedef struct JsonValue {
  JsonType type;
  
  // Text of a JSON_STRING
  char* string;
  
  // Value of a JSON_NUMBER
  double number;
  
  // Value of a JSON_BOOL
  int boolean;
  
  // Items of a JSON_ARRAY, or the values of a JSON_OBJECT
  struct JsonValue** items;
  
  // Names of the values of a JSON_OBJECT, in the same order as 'items'
  char** keys;
  
  // Number of items
  int count;
} JsonValue;

// Read JSON text. Returns NULL on failure, with a message such as
// "line 3: Expected ':'" copied into 'error'.
JsonValue* JsonParse(const char* text, char* error, int errorSize);

// Find a value of an object by name, or NULL if there is none
JsonValue* JsonGet(JsonValue* object, const char* key);

// Free a value and everything inside it
void JsonFree(JsonValue* value);

#endif
