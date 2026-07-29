#include <string.h>
#include "lexer.h"

// Current source code
char* source;

// Current reading index
int position;

// Current source line
int line;

// Check if character can be part of a word
int IsWordChar(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == '_';
}

void LexerInit(char* input) {
  // Save source reference
  source = input;

  // Reset lexer position
  position = 0;

  // Reset line counter
  line = 1;
}

Token LexerNext() {
  Token token;

  // Skip whitespace
  while(source[position] == ' ' ||
        source[position] == '\n' ||
        source[position] == '\t') {
    if(source[position] == '\n') {
      line++;
    }

    position++;
  }

  // Mark token start line
  token.line = line;

  // Check end of source
  if(source[position] == '\0') {
    token.type = TOKEN_EOF;
    return token;
  }

  // Read print keyword
  if(strncmp(&source[position], "print", 5) == 0 &&
     !IsWordChar(source[position + 5])) {
    position += 5;

    token.type = TOKEN_PRINT;
    return token;
  }

  // Read string literal
  if(source[position] == '"') {
    position++;

    int index = 0;

    while(source[position] != '"' &&
          source[position] != '\0' &&
          index < TOKEN_MAX_LEN - 1) {
      token.value[index++] = source[position++];
    }

    token.value[index] = '\0';

    // Skip closing quote
    if(source[position] == '"') {
      position++;
    }

    token.type = TOKEN_STRING;
    return token;
  }

  // Read statement separator
  if(source[position] == ';') {
    position++;

    token.type = TOKEN_SEMICOLON;
    return token;
  }

  // Unknown character
  token.type = TOKEN_ERROR;

  position++;

  return token;
}