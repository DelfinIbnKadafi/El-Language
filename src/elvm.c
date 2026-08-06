#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elvm.h"

#define MAX_STACK 256

Variable variables[MAX_VARIABLES];
int variableCount = 0;

// Evaluation stack, used for numeric expression math (int, float, bool, comparisons, indices)
double stack[MAX_STACK];
int stackTop = 0;

// Current running file, used for runtime error messages
char* currentFilename;

// Push value onto evaluation stack
void Push(double value) {
  stack[stackTop++] = value;
}

// Pop value from evaluation stack
double Pop() {
  return stack[--stackTop];
}

// Validate an array index is within bounds, stopping the program with a clear
// error if not. Returns the index as an int for convenience.
int CheckBounds(double rawIndex, int size, int line) {
  int index = (int) rawIndex;
  
  if(index < 0 || index >= size) {
    printf("%s (%d) : Array index %d out of bounds (size %d)\n", currentFilename, line, index, size);
    exit(1);
  }
  
  return index;
}

// Resolve the string to copy for OP_DECLARE_STR / OP_STORE_STR into 'out': either
// a literal, a whole scalar variable, or one element of an array variable (index
// popped from the stack when srcIsArray is set).
void ResolveStoreSource(Instruction instruction, char* out, int outSize) {
  char* text;
  
  if(instruction.srcVarIndex == -1) {
    text = instruction.stringLiteral;
  } else {
    int srcIdx = 0;
    
    if(instruction.srcIsArray) {
      srcIdx = CheckBounds(Pop(), variables[instruction.srcVarIndex].arraySize, instruction.line);
    }
    
    text = variables[instruction.srcVarIndex].strings[srcIdx];
  }
  
  strncpy(out, text, outSize);
  out[outSize] = '\0';
}

