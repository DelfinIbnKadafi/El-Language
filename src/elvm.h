#ifndef ELVM_H
#define ELVM_H

#include "lexer.h"

// Max number of variables the VM can hold
#define MAX_VARIABLES 256

// Max number of elements in an array
#define MAX_ARRAY_SIZE 64

// Max length of instruction string data
#define INSTRUCTION_MAX_LEN 256

// Max characters per string, per array element (matches INSTRUCTION_MAX_LEN)
#define MAX_STR_LEN 256

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
  OP_PUSH_ARR,
  OP_STORE_ARR,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_IDIV,
  OP_CMP_GT,
  OP_CMP_LT,
  OP_CMP_EQ,
  OP_CMP_GE,
  OP_CMP_LE,
  OP_CMP_NE,
  OP_AND,
  OP_OR,
  OP_JUMP,
  OP_JUMP_IF_FALSE
} Opcode;

// Supported variable types
typedef enum {
  VAR_INT,
  VAR_FLOAT,
  VAR_STR,
  VAR_BOOL
} VarType;

// Single variable entry. A scalar is simply an array with arraySize == 1,
// always stored / accessed at index 0.
typedef struct {
  char name[TOKEN_MAX_LEN];
  
  VarType type;
  
  int isArray;
  
  // Number of elements (1 for a scalar)
  int arraySize;
  
  // Max characters allowed per string element, only used when type is VAR_STR
  int strSize;
  
  // Holds int, float, or bool values (bool as 0/1)
  double numbers[MAX_ARRAY_SIZE];
  
  // Holds string values, only used when type is VAR_STR
  char strings[MAX_ARRAY_SIZE][MAX_STR_LEN];
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
  
  // Destination variable slot
  int varIndex;
  
  // Source variable slot for string copy, used by OP_DECLARE_STR / OP_STORE_STR, -1 if unused
  int srcVarIndex;
  
  // Expression result type, used by OP_PRINT_VALUE to format output
  VarType valueType;
  
  // Source line, used to report runtime errors (division, array bounds)
  int line;
  
  // Bytecode index to jump to, used by OP_JUMP and OP_JUMP_IF_FALSE
  int jumpTarget;
  
  // Declares an array instead of a scalar, used by OP_DECLARE_*
  int isArray;
  
  // Number of elements to declare, used by OP_DECLARE_* when isArray is set
  int arraySize;
  
  // Max characters per string element, used by OP_DECLARE_STR (0 = use default)
  int strSize;
  
  // Destination (varIndex) is accessed by index popped from the stack,
  // used by OP_STORE_STR and OP_PRINT_STR_VAR
  int destIsArray;
  
  // Source (srcVarIndex) is accessed by index popped from the stack,
  // used by OP_DECLARE_STR and OP_STORE_STR
  int srcIsArray;
} Instruction;

// Execute bytecode
void VMRun(Instruction* code, int count, char* filename);

#endif
