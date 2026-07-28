#include <string.h>
#include "lexer.h"

// Current source code
char* source;

// Current reading index
int position;

void LexerInit(char* input) {
  // Save source reference
  source = input;

  // Reset lexer position
  position = 0;
}

Token LexerNext() {
  Token token;

  // Skip whitespace
  while(source[position] == ' ' ||
        source[position] == '\n' ||
        source[position] == '\t') {
    position++;
  }

  // Check end of source
  if(source[position] == '\0') {
    token.type = TOKEN_EOF;
    return token;
  }

  // Read print keyword
  if(strncmp(&source[position], "print", 5) == 0) {
    position += 5;

    token.type = TOKEN_PRINT;
    return token;
  }

  // Read string literal
  if(source[position] == '"') {
    position++;

    int index = 0;

    while(source[position] != '"' &&
          source[position] != '\0') {
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