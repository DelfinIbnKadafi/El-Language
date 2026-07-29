/*
  El Language 0.0.1 - print
*/

// include libary
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// include file
#include "lexer.h"
#include "elvm.h"

// Initial bytecode capacity
#define INITIAL_BYTECODE_CAP 256

// Bytecode storage
Instruction* bytecode;

// Current bytecode size
int bytecodeCount = 0;

// Current bytecode capacity
int bytecodeCap = 0;

// Single declared variable name
typedef struct {
  char name[TOKEN_MAX_LEN];
} Symbol;

// Symbol table storage
Symbol symbols[MAX_VARIABLES];

// Current symbol table size
int symbolCount = 0;

// Find variable index by name, or -1 if not declared
int FindSymbol(char* name) {
  for(int i = 0; i < symbolCount; i++) {
    if(strcmp(symbols[i].name, name) == 0) {
      return i;
    }
  }
  
  return -1;
}

// Register new variable, error if name already declared
int DeclareSymbol(char* name, int line) {
  if(FindSymbol(name) != -1) {
    printf("Line %d: Variable '%s' already declared\n", line, name);
    exit(1);
  }
  
  if(symbolCount >= MAX_VARIABLES) {
    printf("Line %d: Too many variables\n", line);
    exit(1);
  }
  
  strncpy(symbols[symbolCount].name, name, TOKEN_MAX_LEN - 1);
  symbols[symbolCount].name[TOKEN_MAX_LEN - 1] = '\0';
  
  return symbolCount++;
}

// Add one instruction, growing storage if needed
void EmitInstruction(Instruction instruction) {
  if(bytecodeCount >= bytecodeCap) {
    bytecodeCap *= 2;
    
    bytecode = realloc(bytecode, bytecodeCap * sizeof(Instruction));
  }
  
  bytecode[bytecodeCount++] = instruction;
}

