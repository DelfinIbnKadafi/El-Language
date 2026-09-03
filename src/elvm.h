#ifndef ELVM_H
#define ELVM_H

#include "lexer.h"

// Number of variable slots the VM allocates, both for globals and for each
// call frame's locals. Set once, right before VMRun(), to the peak number of
// variables simultaneously in scope anywhere in the compiled program -- so
// there's no fixed cap on how many variables a program can declare.
extern int variableSlotCount;

// Default max characters per string when no explicit str[size] is given.
// A custom size is not limited by this -- see AllocateStringStorage.
#define MAX_STR_LEN 256

// Kept only for the small handful of fixed-size name buffers (variable and
// function names, unrelated to string content length).
#define INSTRUCTION_MAX_LEN 256

// Max depth of function call nesting (including recursion), guards against
// runaway/infinite recursion with a clean error instead of a native crash.
#define MAX_CALL_DEPTH 500

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
  OP_BROADCAST_ARR,
  OP_BROADCAST_STR_ARR,
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
  OP_NOT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_POP,
  OP_PUSH_LAST_NONE_FLAG,
  OP_STR_IS_NONE,
  OP_STR_IS_NOT_NONE,
  OP_CMP_STR_EQ,
  OP_CMP_STR_NE,
  OP_STR_VALUE_IS_NONE,
  OP_STR_VALUE_IS_NOT_NONE,
  OP_CALL,
  OP_RETURN,
  OP_PUSH_STRING_VALUE,
  OP_POP_STRING_VALUE,
  OP_PRINT_STRING_VALUE,
  OP_INPUT_STR,
  OP_DUP,
  OP_DUP_STRING_VALUE,
  OP_PUSH_ARR_ELEMENT_TO_STAGE,
  OP_PUSH_STR_ARR_ELEMENT_TO_STAGE,
  OP_RETURN_ARR,
  OP_RETURN_STR_ARR,
  OP_CAPTURE_RETURNED_ARR,
  OP_CAPTURE_RETURNED_STR_ARR,
  OP_INDEX_RETURNED_ARR,
  OP_INDEX_RETURNED_STR_ARR,
  OP_DISCARD_RETURNED_ARR
} Opcode;

// Supported variable types
typedef enum {
  VAR_INT,
  VAR_FLOAT,
  VAR_STR,
  VAR_BOOL
} VarType;

// Single variable entry. A scalar is simply an array with arraySize == 1,
// always stored / accessed at index 0. Storage is allocated at declare time,
// sized exactly to arraySize, so array size is only limited by available memory.
typedef struct {
  char name[TOKEN_MAX_LEN];
  
  VarType type;
  
  int isArray;
  
  // Number of elements (1 for a scalar)
  int arraySize;
  
  // Max characters allowed per string element, only used when type is VAR_STR
  int strSize;
  
  // Holds int, float, or bool values (bool as 0/1). Allocated to arraySize elements.
  double* numbers;
  
  // Holds string values, only used when type is VAR_STR. Allocated to arraySize
  // pointers, each pointing to a buffer of strSize + 1 characters.
  char** strings;
  
  // 1 if this element (or index 0 for a scalar) has never been given a real
  // value, or was explicitly set to NONE. Not used for VAR_BOOL. Allocated to
  // arraySize elements.
  int* isNone;
} Variable;

// Single bytecode instruction
typedef struct {
  Opcode opcode;
  
  // Variable name (OP_DECLARE_*) or literal text to print (OP_PRINT).
  // Heap-allocated, no length limit.
  char* text;
  
  // String literal to store, used by OP_DECLARE_STR and OP_STORE_STR.
  // Heap-allocated, no length limit.
  char* stringLiteral;
  
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
  
  // Set the destination to NONE instead of storing a value, used by
  // OP_DECLARE_INT/FLOAT/STR and OP_STORE_VAR/STORE_ARR/STORE_STR
  int storeNone;
  
  // After a normal store, also copy the NONE status of the variable that was
  // just read by the immediately preceding OP_PUSH_VAR/OP_PUSH_ARR, used by
  // OP_STORE_VAR/STORE_ARR for bare "x = y;" style assignments
  int propagateNone;
  
  // varIndex refers to a function-local slot (current call frame) instead of
  // a global variable. Used by every opcode that touches a variable.
  int isLocal;
  
  // srcVarIndex refers to a function-local slot instead of a global variable.
  // Used by OP_DECLARE_STR / OP_STORE_STR / OP_BROADCAST_STR_ARR.
  int srcIsLocal;
  
  // Instead of using stringLiteral/srcVarIndex, pop the string value straight
  // off the string-value stack. Used by OP_DECLARE_STR / OP_STORE_STR to bind
  // a string parameter, or to receive a function call's string return value.
  int sourceFromArgStack;
  
  // Instead of using stringLiteral/srcVarIndex/sourceFromArgStack, index
  // straight into the array sitting on top of returnedArrStack (the index
  // itself is already on the eval stack). Used when a call to an
  // array-returning function is indexed directly, e.g. "s = buildArr()[0];".
  int sourceFromReturnedArrIndex;
  
  // Array parameter binding mode, used by OP_DECLARE_INT/FLOAT/BOOL/STR when
  // isArray is set and this declare is bound directly from a call's argument
  // (i.e. it's a parameter, not a plain local array declaration). 0 = not a
  // parameter bind; 1 = pop 'arraySize' values in order (list/variable-copy
  // form, caller already pushed one per element); 2 = pop exactly one value
  // and replicate it across every element (broadcast form).
  int arrayBindMode;
  
  // Element index within the source array, used by OP_PUSH_ARR_ELEMENT_TO_STAGE
  // / OP_PUSH_STR_ARR_ELEMENT_TO_STAGE to read one specific element of an
  // existing array (rather than a runtime-popped index) when a caller passes
  // an existing array variable as an argument.
  int elementIndex;
  
  // Used by OP_INPUT_STR: true if a prompt was written before ";" (e.g.
  // input "Name: ";), false for a bare "input;". When true, the prompt
  // itself is resolved the same way any other string source is (literal,
  // variable, or sourceFromArgStack for a nested string-producing value) and
  // printed with no trailing newline before reading a line from stdin.
  int hasPrompt;
} Instruction;

// Execute bytecode
void VMRun(Instruction* code, int count, char* filename);

#endif
