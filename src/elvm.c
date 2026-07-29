#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elvm.h"

#define MAX_STACK 256

Variable variables[MAX_VARIABLES];
int variableCount = 0;

// Evaluation stack, used for expression math
int stack[MAX_STACK];
int stackTop = 0;

// Push value onto evaluation stack
void Push(int value) {
  stack[stackTop++] = value;
}

// Pop value from evaluation stack
int Pop() {
  return stack[--stackTop];
}

void VMRun(Instruction* code, int count) {
  int ip = 0;
  
  while(ip < count) {
    Instruction instruction = code[ip++];
    
    switch(instruction.opcode) {
      case OP_PRINT:
        printf("%s\n", instruction.text);
        break;
      case OP_DECLARE_INT:
        // Store new variable, value comes from evaluation stack
        strncpy(variables[instruction.varIndex].name, instruction.text, TOKEN_MAX_LEN - 1);
        variables[instruction.varIndex].name[TOKEN_MAX_LEN - 1] = '\0';
        variables[instruction.varIndex].type = VAR_INT;
        variables[instruction.varIndex].value = Pop();
        variableCount++;
        break;
      case OP_STORE_VAR:
        // Overwrite existing variable value
        variables[instruction.varIndex].value = Pop();
        break;
      case OP_PUSH_NUMBER:
        Push(instruction.intValue);
        break;
      case OP_PUSH_VAR:
        Push(variables[instruction.varIndex].value);
        break;
      case OP_PRINT_VALUE:
        printf("%d\n", Pop());
        break;
      case OP_ADD: {
        int b = Pop();
        int a = Pop();
        
        Push(a + b);
        break;
      }
      case OP_SUB: {
        int b = Pop();
        int a = Pop();
        
        Push(a - b);
        break;
      }
      case OP_MUL: {
        int b = Pop();
        int a = Pop();
        
        Push(a * b);
        break;
      }
      case OP_DIV: {
        int b = Pop();
        int a = Pop();
        
        if(b == 0) {
          printf("Runtime error: division by zero\n");
          exit(1);
        }
        
        Push(a / b);
        break;
      }
      case OP_HALT:
        return;
    }
  }
}