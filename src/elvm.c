#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elvm.h"

#define MAX_STACK 256

Variable variables[MAX_VARIABLES];
int variableCount = 0;

// Evaluation stack, used for numeric expression math (int, float, bool)
double stack[MAX_STACK];
int stackTop = 0;

// Push value onto evaluation stack
void Push(double value) {
  stack[stackTop++] = value;
}

// Pop value from evaluation stack
double Pop() {
  return stack[--stackTop];
}

// Copy string literal or source variable text into destination variable
void StoreString(Instruction instruction) {
  char* text;
  
  if(instruction.srcVarIndex != -1) {
    text = variables[instruction.srcVarIndex].stringValue;
  } else {
    text = instruction.stringLiteral;
  }
  
  strncpy(variables[instruction.varIndex].stringValue, text, INSTRUCTION_MAX_LEN - 1);
  variables[instruction.varIndex].stringValue[INSTRUCTION_MAX_LEN - 1] = '\0';
}

void VMRun(Instruction* code, int count, char* filename) {
  int ip = 0;
  
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
      case OP_PRINT_STR_VAR:
        printf("%s\n", variables[instruction.varIndex].stringValue);
        break;
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
        
        variables[instruction.varIndex].numberValue = Pop();
        variableCount++;
        break;
      case OP_DECLARE_STR:
        strncpy(variables[instruction.varIndex].name, instruction.text, TOKEN_MAX_LEN - 1);
        variables[instruction.varIndex].name[TOKEN_MAX_LEN - 1] = '\0';
        
        variables[instruction.varIndex].type = VAR_STR;
        
        StoreString(instruction);
        
        variableCount++;
        break;
      case OP_STORE_VAR:
        // Overwrite existing int, float, or bool variable value
        variables[instruction.varIndex].numberValue = Pop();
        break;
      case OP_STORE_STR:
        // Overwrite existing string variable value
        StoreString(instruction);
        break;
      case OP_PUSH_NUMBER:
        Push(instruction.numberValue);
        break;
      case OP_PUSH_VAR:
        Push(variables[instruction.varIndex].numberValue);
        break;
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
      case OP_HALT:
        return;
    }
  }
}
