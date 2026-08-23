#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "elvm.h"
#include "parser.h"

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

int main(int argc, char** argv) {
  if(argc < 2) {
    printf("Usage: elvm <file.ell>\n");
    return 1;
  }
  
  // Remember filename for error messages
  sourceFilename = argv[1];
  
  // Load El source
  char* source = LoadFile(argv[1]);
  
  // Start lexer
  LexerInit(source);
  
  // Compile source
  Compile();
  
  // Run VM
  VMRun(
    bytecode,
    bytecodeCount,
    sourceFilename
  );
  
  // Free memory
  free(source);
  free(bytecode);
  
  return 0;
}
