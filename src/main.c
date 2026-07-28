/*
  El Language 0.0.1 - print
*/

// include libary
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// include file
#include "lexer.h"
#include "elvm.h"

// max Bytecode
#define MAX_BYTECODE 256

// Bytecode storage
Instruction bytecode[MAX_BYTECODE];

// Current bytecode size
int bytecodeCount = 0;

// Load source file
char* LoadFile(char* filename) {
  FILE* file = fopen(filename, "r");

  if(file == NULL) {
    printf("Cannot open file\n");
    exit(1);
  }

  // Get file size
  fseek(file, 0, SEEK_END);

  long size = ftell(file);

  rewind(file);

  char* buffer = malloc(size + 1);

  // Read source content
  fread(buffer, 1, size, file);

  buffer[size] = '\0';

  fclose(file);

  return buffer;
}

// Convert source into bytecode
void Compile() {
  while(1) {
    // Get next token
    Token token = LexerNext();

    if(token.type == TOKEN_EOF) {
      break;
    }

    // Handle print statement
    if(token.type == TOKEN_PRINT) {
      Token string = LexerNext();
      Token semicolon = LexerNext();

      // Validate string
      if(string.type != TOKEN_STRING) {
        printf("Expected string\n");
        exit(1);
      }

      // Validate semicolon
      if(semicolon.type != TOKEN_SEMICOLON) {
        printf("Expected ;\n");
        exit(1);
      }

      // Create print opcode
      bytecode[bytecodeCount].opcode = OP_PRINT;

      strcpy(
        bytecode[bytecodeCount].text,
        string.value
      );

      bytecodeCount++;
    }
  }

  // Add program stop
  bytecode[bytecodeCount].opcode = OP_HALT;
}

int main(int argc, char** argv) {
  if(argc < 2) {
    printf("Usage: elvm <file.ell>\n");
    return 1;
  }

  // Load El source
  char* source = LoadFile(argv[1]);

  // Start lexer
  LexerInit(source);

  // Compile source
  Compile();

  // Run VM
  VMRun(
    bytecode,
    bytecodeCount + 1
  );

  // Free memory
  free(source);

  return 0;
}