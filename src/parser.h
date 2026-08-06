#ifndef PARSER_H
#define PARSER_H

#include "elvm.h"

// Bytecode produced by Compile(), consumed by VMRun()
extern Instruction* bytecode;

// Current bytecode size
extern int bytecodeCount;

// Source filename, used for error messages and passed on to VMRun()
extern char* sourceFilename;

// Convert the source loaded into the lexer into bytecode
void Compile(void);

#endif
