#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "elvm.h"
#include "parser.h"
#include "source.h"

#define ELVM_VERSION "0.8.3"

// Load source file
char* LoadFile(char* filename) {

  // cek extension
  char* ext = strrchr(filename, '.');
  if(ext == NULL || strcmp(ext, ".ell") != 0) {
    printf("File must have .ell extension\n");
    exit(1);
  }

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

  if(strcmp(argv[1], "--version") == 0) {
    printf("elvm version %s\n", ELVM_VERSION);
    return 0;
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
