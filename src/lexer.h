#ifndef LEXER_H
#define LEXER_H

// Token types supported by lexer
typedef enum {
  TOKEN_PRINT,
  TOKEN_STRING,
  TOKEN_SEMICOLON,
  TOKEN_EOF,
  TOKEN_ERROR
} TokenType;

// Store token information
typedef struct {
  TokenType type;

  // Token text data
  char value[256];
} Token;

// Initialize lexer source
void LexerInit(char* source);

// Read next token
Token LexerNext();

#endif