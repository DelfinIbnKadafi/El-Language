#ifndef ELVM_H
#define ELVM_H

#include "lexer.h"

// Max number of variables the VM can hold
#define MAX_VARIABLES 256

// Max length of instruction string data
#define INSTRUCTION_MAX_LEN 256

// VM instruction types
typedef enum {
  OP_PRINT,
  OP_PRINT_VALUE,
  OP_PRINT_STR_VAR,
  OP_HALT,
  OP_DECLARE_INT,
  OP_DECLARE_FLOAT,
  OP_DECLARE_BOOL,
  OP_DECLARE_STR,
  OP_STORE_VAR,
  OP_STORE_STR,
  OP_PUSH_NUMBER,
  OP_PUSH_VAR,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_IDIV
} Opcode;

// Supported variable types
typedef enum {
  VAR_INT,
  VAR_FLOAT,
  VAR_STR,
  VAR_BOOL
} VarType;

// Single variable entry
typedef struct {
  char name[TOKEN_MAX_LEN];
  
  VarType type;
  
  // Holds int, float, or bool value (bool as 0/1)
  double numberValue;
  
  // Holds string value, only used when type is VAR_STR
  char stringValue[INSTRUCTION_MAX_LEN];
} Variable;

// Single bytecode instruction
typedef struct {
  Opcode opcode;
  
  // Variable name (OP_DECLARE_*) or literal text to print (OP_PRINT)
  char text[INSTRUCTION_MAX_LEN];
  
  // String literal to store, used by OP_DECLARE_STR and OP_STORE_STR
  char stringLiteral[INSTRUCTION_MAX_LEN];
  
  // Numeric literal value, used by OP_PUSH_NUMBER
  double numberValue;
  
  // Destination variable slot, used by OP_DECLARE_*, OP_STORE_VAR, OP_STORE_STR, OP_PUSH_VAR, OP_PRINT_STR_VAR
  int varIndex;
  
  // Source variable slot for string copy, used by OP_DECLARE_STR / OP_STORE_STR, -1 if unused
  int srcVarIndex;
  
  // Expression result type, used by OP_PRINT_VALUE to format output
  VarType valueType;
  
  // Source line, used to report runtime errors (e.g. OP_DIV, OP_IDIV)
  int line;
} Instruction;

// Execute bytecode
void VMRun(Instruction* code, int count, char* filename);

#endif
