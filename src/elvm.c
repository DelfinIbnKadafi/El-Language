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

// Allocate the number/isNone storage for a variable holding 'size' elements,
// stopping the program with a clear error if memory could not be obtained.
// Above this, Linux's memory overcommit can let malloc "succeed" and then
// have the OS kill the process later once the memory is actually touched,
// which looks like a crash rather than a clean error. Refuse before that
// point instead, with a clear message.
#define MAX_SAFE_ALLOCATION_BYTES 1073741824L

// Check that a requested allocation of 'count' elements of 'elementSize' bytes
// each stays within a sane total, stopping the program with a clear error if not.
void CheckAllocationSize(long count, long elementSize, int line) {
  double totalBytes = (double) count * (double) elementSize;
  
  if(totalBytes > MAX_SAFE_ALLOCATION_BYTES) {
    printf("%s (%d) : Requested storage (%.0f MB) exceeds the safety limit of %ld MB\n",
      currentFilename, line, totalBytes / (1024 * 1024), MAX_SAFE_ALLOCATION_BYTES / (1024 * 1024));
    exit(1);
  }
}

void AllocateNumberStorage(int varIndex, int size, int line) {
  CheckAllocationSize(size, sizeof(double) + sizeof(int), line);
  
  // Free any previous allocation for this variable slot, in case this declare
  // instruction sits inside a loop body and is running again. free(NULL) is
  // always safe, so this is harmless on the very first declaration too.
  free(variables[varIndex].numbers);
  free(variables[varIndex].isNone);
  
  variables[varIndex].numbers = calloc(size, sizeof(double));
  variables[varIndex].isNone = malloc(size * sizeof(int));
  
  if(variables[varIndex].numbers == NULL || variables[varIndex].isNone == NULL) {
    printf("%s (%d) : Failed to allocate array of size %d\n", currentFilename, line, size);
    exit(1);
  }
}

// Allocate the string storage for a variable holding 'size' elements, each up
// to 'strSize' characters, stopping the program with a clear error if memory
// could not be obtained.
void AllocateStringStorage(int varIndex, int size, int strSize, int line) {
  CheckAllocationSize(size, strSize + 1 + sizeof(char*) + sizeof(int), line);
  
  // Free any previous allocation for this variable slot, in case this declare
  // instruction sits inside a loop body and is running again.
  if(variables[varIndex].strings != NULL) {
    for(int i = 0; i < variables[varIndex].arraySize; i++) {
      free(variables[varIndex].strings[i]);
    }
    
    free(variables[varIndex].strings);
  }
  
  free(variables[varIndex].isNone);
  
  variables[varIndex].strings = malloc(size * sizeof(char*));
  variables[varIndex].isNone = malloc(size * sizeof(int));
  
  if(variables[varIndex].strings == NULL || variables[varIndex].isNone == NULL) {
    printf("%s (%d) : Failed to allocate array of size %d\n", currentFilename, line, size);
    exit(1);
  }
  
  for(int i = 0; i < size; i++) {
    variables[varIndex].strings[i] = calloc(strSize + 1, sizeof(char));
    
    if(variables[varIndex].strings[i] == NULL) {
      printf("%s (%d) : Failed to allocate string storage\n", currentFilename, line);
      exit(1);
    }
  }
}

// Free the dynamically allocated storage of every declared variable, called
// once the program finishes running. Iterates over the fixed MAX_VARIABLES
// bound (not variableCount, which can be inflated by a var declaration that
// sits inside a loop body and therefore runs more than once) and skips any
// slot that was never actually allocated.
void FreeAllVariables() {
  for(int i = 0; i < MAX_VARIABLES; i++) {
    if(variables[i].numbers == NULL && variables[i].strings == NULL) {
      continue;
    }
    
    free(variables[i].numbers);
    
    if(variables[i].strings != NULL) {
      for(int j = 0; j < variables[i].arraySize; j++) {
        free(variables[i].strings[j]);
      }
      
      free(variables[i].strings);
    }
    
    free(variables[i].isNone);
  }
}

