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
  
  
  // keyword
  // exit keyword
  if(strncmp(&source[position], "exit", 4) == 0 && !IsWordChar(source[position + 4])) {
    position += 4;
    
    token.type = TOKEN_KW_EXIT;
    return token;
  }
  // Read print keyword
  if(strncmp(&source[position], "print", 5) == 0 && !IsWordChar(source[position + 5])) {
    position += 5;
    
    token.type = TOKEN_KW_PRINT;
    return token;
  }
  // Read var keyword
  if(strncmp(&source[position], "var", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_KW_VAR;
    return token;
  }
  // Read int keyword
  if(strncmp(&source[position], "int", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_TYPE_INT;
    return token;
  }
  
  
  // literal 
  // Read number literal
  if(source[position] >= '0' && source[position] <= '9') {
    int index = 0;
    
    while(source[position] >= '0' && source[position] <= '9' &&
          index < TOKEN_MAX_LEN - 1) {
      token.value[index++] = source[position++];
    }
    
    token.value[index] = '\0';
    
    token.type = TOKEN_LIT_NUMBER;
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
    
    token.type = TOKEN_LIT_STRING;
    return token;
  }
  
  
  // Read statement separator
  if(source[position] == ';') {
    position++;
    
    token.type = TOKEN_SEMICOLON;
    return token;
  }
  
  
  // operator
  // Read assignment operator
  if(source[position] == '=') {
    position++;
    
    token.type = TOKEN_OP_ASSIGN;
    return token;
  }
  // Read add operator
  if(source[position] == '+') {
    position++;
    
    token.type = TOKEN_OP_ADD;
    return token;
  }
  // Read sub operator
  if(source[position] == '-') {
    position++;
    
    token.type = TOKEN_OP_SUB;
    return token;
  }
  // Read mul operator
  if(source[position] == '*') {
    position++;
    
    token.type = TOKEN_OP_MUL;
    return token;
  }
  // Read div operator
  if(source[position] == '/') {
    position++;
    
    token.type = TOKEN_OP_DIV;
    return token;
  }
  
  
  // Read identifier (must come after all keyword checks above)
  if(IsWordChar(source[position])) {
    int index = 0;
    
    while(IsWordChar(source[position]) && index < TOKEN_MAX_LEN - 1) {
      token.value[index++] = source[position++];
    }
    
    token.value[index] = '\0';
    
    token.type = TOKEN_IDENTIFIER;
    return token;
  }
  
  // Unknown character
  token.type = TOKEN_ERROR;
  
  position++;
  
  return token;
}