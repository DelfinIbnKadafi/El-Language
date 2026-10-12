#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "lexer.h"
#include "json.h"

// How deep arrays and objects may be nested
#define JSON_MAX_DEPTH 64

// JSON text being read, and the current reading index
const char* jsonText;
int jsonPosition;

// Current line, used for error messages
int jsonLine;

// Current nesting depth
int jsonDepth;

// Where the error message is written, and whether one was already written
char* jsonError;
int jsonErrorSize;
int jsonFailed;

// Remember the first error found. Later ones are only a result of the first.
void JsonFail(char* format, ...) {
  if(jsonFailed) {
    return;
  }
  
  jsonFailed = 1;
  
  char message[200];
  
  va_list args;
  
  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);
  
  snprintf(jsonError, jsonErrorSize, "line %d: %s", jsonLine, message);
}

// Skip spaces, tabs and line breaks
void JsonSkipSpace() {
  while(jsonText[jsonPosition] == ' ' ||
        jsonText[jsonPosition] == '\t' ||
        jsonText[jsonPosition] == '\r' ||
        jsonText[jsonPosition] == '\n') {
    if(jsonText[jsonPosition] == '\n') {
      jsonLine++;
    }
    
    jsonPosition++;
  }
}

JsonValue* JsonNew(JsonType type) {
  JsonValue* value = calloc(1, sizeof(JsonValue));
  
  value->type = type;
  
  return value;
}

void JsonFree(JsonValue* value) {
  if(value == NULL) {
    return;
  }
  
  free(value->string);
  
  for(int i = 0; i < value->count; i++) {
    JsonFree(value->items[i]);
    
    if(value->keys != NULL) {
      free(value->keys[i]);
    }
  }
  
  free(value->items);
  free(value->keys);
  free(value);
}

JsonValue* JsonGet(JsonValue* object, const char* key) {
  if(object == NULL || object->type != JSON_OBJECT) {
    return NULL;
  }
  
  for(int i = 0; i < object->count; i++) {
    if(strcmp(object->keys[i], key) == 0) {
      return object->items[i];
    }
  }
  
  return NULL;
}

// Read four hex digits after \u. Returns -1 if they are not valid.
int JsonReadHex4() {
  int result = 0;
  
  for(int i = 0; i < 4; i++) {
    char c = jsonText[jsonPosition];
    int digit;
    
    if(c >= '0' && c <= '9') {
      digit = c - '0';
    } else if(c >= 'a' && c <= 'f') {
      digit = c - 'a' + 10;
    } else if(c >= 'A' && c <= 'F') {
      digit = c - 'A' + 10;
    } else {
      return -1;
    }
    
    result = result * 16 + digit;
    jsonPosition++;
  }
  
  return result;
}

// Append a Unicode character to a buffer as UTF-8
void JsonAppendUtf8(char** buffer, int* capacity, int* length, long code) {
  if(code < 0x80) {
    AppendChar(buffer, capacity, length, (char) code);
  } else if(code < 0x800) {
    AppendChar(buffer, capacity, length, (char) (0xC0 | (code >> 6)));
    AppendChar(buffer, capacity, length, (char) (0x80 | (code & 0x3F)));
  } else if(code < 0x10000) {
    AppendChar(buffer, capacity, length, (char) (0xE0 | (code >> 12)));
    AppendChar(buffer, capacity, length, (char) (0x80 | ((code >> 6) & 0x3F)));
    AppendChar(buffer, capacity, length, (char) (0x80 | (code & 0x3F)));
  } else {
    AppendChar(buffer, capacity, length, (char) (0xF0 | (code >> 18)));
    AppendChar(buffer, capacity, length, (char) (0x80 | ((code >> 12) & 0x3F)));
    AppendChar(buffer, capacity, length, (char) (0x80 | ((code >> 6) & 0x3F)));
    AppendChar(buffer, capacity, length, (char) (0x80 | (code & 0x3F)));
  }
}

