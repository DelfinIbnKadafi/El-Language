#ifndef ELVM_H
#define ELVM_H

// VM instruction types
typedef enum {
  OP_PRINT,
  OP_HALT
} Opcode;

// Single bytecode instruction
typedef struct {
  Opcode opcode;

  // Instruction string data
  char text[256];
} Instruction;

// Execute bytecode
void VMRun(Instruction* code, int count);

#endif