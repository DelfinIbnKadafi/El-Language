/*
  El Language 0.0.1 Alpha - print
  El Language 0.0.2 Alpha - Variable (int, str, bool, float) and math
  El Language 0.1.0 - if / else if / else, comparison, and increment/decrement
  El Language 0.1.1 - comments (line and block), inline single-line block body,
                      empty if/else/else if body no longer errors
  El Language 0.1.2 - fixed block body leaking out after inline+indented mix,
                      added unary minus/plus, clearer error messages
  El Language 0.2.0 - arrays (var type name[size];), sized strings (str[size]),
                      parsing logic moved to its own parser.c / parser.h
  El Language 0.2.1 - array initializers (broadcast and { list } forms), and
                      the NONE value for int, float, str, and bool variables
*/

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