// Read a string, starting at its opening quote. Returns NULL on failure.
char* JsonParseString() {
  // Skip opening quote
  jsonPosition++;
  
  int capacity = 32;
  int index = 0;
  char* buffer = malloc(capacity);
  
  while(jsonText[jsonPosition] != '"') {
    char c = jsonText[jsonPosition];
    
    if(c == '\0' || c == '\n') {
      JsonFail("Unterminated string");
      free(buffer);
      return NULL;
    }
    
    if(c != '\\') {
      AppendChar(&buffer, &capacity, &index, c);
      jsonPosition++;
      continue;
    }
    
    // Escape sequence
    jsonPosition++;
    
    char escape = jsonText[jsonPosition];
    
    jsonPosition++;
    
    if(escape == '"' || escape == '\\' || escape == '/') {
      AppendChar(&buffer, &capacity, &index, escape);
    } else if(escape == 'b') {
      AppendChar(&buffer, &capacity, &index, '\b');
    } else if(escape == 'f') {
      AppendChar(&buffer, &capacity, &index, '\f');
    } else if(escape == 'n') {
      AppendChar(&buffer, &capacity, &index, '\n');
    } else if(escape == 'r') {
      AppendChar(&buffer, &capacity, &index, '\r');
    } else if(escape == 't') {
      AppendChar(&buffer, &capacity, &index, '\t');
    } else if(escape == 'u') {
      long code = JsonReadHex4();
      
      if(code < 0) {
        JsonFail("Invalid \\u escape in string");
        free(buffer);
        return NULL;
      }
      
      // A high surrogate must be followed by a low one to form one character
      if(code >= 0xD800 && code <= 0xDBFF) {
        long low = -1;
        
        if(jsonText[jsonPosition] == '\\' && jsonText[jsonPosition + 1] == 'u') {
          jsonPosition += 2;
          
          low = JsonReadHex4();
        }
        
        if(low >= 0xDC00 && low <= 0xDFFF) {
          code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
        } else {
          JsonFail("Invalid surrogate pair in string");
          free(buffer);
          return NULL;
        }
      } else if(code >= 0xDC00 && code <= 0xDFFF) {
        JsonFail("Invalid surrogate pair in string");
        free(buffer);
        return NULL;
      }
      
      JsonAppendUtf8(&buffer, &capacity, &index, code);
    } else {
      JsonFail("Invalid escape '\\%c' in string", escape);
      free(buffer);
      return NULL;
    }
  }
  
  // Skip closing quote
  jsonPosition++;
  
  buffer[index] = '\0';
  
  return buffer;
}