// Load source file
char* LoadFile(char* filename) {
  FILE* file = fopen(filename, "r");
  
  if(file == NULL) {
    printf("Cannot open file\n");
    exit(1);
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  
  long size = ftell(file);
  
  rewind(file);
  
  char* buffer = malloc(size + 1);
  
  // Read source content
  fread(buffer, 1, size, file);
  
  buffer[size] = '\0';
  
  fclose(file);
  
  return buffer;
}

// Parse a single value: number literal or variable reference.
// 'token' is the already-read token to interpret as the value.
Token ParseFactor(Token token) {
  if(token.type == TOKEN_LIT_NUMBER) {
    Instruction instruction;
    
    instruction.opcode = OP_PUSH_NUMBER;
    instruction.intValue = atoi(token.value);
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      printf("Line %d: Undeclared variable '%s'\n", token.line, token.value);
      exit(1);
    }
    
    Instruction instruction;
    
    instruction.opcode = OP_PUSH_VAR;
    instruction.varIndex = index;
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  printf("Line %d: Expected value\n", token.line);
  exit(1);
}

// Parse * and / (higher precedence)
Token ParseTerm(Token token) {
  Token next = ParseFactor(token);
  
  while(next.type == TOKEN_OP_MUL || next.type == TOKEN_OP_DIV) {
    TokenType op = next.type;
    
    next = ParseFactor(LexerNext());
    
    Instruction instruction;
    
    instruction.opcode = (op == TOKEN_OP_MUL) ? OP_MUL : OP_DIV;
    
    EmitInstruction(instruction);
  }
  
  return next;
}

// Parse + and - (lower precedence)
Token ParseExpression(Token token) {
  Token next = ParseTerm(token);
  
  while(next.type == TOKEN_OP_ADD || next.type == TOKEN_OP_SUB) {
    TokenType op = next.type;
    
    next = ParseTerm(LexerNext());
    
    Instruction instruction;
    
    instruction.opcode = (op == TOKEN_OP_ADD) ? OP_ADD : OP_SUB;
    
    EmitInstruction(instruction);
  }
  
  return next;
}

// Convert source into bytecode
void Compile() {
  while(1) {
    // Get next token
    Token token = LexerNext();
    
    if(token.type == TOKEN_EOF) {
      break;
    }
    
    // Handle var declaration
    if(token.type == TOKEN_KW_VAR) {
      Token type = LexerNext();
      
      // Validate type
      if(type.type != TOKEN_TYPE_INT) {
        printf("Line %d: Expected type\n", type.line);
        exit(1);
      }
      
      Token name = LexerNext();
      
      // Validate identifier
      if(name.type != TOKEN_IDENTIFIER) {
        printf("Line %d: Expected variable name\n", name.line);
        exit(1);
      }
      
      Token next = LexerNext();
      
      // Check optional assignment
      if(next.type == TOKEN_OP_ASSIGN) {
        next = ParseExpression(LexerNext());
      } else {
        Instruction zero;
        
        zero.opcode = OP_PUSH_NUMBER;
        zero.intValue = 0;
        
        EmitInstruction(zero);
      }
      
      // Validate semicolon
      if(next.type != TOKEN_SEMICOLON) {
        printf("Line %d: Expected ;\n", next.line);
        exit(1);
      }
      
      // Register variable after evaluating expression, so it cannot reference itself
      int index = DeclareSymbol(name.value, name.line);
      
      Instruction instruction;
      
      instruction.opcode = OP_DECLARE_INT;
      instruction.varIndex = index;
      
      strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
      instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      EmitInstruction(instruction);
    }
    
    // handle exit func
    if(token.type == TOKEN_KW_EXIT) {
      Token semicolon = LexerNext();
      
      // semicolon
      if(semicolon.type != TOKEN_SEMICOLON) {
        printf("Line %d: Expected ;\n", semicolon.line);
        exit(1);
      }
      
      Instruction instruction;
      instruction.opcode = OP_HALT;
      EmitInstruction(instruction);
    }
    
    // Handle variable assignment
    if(token.type == TOKEN_IDENTIFIER) {
      int index = FindSymbol(token.value);
      
      // Validate variable exists
      if(index == -1) {
        printf("Line %d: Undeclared variable '%s'\n", token.line, token.value);
        exit(1);
      }
      
      Token assign = LexerNext();
      
      // Validate assignment operator
      if(assign.type != TOKEN_OP_ASSIGN) {
        printf("Line %d: Expected =\n", assign.line);
        exit(1);
      }
      
      Token next = ParseExpression(LexerNext());
      
      // Validate semicolon
      if(next.type != TOKEN_SEMICOLON) {
        printf("Line %d: Expected ;\n", next.line);
        exit(1);
      }
      
      Instruction instruction;
      
      instruction.opcode = OP_STORE_VAR;
      instruction.varIndex = index;
      
      EmitInstruction(instruction);
    }
    
    // Handle print statement
    if(token.type == TOKEN_KW_PRINT) {
      Token value = LexerNext();
      
      // Print string literal
      if(value.type == TOKEN_LIT_STRING) {
        Token semicolon = LexerNext();
        
        // Validate semicolon
        if(semicolon.type != TOKEN_SEMICOLON) {
          printf("Line %d: Expected ;\n", semicolon.line);
          exit(1);
        }
        
        // Create print opcode
        Instruction instruction;
        
        instruction.opcode = OP_PRINT;
        
        strncpy(
          instruction.text,
          value.value,
          INSTRUCTION_MAX_LEN - 1
        );
        
        instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
        
        EmitInstruction(instruction);
      } else {
        // Print expression result
        Token next = ParseExpression(value);
        
        // Validate semicolon
        if(next.type != TOKEN_SEMICOLON) {
          printf("Line %d: Expected ;\n", next.line);
          exit(1);
        }
        
        Instruction instruction;
        
        instruction.opcode = OP_PRINT_VALUE;
        
        EmitInstruction(instruction);
      }
    }
  }
  
  // Add program stop
  Instruction halt;
  
  halt.opcode = OP_HALT;
  
  EmitInstruction(halt);
}

int main(int argc, char** argv) {
  if(argc < 2) {
    printf("Usage: elvm <file.ell>\n");
    return 1;
  }
  
  // Load El source
  char* source = LoadFile(argv[1]);
  
  // Start lexer
  LexerInit(source);
  
  // Prepare bytecode storage
  bytecodeCap = INITIAL_BYTECODE_CAP;
  
  bytecode = malloc(bytecodeCap * sizeof(Instruction));
  
  // Compile source
  Compile();
  
  // Run VM
  VMRun(
    bytecode,
    bytecodeCount
  );
  
  // Free memory
  free(source);
  free(bytecode);
  
  return 0;
}