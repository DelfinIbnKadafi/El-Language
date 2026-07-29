#ifndef ELVM_H
#define ELVM_H

#include "lexer.h"

// Max number of variables the VM can hold
#define MAX_VARIABLES 256

// VM instruction types
typedef enum {
  OP_PRINT,
  OP_HALT,
  OP_DECLARE_INT,
  OP_STORE_VAR,
  OP_PUSH_NUMBER,
  OP_PUSH_VAR,
  OP_PRINT_VALUE,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV
} Opcode;

// Supported variable types
typedef enum {
  VAR_INT
} VarType;

// Single variable entry
typedef struct {
  char name[TOKEN_MAX_LEN];
  
  VarType type;
  
  int value;
} Variable;

// Max length of instruction string data
#define INSTRUCTION_MAX_LEN 256

// Single bytecode instruction
typedef struct {
  Opcode opcode;
  
  // Instruction string data
  char text[INSTRUCTION_MAX_LEN];
  
  // Numeric literal value, used by OP_PUSH_NUMBER
  int intValue;
  
  // Variable slot index, used by OP_DECLARE_INT, OP_STORE_VAR, OP_PUSH_VAR
  int varIndex;
} Instruction;

// Execute bytecode
void VMRun(Instruction* code, int count);

#endif