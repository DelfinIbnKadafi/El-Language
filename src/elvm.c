#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elvm.h"

// Sized generously since recursive functions can leave several pending
// values on this stack per nesting level (e.g. 'return 1 + f(n - 1);' keeps
// its '1' here while the recursive call runs), not just one per call depth.
#define MAX_STACK 8192

Variable variables[MAX_VARIABLES];
int variableCount = 0;

// Function-local variable slots. Unlike globals (above), a local slot's
// storage is not one fixed Variable -- it's swapped in/out per call (see
// CallFrame below) so that recursive calls each get their own independent
// copy of a function's parameters and local variables. NULL until a call
// currently has that slot allocated.
Variable* localSlots[MAX_VARIABLES];

// One entry per currently-active function call (a real call stack), enabling
// correct recursion: each call gets its own storage for whichever local slots
// it declares (parameters + body locals), and the slot each one occupied
// before this call (NULL, or the paused outer call's own instance if this is
// a recursive re-entry) is restored once this call returns.
typedef struct {
  int returnAddress;
  
  int savedSlots[MAX_VARIABLES];
  Variable* savedPointers[MAX_VARIABLES];
  int savedCount;
  
  // Whether a given local slot has already been bound during this specific
  // call, so a declare instruction that re-runs (e.g. a var inside a loop
  // inside the function) reuses its own storage instead of re-registering.
  int slotRegistered[MAX_VARIABLES];
} CallFrame;

CallFrame callStack[MAX_CALL_DEPTH];
int callStackTop = 0;

// String-value stack, used to pass string arguments into a function call and
// to receive a function's string return value. A real stack (not a flat
// array indexed by position) so nested calls -- e.g. a function call used as
// the argument to another call -- nest correctly, the same way the numeric
// eval stack already does for arithmetic.
#define MAX_STRING_STACK 1024

char stringValueStack[MAX_STRING_STACK][INSTRUCTION_MAX_LEN];
int stringValueStackIsNone[MAX_STRING_STACK];
int stringValueStackTop = 0;

// Holds an array a function just returned, until whatever the caller does
// with it (assign into a real variable, index once, or ignore it) consumes
// it. A real stack, not a single slot, so a returned array used as part of
// preparing another call (or another array return) nests correctly, the same
// reasoning as stringValueStack above. Each entry owns its own storage and
// is always freed by whichever opcode consumes it.
#define MAX_RETURNED_ARR_STACK 64

Variable* returnedArrStack[MAX_RETURNED_ARR_STACK];
int returnedArrStackTop = 0;

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
  if(stackTop >= MAX_STACK) {
    printf("%s : Stack overflow: expression nesting or recursion too deep\n", currentFilename);
    exit(1);
  }
  
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

// Resolve a variable reference to its physical storage: the global table for
// a global variable, or the currently-active call's own slot for a local one.
Variable* ResolveVariable(int varIndex, int isLocal) {
  if(isLocal) {
    return localSlots[varIndex];
  }
  
  return &variables[varIndex];
}

// Frees whatever storage this variable slot currently holds (if any) and
// resets it to a clean, unallocated state. MUST be called before an
// OP_DECLARE_* instruction overwrites the slot's type/arraySize/etc, since it
// relies on those fields still holding the slot's PREVIOUS declaration to
// free correctly. This matters once block scoping is involved: sibling
// blocks can let two unrelated variables (different name, possibly different
// type or array size) share the same physical slot, and loop bodies can
// re-run the same declare instruction on every iteration.
void FreeVariableStorage(Variable* v) {
  free(v->numbers);
  v->numbers = NULL;
  
  if(v->strings != NULL) {
    for(int i = 0; i < v->arraySize; i++) {
      free(v->strings[i]);
    }
    
    free(v->strings);
    v->strings = NULL;
  }
  
  free(v->isNone);
  v->isNone = NULL;
}

void AllocateNumberStorage(Variable* v, int size, int line) {
  CheckAllocationSize(size, sizeof(double) + sizeof(int), line);
  
  v->numbers = calloc(size, sizeof(double));
  v->isNone = malloc(size * sizeof(int));
  
  if(v->numbers == NULL || v->isNone == NULL) {
    printf("%s (%d) : Failed to allocate array of size %d\n", currentFilename, line, size);
    exit(1);
  }
}

