#ifndef LEXER_H
#define LEXER_H

// Token types supported by lexer
typedef enum {
  // keyword
  TOKEN_KW_EXIT,
  TOKEN_KW_PRINT,
  TOKEN_KW_VAR,
  TOKEN_KW_IF,
  TOKEN_KW_ELSE,
  TOKEN_KW_OR,
  TOKEN_KW_AND,
  TOKEN_KW_NONE,
  
  
  // data type
  TOKEN_TYPE_INT,
  TOKEN_TYPE_FLOAT,
  TOKEN_TYPE_STR,
  TOKEN_TYPE_BOOL,
  
  
  // literal
  TOKEN_LIT_STRING,
  TOKEN_LIT_NUMBER,
  TOKEN_LIT_FLOAT,
  TOKEN_LIT_BOOL,
  
  
  // operator
  TOKEN_OP_ASSIGN,
  TOKEN_OP_ADD,
  TOKEN_OP_SUB,
  TOKEN_OP_MUL,
  TOKEN_OP_DIV,
  TOKEN_OP_INC,
  TOKEN_OP_DEC,
  TOKEN_OP_PLUS_ASSIGN,
  TOKEN_OP_MINUS_ASSIGN,
  
  
  // comparison operator
  TOKEN_OP_GT,
  TOKEN_OP_LT,
  TOKEN_OP_EQ,
  TOKEN_OP_GE,
  TOKEN_OP_LE,
  TOKEN_OP_NE,
  
  
  // symbol
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_COMMA,
  
  
  TOKEN_IDENTIFIER,
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
  
  // Source column where token was read, used for indentation-based blocks
  int column;
} Token;

// Initialize lexer source
void LexerInit(char* source);

// Read next token
Token LexerNext();

#endif