void VMRun(Instruction* code, int count, char* filename) {
  int ip = 0;
  
  currentFilename = filename;
  
  while(ip < count) {
    Instruction instruction = code[ip++];
    
    switch(instruction.opcode) {
      case OP_PRINT:
        printf("%s\n", instruction.text);
        break;
      case OP_PRINT_VALUE: {
        double value = Pop();
        
        if(instruction.valueType == VAR_FLOAT) {
          printf("%g\n", value);
        } else if(instruction.valueType == VAR_BOOL) {
          printf("%s\n", value != 0 ? "true" : "false");
        } else {
          printf("%d\n", (int) value);
        }
        break;
      }
      case OP_PRINT_STR_VAR: {
        int idx = 0;
        
        if(instruction.destIsArray) {
          idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
        }
        
        printf("%s\n", variables[instruction.varIndex].strings[idx]);
        break;
      }
      case OP_DECLARE_INT:
      case OP_DECLARE_FLOAT:
      case OP_DECLARE_BOOL:
        strncpy(variables[instruction.varIndex].name, instruction.text, TOKEN_MAX_LEN - 1);
        variables[instruction.varIndex].name[TOKEN_MAX_LEN - 1] = '\0';
        
        if(instruction.opcode == OP_DECLARE_INT) {
          variables[instruction.varIndex].type = VAR_INT;
        } else if(instruction.opcode == OP_DECLARE_FLOAT) {
          variables[instruction.varIndex].type = VAR_FLOAT;
        } else {
          variables[instruction.varIndex].type = VAR_BOOL;
        }
        
        variables[instruction.varIndex].isArray = instruction.isArray;
        variables[instruction.varIndex].arraySize = instruction.isArray ? instruction.arraySize : 1;
        
        if(!instruction.isArray) {
          // Scalar: pop its single initializer value
          variables[instruction.varIndex].numbers[0] = Pop();
        }
        // Arrays start zero-initialized (static storage default), no initializer supported
        
        variableCount++;
        break;
      case OP_DECLARE_STR: {
        strncpy(variables[instruction.varIndex].name, instruction.text, TOKEN_MAX_LEN - 1);
        variables[instruction.varIndex].name[TOKEN_MAX_LEN - 1] = '\0';
        
        variables[instruction.varIndex].type = VAR_STR;
        variables[instruction.varIndex].isArray = instruction.isArray;
        variables[instruction.varIndex].arraySize = instruction.isArray ? instruction.arraySize : 1;
        variables[instruction.varIndex].strSize =
          instruction.strSize > 0 ? instruction.strSize : (MAX_STR_LEN - 1);
        
        if(!instruction.isArray) {
          // Scalar: copy its single initializer string
          int maxLen = variables[instruction.varIndex].strSize;
          
          ResolveStoreSource(instruction, variables[instruction.varIndex].strings[0], maxLen);
        }
        // Arrays start as empty strings (zero-initialized), no initializer supported
        
        variableCount++;
        break;
      }
      case OP_STORE_VAR:
        // Overwrite an existing scalar int, float, or bool variable
        variables[instruction.varIndex].numbers[0] = Pop();
        break;
      case OP_STORE_STR: {
        int destIdx = 0;
        
        if(instruction.destIsArray) {
          destIdx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
        }
        
        int maxLen = variables[instruction.varIndex].strSize;
        
        ResolveStoreSource(instruction, variables[instruction.varIndex].strings[destIdx], maxLen);
        break;
      }
      case OP_PUSH_NUMBER:
        Push(instruction.numberValue);
        break;
      case OP_PUSH_VAR:
        Push(variables[instruction.varIndex].numbers[0]);
        break;
      case OP_PUSH_ARR: {
        int idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
        
        Push(variables[instruction.varIndex].numbers[idx]);
        break;
      }
      case OP_STORE_ARR: {
        double value = Pop();
        int idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
        
        variables[instruction.varIndex].numbers[idx] = value;
        break;
      }
      case OP_ADD: {
        double b = Pop();
        double a = Pop();
        
        Push(a + b);
        break;
      }
      case OP_SUB: {
        double b = Pop();
        double a = Pop();
        
        Push(a - b);
        break;
      }
      case OP_MUL: {
        double b = Pop();
        double a = Pop();
        
        Push(a * b);
        break;
      }
      case OP_DIV: {
        double b = Pop();
        double a = Pop();
        
        if(b == 0) {
          printf("%s (%d) : Division by zero\n", filename, instruction.line);
          exit(1);
        }
        
        Push(a / b);
        break;
      }
      case OP_IDIV: {
        double b = Pop();
        double a = Pop();
        
        if(b == 0) {
          printf("%s (%d) : Division by zero\n", filename, instruction.line);
          exit(1);
        }
        
        Push((double) ((long) a / (long) b));
        break;
      }
      case OP_CMP_GT: {
        double b = Pop();
        double a = Pop();
        
        Push(a > b ? 1 : 0);
        break;
      }
      case OP_CMP_LT: {
        double b = Pop();
        double a = Pop();
        
        Push(a < b ? 1 : 0);
        break;
      }
      case OP_CMP_EQ: {
        double b = Pop();
        double a = Pop();
        
        Push(a == b ? 1 : 0);
        break;
      }
      case OP_CMP_GE: {
        double b = Pop();
        double a = Pop();
        
        Push(a >= b ? 1 : 0);
        break;
      }
      case OP_CMP_LE: {
        double b = Pop();
        double a = Pop();
        
        Push(a <= b ? 1 : 0);
        break;
      }
      case OP_CMP_NE: {
        double b = Pop();
        double a = Pop();
        
        Push(a != b ? 1 : 0);
        break;
      }
      case OP_AND: {
        double b = Pop();
        double a = Pop();
        
        Push((a != 0 && b != 0) ? 1 : 0);
        break;
      }
      case OP_OR: {
        double b = Pop();
        double a = Pop();
        
        Push((a != 0 || b != 0) ? 1 : 0);
        break;
      }
      case OP_JUMP:
        ip = instruction.jumpTarget;
        break;
      case OP_JUMP_IF_FALSE: {
        double condition = Pop();
        
        if(condition == 0) {
          ip = instruction.jumpTarget;
        }
        break;
      }
      case OP_HALT:
        return;
    }
  }
}
