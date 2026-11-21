#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "elvm.h"
#include "parser.h"
#include "source.h"

// Load source file
char* LoadFile(char* filename) {
  char* buffer = ReadFileText(filename);
  
  if(buffer == NULL) {
    printf("Cannot open file\n");
    exit(1);
  }
  
  if(!SourceTextFits(buffer)) {
    printf("%s : File has too many lines\n", filename);
    exit(1);
  }
  
  return buffer;
}

int main(int argc, char** argv) {
  if(argc < 2) {
    printf("Usage: elvm <file.ell>\n");
    return 1;
  }
  
  // Remember where elvm is, libraries are searched next to it
  SetExecutablePath(argv[0]);
  
  // Remember filename for error messages
  sourceFilename = argv[1];
  
  // The main file is always the first registered source file (id 0)
  AddSourceFile(argv[1], argv[1]);
  
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
