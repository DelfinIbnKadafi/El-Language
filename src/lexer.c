#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"
#include "source.h"

// Copy a string onto the heap. Used instead of the non-standard strdup() to
// stay within -std=c99.
char* DupString(const char* text) {
  char* copy = malloc(strlen(text) + 1);
  
  strcpy(copy, text);
  
  return copy;
}

// Append one character to a growable buffer, doubling capacity as needed.
// Always leaves room for a trailing '\0' after the append.
void AppendChar(char** buffer, int* capacity, int* length, char c) {
  if(*length + 1 >= *capacity) {
    *capacity *= 2;
    *buffer = realloc(*buffer, *capacity);
  }
  
  (*buffer)[(*length)++] = c;
}

// Current source code
char* source;

// Current reading index
int position;

// Current source location (line and file, see MakeLocation in source.h)
int line;

// Source index of the start of the current line, used to compute column
int lineStart;

// State of a file that is paused while an included file is being read
typedef struct {
  char* source;
  int position;
  int line;
  int lineStart;
} LexerState;

// Paused files, innermost last. Grows as needed, so there's no fixed cap on
// how deep includes can be nested.
LexerState* pausedStates;
int pausedCapacity = 0;
int pausedCount = 0;

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
  
  // Reset line counter (the main file always has file id 0)
  line = MakeLocation(0, 1);
  
  // Reset column tracking
  lineStart = 0;
}

void LexerPushSource(char* input, int fileId) {
  if(pausedCount >= pausedCapacity) {
    pausedCapacity = pausedCapacity == 0 ? 8 : pausedCapacity * 2;
    pausedStates = realloc(pausedStates, pausedCapacity * sizeof(LexerState));
  }
  
  // Pause the current file exactly where it is
  pausedStates[pausedCount].source = source;
  pausedStates[pausedCount].position = position;
  pausedStates[pausedCount].line = line;
  pausedStates[pausedCount].lineStart = lineStart;
  
  pausedCount++;
  
  // Start the new file from its first line
  source = input;
  position = 0;
  line = MakeLocation(fileId, 1);
  lineStart = 0;
}

// Finish the current included file and go back to the one that was paused
void LexerPopSource() {
  free(source);
  
  pausedCount--;
  
  source = pausedStates[pausedCount].source;
  position = pausedStates[pausedCount].position;
  line = pausedStates[pausedCount].line;
  lineStart = pausedStates[pausedCount].lineStart;
}

// Read next token from the current file only
Token LexerReadToken() {
  Token token;
  
  // Most token types carry no text; only set for literals/identifiers/errors
  token.value = NULL;
  
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
      
      token.value = DupString("Unterminated comment");
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
  // Read format keyword
  if(strncmp(&source[position], "format", 6) == 0 && !IsWordChar(source[position + 6])) {
    position += 6;
    
    token.type = TOKEN_KW_FORMAT;
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
  // Read module keyword
  if(strncmp(&source[position], "module", 6) == 0 && !IsWordChar(source[position + 6])) {
    position += 6;
    
    token.type = TOKEN_KW_MODULE;
    return token;
  }
  // Read library keyword
  if(strncmp(&source[position], "library", 7) == 0 && !IsWordChar(source[position + 7])) {
    position += 7;
    
    token.type = TOKEN_KW_LIBRARY;
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
    
    token.value = DupString("1");
    
    token.type = TOKEN_LIT_BOOL;
    return token;
  }
  // Read false literal
  if(strncmp(&source[position], "false", 5) == 0 && !IsWordChar(source[position + 5])) {
    position += 5;
    
    token.value = DupString("0");
    
    token.type = TOKEN_LIT_BOOL;
    return token;
  }
  // Read number or float literal
  if(source[position] >= '0' && source[position] <= '9') {
    int capacity = 32;
    int index = 0;
    int isFloat = 0;
    char* buffer = malloc(capacity);
    
    while(source[position] >= '0' && source[position] <= '9') {
      AppendChar(&buffer, &capacity, &index, source[position]);
      
      position++;
    }
    
    // Check decimal point
    if(source[position] == '.' &&
       source[position + 1] >= '0' && source[position + 1] <= '9') {
      isFloat = 1;
      
      AppendChar(&buffer, &capacity, &index, source[position]);
      
      position++;
      
      while(source[position] >= '0' && source[position] <= '9') {
        AppendChar(&buffer, &capacity, &index, source[position]);
        
        position++;
      }
    }
    
    buffer[index] = '\0';
    token.value = buffer;
    
    token.type = isFloat ? TOKEN_LIT_FLOAT : TOKEN_LIT_NUMBER;
    return token;
  }
  // Read string literal
  if(source[position] == '"') {
    position++;
    
    int capacity = 32;
    int index = 0;
    char* buffer = malloc(capacity);
    
    while(source[position] != '"' &&
          source[position] != '\0') {
      AppendChar(&buffer, &capacity, &index, source[position]);
      
      position++;
    }
    
    buffer[index] = '\0';
    token.value = buffer;
    
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
    int capacity = 32;
    int index = 0;
    char* buffer = malloc(capacity);
    
    while(IsWordChar(source[position])) {
      AppendChar(&buffer, &capacity, &index, source[position]);
      
      position++;
    }
    
    buffer[index] = '\0';
    token.value = buffer;
    
    token.type = TOKEN_IDENTIFIER;
    return token;
  }
  
  // Unknown character
  token.value = malloc(32);
  sprintf(token.value, "Unexpected character '%c'", source[position]);
  
  token.type = TOKEN_ERROR;
  
  position++;
  
  return token;
}

// Set when an included file has just ended, until the next token is read, and
// the location where the file that included it was paused
int resumedAfterInclude = 0;
int resumedLine = 0;

Token LexerNext() {
  while(1) {
    Token token = LexerReadToken();
    
    // An included file ended, keep reading where the file that included it
    // was paused. The end of an included file is never seen by the parser.
    if(token.type == TOKEN_EOF && pausedCount > 0) {
      LexerPopSource();
      
      resumedAfterInclude = 1;
      resumedLine = line;
      continue;
    }
    
    // Code written after 'module "file.ell";' on the same line starts a new
    // statement, so it is given the column of a statement at the start of a
    // line. Otherwise its column would make it look like part of a block that
    // was open at the end of the included file.
    if(resumedAfterInclude) {
      resumedAfterInclude = 0;
      
      if(token.line == resumedLine) {
        token.column = 1;
      }
    }
    
    return token;
  }
}
