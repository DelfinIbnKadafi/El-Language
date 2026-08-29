#include <stdio.h>
#include <string.h>
#include "lexer.h"

// Current source code
char* source;

// Current reading index
int position;

// Current source line
int line;

// Source index of the start of the current line, used to compute column
int lineStart;

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
  
  // Reset column tracking
  lineStart = 0;
}

Token LexerNext() {
  Token token;
  
  // Skip whitespace and comments
  while(1) {
    if(source[position] == ' ' || source[position] == '\t') {
      position++;
      continue;
    }
    
    if(source[position] == '\n') {
      line++;
      
      position++;
      
      lineStart = position;
      continue;
    }
    
    // Skip line comment
    if(source[position] == '/' && source[position + 1] == '/') {
      position += 2;
      
      while(source[position] != '\n' && source[position] != '\0') {
        position++;
      }
      
      continue;
    }
    
    // Skip block comment
    if(source[position] == '/' && source[position + 1] == '*') {
      position += 2;
      
      while(!(source[position] == '*' && source[position + 1] == '/') &&
            source[position] != '\0') {
        if(source[position] == '\n') {
          line++;
          
          position++;
          
          lineStart = position;
        } else {
          position++;
        }
      }
      
      if(source[position] == '*' && source[position + 1] == '/') {
        position += 2;
        
        continue;
      }
      
      // Reached end of file without a closing */
      token.line = line;
      token.column = position - lineStart + 1;
      token.type = TOKEN_ERROR;
      
      strcpy(token.value, "Unterminated comment");
      return token;
    }
    
    break;
  }
  
  // Mark token start line and column
  token.line = line;
  token.column = position - lineStart + 1;
  
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
  // Read input keyword
  if(strncmp(&source[position], "input", 5) == 0 && !IsWordChar(source[position + 5])) {
    position += 5;
    
    token.type = TOKEN_KW_INPUT;
    return token;
  }
  // Read var keyword
  if(strncmp(&source[position], "var", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_KW_VAR;
    return token;
  }
  // Read if keyword
  if(strncmp(&source[position], "if", 2) == 0 && !IsWordChar(source[position + 2])) {
    position += 2;
    
    token.type = TOKEN_KW_IF;
    return token;
  }
  // Read else keyword
  if(strncmp(&source[position], "else", 4) == 0 && !IsWordChar(source[position + 4])) {
    position += 4;
    
    token.type = TOKEN_KW_ELSE;
    return token;
  }
  // Read for keyword
  if(strncmp(&source[position], "for", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_KW_FOR;
    return token;
  }
  // Read while keyword
  if(strncmp(&source[position], "while", 5) == 0 && !IsWordChar(source[position + 5])) {
    position += 5;
    
    token.type = TOKEN_KW_WHILE;
    return token;
  }
  // Read function keyword
  if(strncmp(&source[position], "function", 8) == 0 && !IsWordChar(source[position + 8])) {
    position += 8;
    
    token.type = TOKEN_KW_FUNCTION;
    return token;
  }
  // Read return keyword
  if(strncmp(&source[position], "return", 6) == 0 && !IsWordChar(source[position + 6])) {
    position += 6;
    
    token.type = TOKEN_KW_RETURN;
    return token;
  }
  // Read or keyword
  if(strncmp(&source[position], "or", 2) == 0 && !IsWordChar(source[position + 2])) {
    position += 2;
    
    token.type = TOKEN_KW_OR;
    return token;
  }
  // Read and keyword
  if(strncmp(&source[position], "and", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_KW_AND;
    return token;
  }
  // Read not keyword
  if(strncmp(&source[position], "not", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_KW_NOT;
    return token;
  }
  // Read NONE keyword
  if(strncmp(&source[position], "NONE", 4) == 0 && !IsWordChar(source[position + 4])) {
    position += 4;
    
    token.type = TOKEN_KW_NONE;
    return token;
  }
  
  
  // data type
  // Read int keyword
  if(strncmp(&source[position], "int", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_TYPE_INT;
    return token;
  }
  // Read float keyword
  if(strncmp(&source[position], "float", 5) == 0 && !IsWordChar(source[position + 5])) {
    position += 5;
    
    token.type = TOKEN_TYPE_FLOAT;
    return token;
  }
  // Read str keyword
  if(strncmp(&source[position], "str", 3) == 0 && !IsWordChar(source[position + 3])) {
    position += 3;
    
    token.type = TOKEN_TYPE_STR;
    return token;
  }
  // Read bool keyword
  if(strncmp(&source[position], "bool", 4) == 0 && !IsWordChar(source[position + 4])) {
    position += 4;
    
    token.type = TOKEN_TYPE_BOOL;
    return token;
  }
  
  
  // literal
  // Read true literal
  if(strncmp(&source[position], "true", 4) == 0 && !IsWordChar(source[position + 4])) {
    position += 4;
    
    strcpy(token.value, "1");
    
    token.type = TOKEN_LIT_BOOL;
    return token;
  }
  // Read false literal
  if(strncmp(&source[position], "false", 5) == 0 && !IsWordChar(source[position + 5])) {
    position += 5;
    
    strcpy(token.value, "0");
    
    token.type = TOKEN_LIT_BOOL;
    return token;
  }
  // Read number or float literal
  if(source[position] >= '0' && source[position] <= '9') {
    int index = 0;
    int isFloat = 0;
    
    while(source[position] >= '0' && source[position] <= '9') {
      if(index < TOKEN_MAX_LEN - 1) {
        token.value[index++] = source[position];
      }
      
      position++;
    }
    
    // Check decimal point
    if(source[position] == '.' &&
       source[position + 1] >= '0' && source[position + 1] <= '9') {
      isFloat = 1;
      
      if(index < TOKEN_MAX_LEN - 1) {
        token.value[index++] = source[position];
      }
      
      position++;
      
      while(source[position] >= '0' && source[position] <= '9') {
        if(index < TOKEN_MAX_LEN - 1) {
          token.value[index++] = source[position];
        }
        
        position++;
      }
    }
    
    token.value[index] = '\0';
    
    token.type = isFloat ? TOKEN_LIT_FLOAT : TOKEN_LIT_NUMBER;
    return token;
  }
  // Read string literal
  if(source[position] == '"') {
    position++;
    
    int index = 0;
    
    while(source[position] != '"' &&
          source[position] != '\0') {
      if(index < TOKEN_MAX_LEN - 1) {
        token.value[index++] = source[position];
      }
      
      position++;
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
  // Read equal / assign operator
  if(source[position] == '=') {
    if(source[position + 1] == '=') {
      position += 2;
      
      token.type = TOKEN_OP_EQ;
      return token;
    }
    
    position++;
    
    token.type = TOKEN_OP_ASSIGN;
    return token;
  }
  // Read not-equal operator
  if(source[position] == '!' && source[position + 1] == '=') {
    position += 2;
    
    token.type = TOKEN_OP_NE;
    return token;
  }
  // Read greater / greater-equal operator
  if(source[position] == '>') {
    if(source[position + 1] == '=') {
      position += 2;
      
      token.type = TOKEN_OP_GE;
      return token;
    }
    
    position++;
    
    token.type = TOKEN_OP_GT;
    return token;
  }
  // Read less / less-equal operator
  if(source[position] == '<') {
    if(source[position + 1] == '=') {
      position += 2;
      
      token.type = TOKEN_OP_LE;
      return token;
    }
    
    position++;
    
    token.type = TOKEN_OP_LT;
    return token;
  }
  // Read increment / add-assign / add operator
  if(source[position] == '+') {
    if(source[position + 1] == '+') {
      position += 2;
      
      token.type = TOKEN_OP_INC;
      return token;
    }
    
    if(source[position + 1] == '=') {
      position += 2;
      
      token.type = TOKEN_OP_PLUS_ASSIGN;
      return token;
    }
    
    position++;
    
    token.type = TOKEN_OP_ADD;
    return token;
  }
  // Read decrement / sub-assign / sub operator
  if(source[position] == '-') {
    if(source[position + 1] == '-') {
      position += 2;
      
      token.type = TOKEN_OP_DEC;
      return token;
    }
    
    if(source[position + 1] == '=') {
      position += 2;
      
      token.type = TOKEN_OP_MINUS_ASSIGN;
      return token;
    }
    
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
  
  
  // symbol
  // Read left paren
  if(source[position] == '(') {
    position++;
    
    token.type = TOKEN_LPAREN;
    return token;
  }
  // Read right paren
  if(source[position] == ')') {
    position++;
    
    token.type = TOKEN_RPAREN;
    return token;
  }
  // Read left bracket
  if(source[position] == '[') {
    position++;
    
    token.type = TOKEN_LBRACKET;
    return token;
  }
  // Read right bracket
  if(source[position] == ']') {
    position++;
    
    token.type = TOKEN_RBRACKET;
    return token;
  }
  // Read left brace
  if(source[position] == '{') {
    position++;
    
    token.type = TOKEN_LBRACE;
    return token;
  }
  // Read right brace
  if(source[position] == '}') {
    position++;
    
    token.type = TOKEN_RBRACE;
    return token;
  }
  // Read comma
  if(source[position] == ',') {
    position++;
    
    token.type = TOKEN_COMMA;
    return token;
  }
  
  
  // Read identifier (must come after all keyword checks above)
  if(IsWordChar(source[position])) {
    int index = 0;
    
    while(IsWordChar(source[position])) {
      if(index < TOKEN_MAX_LEN - 1) {
        token.value[index++] = source[position];
      }
      
      position++;
    }
    
    token.value[index] = '\0';
    
    token.type = TOKEN_IDENTIFIER;
    return token;
  }
  
  // Unknown character
  sprintf(token.value, "Unexpected character '%c'", source[position]);
  
  token.type = TOKEN_ERROR;
  
  position++;
  
  return token;
}
