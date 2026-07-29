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

// Initial bytecode capacity
#define INITIAL_BYTECODE_CAP 256

// Bytecode storage
Instruction* bytecode;

// Current bytecode size
int bytecodeCount = 0;

// Current bytecode capacity
int bytecodeCap = 0;

// Add one instruction, growing storage if needed
void EmitInstruction(Instruction instruction) {
  if(bytecodeCount >= bytecodeCap) {
    bytecodeCap *= 2;

    bytecode = realloc(bytecode, bytecodeCap * sizeof(Instruction));
  }

  bytecode[bytecodeCount++] = instruction;
}

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
        printf("Line %d: Expected string\n", string.line);
        exit(1);
      }

      // Validate semicolon
      if(semicolon.type != TOKEN_SEMICOLON) {
        printf("Line %d: Expected ;\n", semicolon.line);
        exit(1);
      }

      // Create print opcode
      Instruction instruction;

      instruction.opcode = OP_PRINT;

      strncpy(
        instruction.text,
        string.value,
        INSTRUCTION_MAX_LEN - 1
      );

      instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';

      EmitInstruction(instruction);
    }
  }

  // Add program stop
  Instruction halt;

  halt.opcode = OP_HALT;

  EmitInstruction(halt);
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

  // Prepare bytecode storage
  bytecodeCap = INITIAL_BYTECODE_CAP;

  bytecode = malloc(bytecodeCap * sizeof(Instruction));

  // Compile source
  Compile();

  // Run VM
  VMRun(
    bytecode,
    bytecodeCount
  );

  // Free memory
  free(source);
  free(bytecode);

  return 0;
}