JsonValue* JsonParseValue() {
  JsonSkipSpace();
  
  char c = jsonText[jsonPosition];
  
  if(c == '"') {
    char* text = JsonParseString();
    
    if(text == NULL) {
      return NULL;
    }
    
    JsonValue* value = JsonNew(JSON_STRING);
    
    value->string = text;
    return value;
  }
  
  if(c == '{' || c == '[') {
    int isObject = c == '{';
    char closer = isObject ? '}' : ']';
    
    if(jsonDepth >= JSON_MAX_DEPTH) {
      JsonFail("Nested too deeply");
      return NULL;
    }
    
    jsonDepth++;
    jsonPosition++;
    
    JsonValue* value = JsonNew(isObject ? JSON_OBJECT : JSON_ARRAY);
    int capacity = 0;
    
    JsonSkipSpace();
    
    // Empty array or object
    if(jsonText[jsonPosition] == closer) {
      jsonPosition++;
      jsonDepth--;
      return value;
    }
    
    while(1) {
      char* key = NULL;
      
      if(isObject) {
        JsonSkipSpace();
        
        if(jsonText[jsonPosition] != '"') {
          JsonFail("Expected a string as the name of a value");
          JsonFree(value);
          return NULL;
        }
        
        key = JsonParseString();
        
        if(key == NULL) {
          JsonFree(value);
          return NULL;
        }
        
        JsonSkipSpace();
        
        if(jsonText[jsonPosition] != ':') {
          JsonFail("Expected ':'");
          free(key);
          JsonFree(value);
          return NULL;
        }
        
        jsonPosition++;
      }
      
      JsonValue* item = JsonParseValue();
      
      if(item == NULL) {
        free(key);
        JsonFree(value);
        return NULL;
      }
      
      if(value->count >= capacity) {
        capacity = capacity == 0 ? 4 : capacity * 2;
        value->items = realloc(value->items, capacity * sizeof(JsonValue*));
        
        if(isObject) {
          value->keys = realloc(value->keys, capacity * sizeof(char*));
        }
      }
      
      value->items[value->count] = item;
      
      if(isObject) {
        value->keys[value->count] = key;
      }
      
      value->count++;
      
      JsonSkipSpace();
      
      if(jsonText[jsonPosition] == ',') {
        jsonPosition++;
        continue;
      }
      
      if(jsonText[jsonPosition] == closer) {
        jsonPosition++;
        break;
      }
      
      JsonFail("Expected ',' or '%c'", closer);
      JsonFree(value);
      return NULL;
    }
    
    jsonDepth--;
    return value;
  }
  
  if(strncmp(&jsonText[jsonPosition], "true", 4) == 0) {
    jsonPosition += 4;
    
    JsonValue* value = JsonNew(JSON_BOOL);
    
    value->boolean = 1;
    return value;
  }
  
  if(strncmp(&jsonText[jsonPosition], "false", 5) == 0) {
    jsonPosition += 5;
    
    return JsonNew(JSON_BOOL);
  }
  
  if(strncmp(&jsonText[jsonPosition], "null", 4) == 0) {
    jsonPosition += 4;
    
    return JsonNew(JSON_NULL);
  }
  
  if(c == '-' || (c >= '0' && c <= '9')) {
    // Only characters that can be part of a JSON number are taken, so strtod
    // never reads forms JSON does not have (hex, inf, nan)
    int length = 0;
    
    while((jsonText[jsonPosition + length] >= '0' && jsonText[jsonPosition + length] <= '9') ||
          jsonText[jsonPosition + length] == '-' ||
          jsonText[jsonPosition + length] == '+' ||
          jsonText[jsonPosition + length] == '.' ||
          jsonText[jsonPosition + length] == 'e' ||
          jsonText[jsonPosition + length] == 'E') {
      length++;
    }
    
    char* digits = malloc(length + 1);
    
    memcpy(digits, &jsonText[jsonPosition], length);
    
    digits[length] = '\0';
    
    char* end;
    double number = strtod(digits, &end);
    int valid = end == digits + length;
    
    free(digits);
    
    if(!valid) {
      JsonFail("Invalid number");
      return NULL;
    }
    
    jsonPosition += length;
    
    JsonValue* value = JsonNew(JSON_NUMBER);
    
    value->number = number;
    return value;
  }
  
  if(c == '\0') {
    JsonFail("Unexpected end of file");
  } else {
    JsonFail("Unexpected character '%c'", c);
  }
  
  return NULL;
}

JsonValue* JsonParse(const char* text, char* error, int errorSize) {
  jsonText = text;
  jsonPosition = 0;
  jsonLine = 1;
  jsonDepth = 0;
  jsonError = error;
  jsonErrorSize = errorSize;
  jsonFailed = 0;
  
  // Skip the UTF-8 byte order mark some editors add
  if((unsigned char) text[0] == 0xEF &&
     (unsigned char) text[1] == 0xBB &&
     (unsigned char) text[2] == 0xBF) {
    jsonPosition = 3;
  }
  
  JsonValue* value = JsonParseValue();
  
  if(value == NULL) {
    return NULL;
  }
  
  JsonSkipSpace();
  
  if(jsonText[jsonPosition] != '\0') {
    JsonFail("Unexpected content after the end of the JSON value");
    JsonFree(value);
    return NULL;
  }
  
  return value;
}
