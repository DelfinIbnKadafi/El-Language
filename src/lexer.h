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

// Max length of token text data
#define TOKEN_MAX_LEN 256

// Store token information
typedef struct {
  TokenType type;

  // Token text data
  char value[TOKEN_MAX_LEN];

  // Source line where token was read
  int line;
} Token;

// Initialize lexer source
void LexerInit(char* source);

// Read next token
Token LexerNext();

#endif