// Resolve the string to copy for OP_DECLARE_STR / OP_STORE_STR into 'out': either
// a literal, a whole scalar variable, or one element of an array variable (index
// popped from the stack when srcIsArray is set). Writes the resolved source index
// (0 for a literal or scalar source) into 'outSrcIdx' for the caller to reuse.
// Must be called before popping any destination index, since the source index
// (if any) was pushed after it and therefore sits on top of the stack.
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
      case OP_DECLARE_BOOL: {
        strncpy(variables[instruction.varIndex].name, instruction.text, TOKEN_MAX_LEN - 1);
        variables[instruction.varIndex].name[TOKEN_MAX_LEN - 1] = '\0';
        
        if(instruction.opcode == OP_DECLARE_INT) {
          variables[instruction.varIndex].type = VAR_INT;
        } else if(instruction.opcode == OP_DECLARE_FLOAT) {
          variables[instruction.varIndex].type = VAR_FLOAT;
        } else {
          variables[instruction.varIndex].type = VAR_BOOL;
        }
        
        int size = instruction.isArray ? instruction.arraySize : 1;
        
        variables[instruction.varIndex].isArray = instruction.isArray;
        variables[instruction.varIndex].arraySize = size;
        
        AllocateNumberStorage(instruction.varIndex, size, instruction.line);
        
        if(instruction.isArray) {
          // Every element starts as NONE; a broadcast/list initializer, if any,
          // is applied afterward through separate instructions
          for(int i = 0; i < size; i++) {
            variables[instruction.varIndex].isNone[i] = 1;
          }
        } else if(instruction.storeNone) {
          variables[instruction.varIndex].isNone[0] = 1;
        } else {
          // Scalar: pop its single initializer value
          variables[instruction.varIndex].numbers[0] = Pop();
          variables[instruction.varIndex].isNone[0] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        
        variableCount++;
        break;
      }
      case OP_DECLARE_STR: {
        strncpy(variables[instruction.varIndex].name, instruction.text, TOKEN_MAX_LEN - 1);
        variables[instruction.varIndex].name[TOKEN_MAX_LEN - 1] = '\0';
        
        variables[instruction.varIndex].type = VAR_STR;
        
        int size = instruction.isArray ? instruction.arraySize : 1;
        int strSize = instruction.strSize > 0 ? instruction.strSize : (MAX_STR_LEN - 1);
        
        variables[instruction.varIndex].isArray = instruction.isArray;
        variables[instruction.varIndex].arraySize = size;
        variables[instruction.varIndex].strSize = strSize;
        
        AllocateStringStorage(instruction.varIndex, size, strSize, instruction.line);
        
        if(instruction.isArray) {
          // Every element starts as NONE; a broadcast/list initializer, if any,
          // is applied afterward through separate instructions
          for(int i = 0; i < size; i++) {
            variables[instruction.varIndex].isNone[i] = 1;
          }
        } else if(instruction.storeNone) {
          variables[instruction.varIndex].isNone[0] = 1;
        } else {
          // Scalar: copy its single initializer string
          int srcIdx = 0;
          
          ResolveStoreSource(instruction, variables[instruction.varIndex].strings[0], strSize, &srcIdx);
          
          variables[instruction.varIndex].isNone[0] =
            (instruction.srcVarIndex != -1) ? variables[instruction.srcVarIndex].isNone[srcIdx] : 0;
        }
        
        variableCount++;
        break;
      }
      case OP_STORE_VAR:
        // Overwrite an existing scalar int, float, or bool variable
        if(instruction.storeNone) {
          variables[instruction.varIndex].isNone[0] = 1;
        } else {
          variables[instruction.varIndex].numbers[0] = Pop();
          variables[instruction.varIndex].isNone[0] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        break;
      case OP_STORE_STR: {
        if(instruction.storeNone) {
          int destIdx = 0;
          
          if(instruction.destIsArray) {
            destIdx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
          }
          
          variables[instruction.varIndex].strings[destIdx][0] = '\0';
          variables[instruction.varIndex].isNone[destIdx] = 1;
        } else {
          // Resolve the source (and pop its index, if it's an array element)
          // BEFORE popping the destination index: the source index, if any,
          // was pushed after the destination index and sits on top of it.
          // The intermediate buffer is sized to the destination's own strSize,
          // since text is truncated to fit the destination anyway.
          int maxLen = variables[instruction.varIndex].strSize;
          char* buffer = malloc(maxLen + 1);
          
          if(buffer == NULL) {
            printf("%s (%d) : Failed to allocate string buffer\n", currentFilename, instruction.line);
            exit(1);
          }
          
          int srcIdx = 0;
          
          ResolveStoreSource(instruction, buffer, maxLen, &srcIdx);
          
          int destIdx = 0;
          
          if(instruction.destIsArray) {
            destIdx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
          }
          
          strncpy(variables[instruction.varIndex].strings[destIdx], buffer, maxLen);
          variables[instruction.varIndex].strings[destIdx][maxLen] = '\0';
          
          variables[instruction.varIndex].isNone[destIdx] =
            (instruction.srcVarIndex != -1) ? variables[instruction.srcVarIndex].isNone[srcIdx] : 0;
          
          free(buffer);
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
          
          variables[instruction.varIndex].isNone[idx] = 1;
        } else {
          double value = Pop();
          int idx = CheckBounds(Pop(), variables[instruction.varIndex].arraySize, instruction.line);
          
          variables[instruction.varIndex].numbers[idx] = value;
          variables[instruction.varIndex].isNone[idx] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        break;
      }
      case OP_BROADCAST_ARR: {
        // Fill every element of the array with the same value, evaluated once.
        // Keeps bytecode size constant regardless of array size.
        double value = 0;
        int noneFlag = 1;
        
        if(!instruction.storeNone) {
          value = Pop();
          noneFlag = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        
        int size = variables[instruction.varIndex].arraySize;
        
        for(int i = 0; i < size; i++) {
          variables[instruction.varIndex].numbers[i] = value;
          variables[instruction.varIndex].isNone[i] = noneFlag;
        }
        break;
      }
      case OP_BROADCAST_STR_ARR: {
        // Fill every element of a string array with the same text, resolved once.
        int maxLen = variables[instruction.varIndex].strSize;
        char* buffer = malloc(maxLen + 1);
        int noneFlag;
        
        if(buffer == NULL) {
          printf("%s (%d) : Failed to allocate string buffer\n", currentFilename, instruction.line);
          exit(1);
        }
        
        if(instruction.storeNone) {
          buffer[0] = '\0';
          noneFlag = 1;
        } else {
          int srcIdx = 0;
          
          ResolveStoreSource(instruction, buffer, maxLen, &srcIdx);
          
          noneFlag = (instruction.srcVarIndex != -1) ? variables[instruction.srcVarIndex].isNone[srcIdx] : 0;
        }
        
        int size = variables[instruction.varIndex].arraySize;
        
        for(int i = 0; i < size; i++) {
          strncpy(variables[instruction.varIndex].strings[i], buffer, maxLen);
          variables[instruction.varIndex].strings[i][maxLen] = '\0';
          variables[instruction.varIndex].isNone[i] = noneFlag;
        }
        
        free(buffer);
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
      case OP_NOT: {
        double a = Pop();
        
        Push(a == 0 ? 1 : 0);
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
        FreeAllVariables();
        return;
    }
  }
  
  FreeAllVariables();
}
