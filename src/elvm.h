#ifndef ELVM_H
#define ELVM_H

// VM instruction types
typedef enum {
  OP_PRINT,
  OP_HALT
} Opcode;

// Max length of instruction string data
#define INSTRUCTION_MAX_LEN 256

// Single bytecode instruction
typedef struct {
  Opcode opcode;
  
  // Instruction string data
  char text[INSTRUCTION_MAX_LEN];
} Instruction;

// Execute bytecode
void VMRun(Instruction* code, int count);

#endif