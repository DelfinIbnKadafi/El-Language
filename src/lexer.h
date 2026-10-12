#ifndef LEXER_H
#define LEXER_H

// Token types supported by lexer
typedef enum {
  // keyword
  TOKEN_KW_EXIT,
  TOKEN_KW_PRINT,
  TOKEN_KW_INPUT,
  TOKEN_KW_FORMAT,
  TOKEN_KW_VAR,
  TOKEN_KW_IF,
  TOKEN_KW_ELSE,
  TOKEN_KW_FOR,
  TOKEN_KW_WHILE,
  TOKEN_KW_FUNCTION,
  TOKEN_KW_RETURN,
  TOKEN_KW_OR,
  TOKEN_KW_AND,
  TOKEN_KW_NOT,
  TOKEN_KW_NONE,
  TOKEN_KW_MODULE,
  TOKEN_KW_LIBRARY,
  
  
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
} LexerTokenType;

// Max length used only for fixed-size name buffers elsewhere (variable and
// function names). Token text itself is unbounded -- see Token.value below.
#define TOKEN_MAX_LEN 256

// Store token information
typedef struct {
  LexerTokenType type;
  
  // Token text data. Heap-allocated and grown as needed, so identifiers and
  // string/number literals have no fixed length limit. NULL for token types
  // that carry no text (keywords, operators, symbols).
  char* value;
  
  // Source location where token was read: the line and the file it belongs
  // to packed into one int -- see MakeLocation in source.h
  int line;
  
  // Source column where token was read, used for indentation-based blocks
  int column;
} Token;

// Initialize lexer source
void LexerInit(char* source);

// Start reading another source file in the middle of the current one, as if
// its text was written right here (module / library). Once it ends, reading
// continues in the file that was paused. 'fileId' is the id the file was
// registered with in source.h. The lexer takes ownership of 'input' and frees
// it when the file ends.
void LexerPushSource(char* input, int fileId);

// Read next token
Token LexerNext();

// Append one character to a growable buffer, doubling capacity as needed.
// Always leaves room for a trailing '\0' after the append.
void AppendChar(char** buffer, int* capacity, int* length, char c);

// Copy a string onto the heap (stand-in for strdup, kept within -std=c99)
char* DupString(const char* text);

#endif
