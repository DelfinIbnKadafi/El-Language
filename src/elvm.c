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

// NONE status of the value most recently pushed by OP_PUSH_VAR / OP_PUSH_ARR,
// used to implement "== NONE" checks and bare "x = y;" NONE propagation
int lastPushedIsNone;

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
// popped from the stack when srcIsArray is set). Writes the resolved source index
// (0 for a literal or scalar source) into 'outSrcIdx' for the caller to reuse.
void ResolveStoreSource(Instruction instruction, char* out, int outSize, int* outSrcIdx) {
  char* text;
  int srcIdx = 0;
  
  if(instruction.srcVarIndex == -1) {
    text = instruction.stringLiteral;
  } else {
    if(instruction.srcIsArray) {
      srcIdx = CheckBounds(Pop(), variables[instruction.srcVarIndex].arraySize, instruction.line);
    }
    
    text = variables[instruction.srcVarIndex].strings[srcIdx];
  }
  
  strncpy(out, text, outSize);
  out[outSize] = '\0';
  
  *outSrcIdx = srcIdx;
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
        
        if(instruction.propagateNone && lastPushedIsNone) {
          printf("NONE\n");
        } else if(instruction.valueType == VAR_FLOAT) {
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
        
        if(variables[instruction.varIndex].isNone[idx]) {
          printf("NONE\n");
        } else {
          printf("%s\n", variables[instruction.varIndex].strings[idx]);
        }
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
        
        if(instruction.isArray) {
          // Every element starts as NONE; a broadcast/list initializer, if any,
          // is applied afterward through separate OP_STORE_ARR instructions
          for(int i = 0; i < instruction.arraySize; i++) {
            variables[instruction.varIndex].isNone[i] = 1;
          }
        } else if(instruction.storeNone) {
          variables[instruction.varIndex].numbers[0] = 0;
          variables[instruction.varIndex].isNone[0] = 1;
        } else {
          // Scalar: pop its single initializer value
          variables[instruction.varIndex].numbers[0] = Pop();
          variables[instruction.varIndex].isNone[0] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        
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
        
        if(instruction.isArray) {
          // Every element starts as NONE; a broadcast/list initializer, if any,
          // is applied afterward through separate OP_STORE_STR instructions
          for(int i = 0; i < instruction.arraySize; i++) {
            variables[instruction.varIndex].strings[i][0] = '\0';
            variables[instruction.varIndex].isNone[i] = 1;
          }
        } else if(instruction.storeNone) {
          variables[instruction.varIndex].strings[0][0] = '\0';
          variables[instruction.varIndex].isNone[0] = 1;
        } else {
          // Scalar: copy its single initializer string
          int maxLen = variables[instruction.varIndex].strSize;
          int srcIdx = 0;
          
          ResolveStoreSource(instruction, variables[instruction.varIndex].strings[0], maxLen, &srcIdx);
          
          variables[instruction.varIndex].isNone[0] =
            (instruction.srcVarIndex != -1) ? variables[instruction.srcVarIndex].isNone[srcIdx] : 0;
        }
        
        variableCount++;
        break;
      }
      case OP_STORE_VAR:
        // Overwrite an existing scalar int, float, or bool variable
        if(instruction.storeNone) {
          variables[instruction.varIndex].numbers[0] = 0;
          variables[instruction.varIndex].isNone[0] = 1;
        } else {
          variables[instruction.varIndex].numbers[0] = Pop();
          variables[instruction.varIndex].isNone[0] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        break;
      case OP_STORE_STR: {
        int destIdx = 0;
        
        if(instruction.destIsArray) {
          destIdx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
        }
        
        if(instruction.storeNone) {
          variables[instruction.varIndex].strings[destIdx][0] = '\0';
          variables[instruction.varIndex].isNone[destIdx] = 1;
        } else {
          int maxLen = variables[instruction.varIndex].strSize;
          int srcIdx = 0;
          
          ResolveStoreSource(instruction, variables[instruction.varIndex].strings[destIdx], maxLen, &srcIdx);
          
          variables[instruction.varIndex].isNone[destIdx] =
            (instruction.srcVarIndex != -1) ? variables[instruction.srcVarIndex].isNone[srcIdx] : 0;
        }
        break;
      }
      case OP_PUSH_NUMBER:
        Push(instruction.numberValue);
        break;
      case OP_PUSH_VAR:
        Push(variables[instruction.varIndex].numbers[0]);
        lastPushedIsNone = variables[instruction.varIndex].isNone[0];
        break;
      case OP_PUSH_ARR: {
        int idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
        
        Push(variables[instruction.varIndex].numbers[idx]);
        lastPushedIsNone = variables[instruction.varIndex].isNone[idx];
        break;
      }
      case OP_STORE_ARR: {
        if(instruction.storeNone) {
          int idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
          
          variables[instruction.varIndex].numbers[idx] = 0;
          variables[instruction.varIndex].isNone[idx] = 1;
        } else {
          double value = Pop();
          int idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
          
          variables[instruction.varIndex].numbers[idx] = value;
          variables[instruction.varIndex].isNone[idx] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
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
      case OP_POP:
        Pop();
        break;
      case OP_PUSH_LAST_NONE_FLAG: {
        int result = lastPushedIsNone;
        
        if(instruction.numberValue != 0) {
          result = !result;
        }
        
        Push(result ? 1 : 0);
        break;
      }
      case OP_STR_IS_NONE:
      case OP_STR_IS_NOT_NONE: {
        int idx = 0;
        
        if(instruction.destIsArray) {
          idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
        }
        
        int isNone = variables[instruction.varIndex].isNone[idx];
        
        Push((instruction.opcode == OP_STR_IS_NONE ? isNone : !isNone) ? 1 : 0);
        break;
      }
      case OP_HALT:
        return;
    }
  }
}