// Allocate the string storage for a variable holding 'size' elements, each up
// to 'strSize' characters, stopping the program with a clear error if memory
// could not be obtained.
void AllocateStringStorage(Variable* v, int size, int strSize, int line) {
  CheckAllocationSize(size, strSize + 1 + sizeof(char*) + sizeof(int), line);
  
  v->strings = malloc(size * sizeof(char*));
  v->isNone = malloc(size * sizeof(int));
  
  if(v->strings == NULL || v->isNone == NULL) {
    printf("%s (%d) : Failed to allocate array of size %d\n", currentFilename, line, size);
    exit(1);
  }
  
  for(int i = 0; i < size; i++) {
    v->strings[i] = calloc(strSize + 1, sizeof(char));
    
    if(v->strings[i] == NULL) {
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

// Resolve the string to copy for OP_DECLARE_STR / OP_STORE_STR / OP_BROADCAST_STR_ARR
// into 'out', and write whether it's NONE into 'outIsNone'. Handles three source
// kinds: a value popped from the string-value stack (used to bind a string
// parameter, or to receive a function call's string return value), a literal,
// or an existing variable (whole, or one array element -- index popped from
// the stack when srcIsArray is set). Must be called before popping any
// destination index: a variable source's array index, if any, was pushed
// after it and therefore sits on top of the stack.
void ResolveStoreSource(Instruction instruction, char* out, int outSize, int* outIsNone) {
  if(instruction.sourceFromArgStack) {
    stringValueStackTop--;
    
    strncpy(out, stringValueStack[stringValueStackTop], outSize);
    out[outSize] = '\0';
    
    *outIsNone = stringValueStackIsNone[stringValueStackTop];
    return;
  }
  
  if(instruction.sourceFromReturnedArrIndex) {
    returnedArrStackTop--;
    Variable* src = returnedArrStack[returnedArrStackTop];
    
    int idx = CheckBounds(Pop(), src->arraySize, instruction.line);
    
    strncpy(out, src->strings[idx], outSize);
    out[outSize] = '\0';
    
    *outIsNone = src->isNone[idx];
    
    FreeVariableStorage(src);
    free(src);
    return;
  }
  
  if(instruction.srcVarIndex == -1) {
    strncpy(out, instruction.stringLiteral, outSize);
    out[outSize] = '\0';
    
    *outIsNone = 0;
    return;
  }
  
  Variable* srcV = ResolveVariable(instruction.srcVarIndex, instruction.srcIsLocal);
  int srcIdx = 0;
  
  if(instruction.srcIsArray) {
    srcIdx = CheckBounds(Pop(), srcV->arraySize, instruction.line);
  }
  
  strncpy(out, srcV->strings[srcIdx], outSize);
  out[outSize] = '\0';
  
  *outIsNone = srcV->isNone[srcIdx];
}

// Enter a new function call: remember where to resume once it returns.
// Local-slot storage is bound lazily as each parameter/var declare
// instruction actually executes (see OP_DECLARE_* below), not here.
void PushCallFrame(int returnAddress, int line) {
  if(callStackTop >= MAX_CALL_DEPTH) {
    printf("%s (%d) : Stack overflow: too many nested/recursive function calls\n", currentFilename, line);
    exit(1);
  }
  
  CallFrame* frame = &callStack[callStackTop++];
  
  frame->returnAddress = returnAddress;
  frame->savedCount = 0;
  
  for(int i = 0; i < MAX_VARIABLES; i++) {
    frame->slotRegistered[i] = 0;
  }
}

// Leave the current function call: free every local slot this specific call
// bound, and restore whatever occupied that slot before this call (NULL, or
// the paused outer call's own instance if this was a recursive re-entry).
// Returns the address execution should resume at.
int PopCallFrame() {
  CallFrame* frame = &callStack[--callStackTop];
  
  for(int i = 0; i < frame->savedCount; i++) {
    int slot = frame->savedSlots[i];
    
    FreeVariableStorage(localSlots[slot]);
    free(localSlots[slot]);
    
    localSlots[slot] = frame->savedPointers[i];
  }
  
  return frame->returnAddress;
}

// Bind a local slot for use during the currently-active call: the first time
// this slot is touched during this specific call, stash whatever occupied it
// before (for restoration on return) and start it fresh; a later re-run of
// the same declare instruction (e.g. a var inside a loop inside the
// function) reuses the storage already bound earlier in this same call.
// Returns the slot's live storage, ready to be (re)populated by the caller.
Variable* BindLocalSlot(int varIndex) {
  CallFrame* frame = &callStack[callStackTop - 1];
  
  if(!frame->slotRegistered[varIndex]) {
    frame->savedSlots[frame->savedCount] = varIndex;
    frame->savedPointers[frame->savedCount] = localSlots[varIndex];
    frame->savedCount++;
    frame->slotRegistered[varIndex] = 1;
    
    localSlots[varIndex] = calloc(1, sizeof(Variable));
  }
  
  return localSlots[varIndex];
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
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        int idx = 0;
        
        if(instruction.destIsArray) {
          idx = CheckBounds(Pop(), v->arraySize, instruction.line);
        }
        
        if(v->isNone[idx]) {
          printf("NONE\n");
        } else {
          printf("%s\n", v->strings[idx]);
        }
        break;
      }
      case OP_DECLARE_INT:
      case OP_DECLARE_FLOAT:
      case OP_DECLARE_BOOL: {
        Variable* v = instruction.isLocal ? BindLocalSlot(instruction.varIndex) : &variables[instruction.varIndex];
        
        FreeVariableStorage(v);
        
        strncpy(v->name, instruction.text, TOKEN_MAX_LEN - 1);
        v->name[TOKEN_MAX_LEN - 1] = '\0';
        
        if(instruction.opcode == OP_DECLARE_INT) {
          v->type = VAR_INT;
        } else if(instruction.opcode == OP_DECLARE_FLOAT) {
          v->type = VAR_FLOAT;
        } else {
          v->type = VAR_BOOL;
        }
        
        int size = instruction.isArray ? instruction.arraySize : 1;
        
        v->isArray = instruction.isArray;
        v->arraySize = size;
        
        AllocateNumberStorage(v, size, instruction.line);
        
        if(instruction.isArray) {
          if(instruction.arrayBindMode) {
            // Parameter bind: caller has already pushed exactly one value
            // per element, in order (a broadcast-style call expands to N
            // identical pushes on the caller's side -- see
            // ParseFunctionCallArgs). Pop them back out in reverse so
            // element 0 ends up with the first value that was pushed.
            for(int i = size - 1; i >= 0; i--) {
              v->numbers[i] = Pop();
              v->isNone[i] = lastPushedIsNone;
            }
          } else {
            // Every element starts as NONE; a broadcast/list initializer, if
            // any, is applied afterward through separate instructions
            for(int i = 0; i < size; i++) {
              v->isNone[i] = 1;
            }
          }
        } else if(instruction.storeNone) {
          v->isNone[0] = 1;
        } else {
          // Scalar: pop its single initializer value (also used to bind a
          // numeric parameter -- the caller already pushed the argument
          // value before jumping in)
          v->numbers[0] = Pop();
          v->isNone[0] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        
        variableCount++;
        break;
      }
      case OP_DECLARE_STR: {
        Variable* v = instruction.isLocal ? BindLocalSlot(instruction.varIndex) : &variables[instruction.varIndex];
        
        FreeVariableStorage(v);
        
        strncpy(v->name, instruction.text, TOKEN_MAX_LEN - 1);
        v->name[TOKEN_MAX_LEN - 1] = '\0';
        
        v->type = VAR_STR;
        
        int size = instruction.isArray ? instruction.arraySize : 1;
        int strSize = instruction.strSize > 0 ? instruction.strSize : (MAX_STR_LEN - 1);
        
        v->isArray = instruction.isArray;
        v->arraySize = size;
        v->strSize = strSize;
        
        AllocateStringStorage(v, size, strSize, instruction.line);
        
        if(instruction.isArray) {
          if(instruction.arrayBindMode) {
            // Parameter bind: caller has already pushed exactly one string
            // per element, in order (a broadcast-style call expands to N
            // identical pushes on the caller's side). Pop them back out in
            // reverse so element 0 ends up with the first one pushed.
            for(int i = size - 1; i >= 0; i--) {
              stringValueStackTop--;
              
              strncpy(v->strings[i], stringValueStack[stringValueStackTop], strSize);
              v->strings[i][strSize] = '\0';
              v->isNone[i] = stringValueStackIsNone[stringValueStackTop];
            }
          } else {
            // Every element starts as NONE; a broadcast/list initializer, if
            // any, is applied afterward through separate instructions
            for(int i = 0; i < size; i++) {
              v->isNone[i] = 1;
            }
          }
        } else if(instruction.storeNone) {
          v->isNone[0] = 1;
        } else {
          // Scalar: copy its single initializer string (also used to bind a
          // string parameter, or receive a function's string return value,
          // via sourceFromArgStack inside ResolveStoreSource)
          int isNone = 0;
          
          ResolveStoreSource(instruction, v->strings[0], strSize, &isNone);
          
          v->isNone[0] = isNone;
        }
        
        variableCount++;
        break;
      }
      case OP_STORE_VAR: {
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        // Overwrite an existing scalar int, float, or bool variable
        if(instruction.storeNone) {
          v->isNone[0] = 1;
        } else {
          v->numbers[0] = Pop();
          v->isNone[0] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        break;
      }
      case OP_STORE_STR: {
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        if(instruction.storeNone) {
          int destIdx = 0;
          
          if(instruction.destIsArray) {
            destIdx = CheckBounds(Pop(), v->arraySize, instruction.line);
          }
          
          v->strings[destIdx][0] = '\0';
          v->isNone[destIdx] = 1;
        } else {
          // Resolve the source (and pop its index, if it's an array element)
          // BEFORE popping the destination index: the source index, if any,
          // was pushed after the destination index and sits on top of it.
          // The intermediate buffer is sized to the destination's own strSize,
          // since text is truncated to fit the destination anyway.
          int maxLen = v->strSize;
          char* buffer = malloc(maxLen + 1);
          
          if(buffer == NULL) {
            printf("%s (%d) : Failed to allocate string buffer\n", currentFilename, instruction.line);
            exit(1);
          }
          
          int isNone = 0;
          
          ResolveStoreSource(instruction, buffer, maxLen, &isNone);
          
          int destIdx = 0;
          
          if(instruction.destIsArray) {
            destIdx = CheckBounds(Pop(), v->arraySize, instruction.line);
          }
          
          strncpy(v->strings[destIdx], buffer, maxLen);
          v->strings[destIdx][maxLen] = '\0';
          
          v->isNone[destIdx] = isNone;
          
          free(buffer);
        }
        break;
      }
      case OP_PUSH_NUMBER:
        Push(instruction.numberValue);
        lastPushedIsNone = instruction.storeNone;
        break;
      case OP_PUSH_VAR: {
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        Push(v->numbers[0]);
        lastPushedIsNone = v->isNone[0];
        break;
      }
      case OP_PUSH_ARR: {
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        int idx = CheckBounds(Pop(), v->arraySize, instruction.line);
        
        Push(v->numbers[idx]);
        lastPushedIsNone = v->isNone[idx];
        break;
      }
      case OP_STORE_ARR: {
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        if(instruction.storeNone) {
          int idx = CheckBounds(Pop(), v->arraySize, instruction.line);
          
          v->isNone[idx] = 1;
        } else {
          double value = Pop();
          int idx = CheckBounds(Pop(), v->arraySize, instruction.line);
          
          v->numbers[idx] = value;
          v->isNone[idx] = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        break;
      }
      case OP_BROADCAST_ARR: {
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        // Fill every element of the array with the same value, evaluated once.
        // Keeps bytecode size constant regardless of array size.
        double value = 0;
        int noneFlag = 1;
        
        if(!instruction.storeNone) {
          value = Pop();
          noneFlag = instruction.propagateNone ? lastPushedIsNone : 0;
        }
        
        int size = v->arraySize;
        
        for(int i = 0; i < size; i++) {
          v->numbers[i] = value;
          v->isNone[i] = noneFlag;
        }
        break;
      }
      case OP_BROADCAST_STR_ARR: {
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        // Fill every element of a string array with the same text, resolved once.
        int maxLen = v->strSize;
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
          ResolveStoreSource(instruction, buffer, maxLen, &noneFlag);
        }
        
        int size = v->arraySize;
        
        for(int i = 0; i < size; i++) {
          strncpy(v->strings[i], buffer, maxLen);
          v->strings[i][maxLen] = '\0';
          v->isNone[i] = noneFlag;
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
      case OP_DUP: {
        // Duplicate the top of the eval stack without consuming it -- used
        // to replicate a once-evaluated broadcast value across every slot
        // of an array parameter, without re-evaluating the expression.
        double top = stack[stackTop - 1];
        int wasNone = lastPushedIsNone;
        
        Push(top);
        lastPushedIsNone = wasNone;
        break;
      }
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
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        int idx = 0;
        
        if(instruction.destIsArray) {
          idx = CheckBounds(Pop(), v->arraySize, instruction.line);
        }
        
        int isNone = v->isNone[idx];
        
        Push((instruction.opcode == OP_STR_IS_NONE ? isNone : !isNone) ? 1 : 0);
        break;
      }
      case OP_CALL:
        PushCallFrame(ip, instruction.line);
        ip = instruction.jumpTarget;
        break;
      case OP_RETURN:
        ip = PopCallFrame();
        break;
      case OP_RETURN_ARR:
      case OP_RETURN_STR_ARR: {
        // Copy the local array being returned into its own independent
        // buffer BEFORE the call frame is torn down (the local's own storage
        // gets freed as part of that), then hand the buffer to the caller
        // through returnedArrStack. Whoever consumes it on the caller side
        // is responsible for freeing it.
        if(returnedArrStackTop >= MAX_RETURNED_ARR_STACK) {
          printf("%s (%d) : Stack overflow: too many nested array returns\n", currentFilename, instruction.line);
          exit(1);
        }
        
        Variable* src = ResolveVariable(instruction.varIndex, instruction.isLocal);
        Variable* buffer = calloc(1, sizeof(Variable));
        
        buffer->type = src->type;
        buffer->arraySize = src->arraySize;
        buffer->strSize = src->strSize;
        
        if(instruction.opcode == OP_RETURN_ARR) {
          AllocateNumberStorage(buffer, src->arraySize, instruction.line);
          
          for(int i = 0; i < src->arraySize; i++) {
            buffer->numbers[i] = src->numbers[i];
            buffer->isNone[i] = src->isNone[i];
          }
        } else {
          AllocateStringStorage(buffer, src->arraySize, src->strSize, instruction.line);
          
          for(int i = 0; i < src->arraySize; i++) {
            strncpy(buffer->strings[i], src->strings[i], src->strSize);
            buffer->strings[i][src->strSize] = '\0';
            buffer->isNone[i] = src->isNone[i];
          }
        }
        
        returnedArrStack[returnedArrStackTop++] = buffer;
        
        ip = PopCallFrame();
        break;
      }
      case OP_CAPTURE_RETURNED_ARR:
      case OP_CAPTURE_RETURNED_STR_ARR: {
        // Assigning a call's array return into a real variable: adopt the
        // returned buffer's contents into the destination (which has already
        // been freshly allocated by a preceding OP_DECLARE_*), then free the
        // now-unneeded temporary buffer.
        Variable* dest = instruction.isLocal ? BindLocalSlot(instruction.varIndex) : &variables[instruction.varIndex];
        
        returnedArrStackTop--;
        Variable* src = returnedArrStack[returnedArrStackTop];
        
        for(int i = 0; i < dest->arraySize; i++) {
          dest->isNone[i] = src->isNone[i];
          
          if(instruction.opcode == OP_CAPTURE_RETURNED_ARR) {
            dest->numbers[i] = src->numbers[i];
          } else {
            strncpy(dest->strings[i], src->strings[i], dest->strSize);
            dest->strings[i][dest->strSize] = '\0';
          }
        }
        
        FreeVariableStorage(src);
        free(src);
        break;
      }
      case OP_INDEX_RETURNED_ARR: {
        // A call's array return, indexed immediately (e.g. buildArr()[0])
        // without ever being stored in a named variable: pull out just the
        // requested element, then the whole temporary buffer is discarded.
        returnedArrStackTop--;
        Variable* src = returnedArrStack[returnedArrStackTop];
        
        int idx = CheckBounds(Pop(), src->arraySize, instruction.line);
        
        Push(src->numbers[idx]);
        lastPushedIsNone = src->isNone[idx];
        
        FreeVariableStorage(src);
        free(src);
        break;
      }
      case OP_INDEX_RETURNED_STR_ARR: {
        returnedArrStackTop--;
        Variable* src = returnedArrStack[returnedArrStackTop];
        
        int idx = CheckBounds(Pop(), src->arraySize, instruction.line);
        
        if(stringValueStackTop >= MAX_STRING_STACK) {
          printf("%s (%d) : Stack overflow: too many nested string arguments/returns\n", currentFilename, instruction.line);
          exit(1);
        }
        
        strncpy(stringValueStack[stringValueStackTop], src->strings[idx], INSTRUCTION_MAX_LEN - 1);
        stringValueStack[stringValueStackTop][INSTRUCTION_MAX_LEN - 1] = '\0';
        stringValueStackIsNone[stringValueStackTop] = src->isNone[idx];
        stringValueStackTop++;
        
        FreeVariableStorage(src);
        free(src);
        break;
      }
      case OP_DISCARD_RETURNED_ARR: {
        // The array return was used as a bare statement, or otherwise never
        // consumed -- just free it.
        returnedArrStackTop--;
        Variable* src = returnedArrStack[returnedArrStackTop];
        
        FreeVariableStorage(src);
        free(src);
        break;
      }
      case OP_PUSH_STRING_VALUE: {
        if(stringValueStackTop >= MAX_STRING_STACK) {
          printf("%s (%d) : Stack overflow: too many nested string arguments/returns\n", currentFilename, instruction.line);
          exit(1);
        }
        
        if(instruction.storeNone) {
          stringValueStack[stringValueStackTop][0] = '\0';
          stringValueStackIsNone[stringValueStackTop] = 1;
        } else {
          int isNone = 0;
          
          ResolveStoreSource(instruction, stringValueStack[stringValueStackTop], INSTRUCTION_MAX_LEN - 1, &isNone);
          stringValueStackIsNone[stringValueStackTop] = isNone;
        }
        
        stringValueStackTop++;
        break;
      }
      case OP_POP_STRING_VALUE:
        stringValueStackTop--;
        break;
      case OP_DUP_STRING_VALUE: {
        // Same idea as OP_DUP, but for the string-value stack: replicate a
        // once-evaluated broadcast string across every slot of a string
        // array parameter, without re-evaluating (and possibly re-running
        // a function call) for every element.
        if(stringValueStackTop >= MAX_STRING_STACK) {
          printf("%s (%d) : Stack overflow: too many nested string arguments/returns\n", currentFilename, instruction.line);
          exit(1);
        }
        
        strncpy(stringValueStack[stringValueStackTop], stringValueStack[stringValueStackTop - 1], INSTRUCTION_MAX_LEN - 1);
        stringValueStack[stringValueStackTop][INSTRUCTION_MAX_LEN - 1] = '\0';
        stringValueStackIsNone[stringValueStackTop] = stringValueStackIsNone[stringValueStackTop - 1];
        
        stringValueStackTop++;
        break;
      }
      case OP_PUSH_ARR_ELEMENT_TO_STAGE: {
        // Push one specific element of an existing array variable onto the
        // eval stack, used when a caller passes an existing array as an
        // argument (each element is staged this way, in order).
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        Push(v->numbers[instruction.elementIndex]);
        lastPushedIsNone = v->isNone[instruction.elementIndex];
        break;
      }
      case OP_PUSH_STR_ARR_ELEMENT_TO_STAGE: {
        // Same as above, for a string array's element, staged onto the
        // string-value stack instead.
        Variable* v = ResolveVariable(instruction.varIndex, instruction.isLocal);
        
        if(stringValueStackTop >= MAX_STRING_STACK) {
          printf("%s (%d) : Stack overflow: too many nested string arguments/returns\n", currentFilename, instruction.line);
          exit(1);
        }
        
        strncpy(stringValueStack[stringValueStackTop], v->strings[instruction.elementIndex], INSTRUCTION_MAX_LEN - 1);
        stringValueStack[stringValueStackTop][INSTRUCTION_MAX_LEN - 1] = '\0';
        stringValueStackIsNone[stringValueStackTop] = v->isNone[instruction.elementIndex];
        
        stringValueStackTop++;
        break;
      }
      case OP_PRINT_STRING_VALUE: {
        stringValueStackTop--;
        
        if(stringValueStackIsNone[stringValueStackTop]) {
          printf("NONE\n");
        } else {
          printf("%s\n", stringValueStack[stringValueStackTop]);
        }
        break;
      }
      case OP_HALT:
        FreeAllVariables();
        return;
    }
  }
  
  FreeAllVariables();
}
