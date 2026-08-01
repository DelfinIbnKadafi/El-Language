/*
  El Language 0.0.1 Alpha - print
  El Language 0.0.2 Alpha - Variable (int, str, bool, float) and math
  El Language 0.1.0 - if / else if / else, comparison, and increment/decrement
*/

// include libary
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

// Current source filename, used for error messages
char* sourceFilename;

// Print a compile error as "file (line) : message", then stop. Never returns.
void CompileError(int line, char* format, ...) __attribute__((noreturn));

void CompileError(int line, char* format, ...) {
  printf("%s (%d) : ", sourceFilename, line);
  
  va_list args;
  
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  
  printf("\n");
  
  exit(1);
}

// Single declared variable name and type
typedef struct {
  char name[TOKEN_MAX_LEN];
  
  VarType type;
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
int DeclareSymbol(char* name, VarType type, int line) {
  if(FindSymbol(name) != -1) {
    CompileError(line, "Variable '%s' already declared", name);
  }
  
  if(symbolCount >= MAX_VARIABLES) {
    CompileError(line, "Too many variables");
  }
  
  strncpy(symbols[symbolCount].name, name, TOKEN_MAX_LEN - 1);
  symbols[symbolCount].name[TOKEN_MAX_LEN - 1] = '\0';
  
  symbols[symbolCount].type = type;
  
  return symbolCount++;
}

// Add one instruction, growing storage if needed
int EmitInstruction(Instruction instruction) {
  if(bytecodeCount >= bytecodeCap) {
    bytecodeCap *= 2;
    
    bytecode = realloc(bytecode, bytecodeCap * sizeof(Instruction));
  }
  
  bytecode[bytecodeCount] = instruction;
  
  return bytecodeCount++;
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

// Result of parsing a numeric expression: lookahead token and resulting type
typedef struct {
  Token next;
  
  VarType type;
} ExprResult;

// Result of parsing a string value: lookahead token, literal text, and source variable
typedef struct {
  Token next;
  
  char literal[INSTRUCTION_MAX_LEN];
  
  int srcVarIndex;
} StringResult;

// Result of parsing a condition: lookahead token and resulting type (always bool once combined)
typedef struct {
  Token next;
  
  VarType type;
} CondResult;

// Forward declarations, needed because parentheses make these mutually recursive
ExprResult ParseNumericExpression(Token token);
CondResult ParseOrExpr(Token token);
Token ParseStatement(Token token);
Token ParseIfStatement(int chainColumn);

// Parse a single numeric value: literal, variable, or parenthesized expression.
// 'token' is the already-read token to interpret as the value.
ExprResult ParseNumericFactor(Token token) {
  ExprResult result = {0};
  
  // Parenthesized sub-expression
  if(token.type == TOKEN_LPAREN) {
    ExprResult inner = ParseNumericExpression(LexerNext());
    
    if(inner.next.type != TOKEN_RPAREN) {
      CompileError(inner.next.line, "Expected )");
    }
    
    inner.next = LexerNext();
    
    return inner;
  }
  
  // Int literal
  if(token.type == TOKEN_LIT_NUMBER) {
    Instruction instruction = {0};
    
    instruction.opcode = OP_PUSH_NUMBER;
    instruction.numberValue = atoi(token.value);
    
    EmitInstruction(instruction);
    
    result.next = LexerNext();
    result.type = VAR_INT;
    
    return result;
  }
  
  // Float literal
  if(token.type == TOKEN_LIT_FLOAT) {
    Instruction instruction = {0};
    
    instruction.opcode = OP_PUSH_NUMBER;
    instruction.numberValue = atof(token.value);
    
    EmitInstruction(instruction);
    
    result.next = LexerNext();
    result.type = VAR_FLOAT;
    
    return result;
  }
  
  // Int or float variable reference
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    VarType type = symbols[index].type;
    
    if(type != VAR_INT && type != VAR_FLOAT) {
      CompileError(token.line, "Variable '%s' cannot be used in a math expression", token.value);
    }
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_PUSH_VAR;
    instruction.varIndex = index;
    
    EmitInstruction(instruction);
    
    result.next = LexerNext();
    result.type = type;
    
    return result;
  }
  
  CompileError(token.line, "Expected value");
}

// Parse * and / (higher precedence)
ExprResult ParseNumericTerm(Token token) {
  ExprResult left = ParseNumericFactor(token);
  
  while(left.next.type == TOKEN_OP_MUL || left.next.type == TOKEN_OP_DIV) {
    TokenType op = left.next.type;
    int opLine = left.next.line;
    
    ExprResult right = ParseNumericFactor(LexerNext());
    
    Instruction instruction = {0};
    
    if(op == TOKEN_OP_MUL) {
      instruction.opcode = OP_MUL;
    } else if(left.type == VAR_INT && right.type == VAR_INT) {
      // Both operands are int, keep integer division semantics
      instruction.opcode = OP_IDIV;
    } else {
      instruction.opcode = OP_DIV;
    }
    
    instruction.line = opLine;
    
    EmitInstruction(instruction);
    
    left.type = (left.type == VAR_FLOAT || right.type == VAR_FLOAT) ? VAR_FLOAT : VAR_INT;
    left.next = right.next;
  }
  
  return left;
}

// Parse + and - (lower precedence)
ExprResult ParseNumericExpression(Token token) {
  ExprResult left = ParseNumericTerm(token);
  
  while(left.next.type == TOKEN_OP_ADD || left.next.type == TOKEN_OP_SUB) {
    TokenType op = left.next.type;
    
    ExprResult right = ParseNumericTerm(LexerNext());
    
    Instruction instruction = {0};
    
    instruction.opcode = (op == TOKEN_OP_ADD) ? OP_ADD : OP_SUB;
    
    EmitInstruction(instruction);
    
    left.type = (left.type == VAR_FLOAT || right.type == VAR_FLOAT) ? VAR_FLOAT : VAR_INT;
    left.next = right.next;
  }
  
  return left;
}

// Parse a single string value: string literal or existing string variable (no concatenation)
StringResult ParseStringValue(Token token) {
  StringResult result = {0};
  
  if(token.type == TOKEN_LIT_STRING) {
    strncpy(result.literal, token.value, INSTRUCTION_MAX_LEN - 1);
    result.literal[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    result.srcVarIndex = -1;
    result.next = LexerNext();
    
    return result;
  }
  
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    if(symbols[index].type != VAR_STR) {
      CompileError(token.line, "Variable '%s' is not a string", token.value);
    }
    
    result.srcVarIndex = index;
    result.next = LexerNext();
    
    return result;
  }
  
  CompileError(token.line, "Expected string value");
}

// Parse a single bool value: true/false literal or bool variable reference (no math allowed)
Token ParseBoolValue(Token token) {
  if(token.type == TOKEN_LIT_BOOL) {
    Instruction instruction = {0};
    
    instruction.opcode = OP_PUSH_NUMBER;
    instruction.numberValue = atoi(token.value);
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    if(symbols[index].type != VAR_BOOL) {
      CompileError(token.line, "Variable '%s' is not a bool", token.value);
    }
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_PUSH_VAR;
    instruction.varIndex = index;
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  CompileError(token.line, "Expected bool value");
}

// Parse a single condition operand: parenthesized condition, bool value, or numeric expression.
// A parenthesized group is parsed as a full condition, so it transparently supports both
// arithmetic grouping (e.g. (a + b) > 5) and logical grouping (e.g. (a > 1) and (b > 2)).
CondResult ParseConditionOperand(Token token) {
  CondResult result = {0};
  
  if(token.type == TOKEN_LPAREN) {
    CondResult inner = ParseOrExpr(LexerNext());
    
    if(inner.next.type != TOKEN_RPAREN) {
      CompileError(inner.next.line, "Expected )");
    }
    
    inner.next = LexerNext();
    
    return inner;
  }
  
  if(token.type == TOKEN_LIT_BOOL) {
    Instruction instruction = {0};
    
    instruction.opcode = OP_PUSH_NUMBER;
    instruction.numberValue = atoi(token.value);
    
    EmitInstruction(instruction);
    
    result.type = VAR_BOOL;
    result.next = LexerNext();
    
    return result;
  }
  
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    if(index != -1 && symbols[index].type == VAR_BOOL) {
      Instruction instruction = {0};
      
      instruction.opcode = OP_PUSH_VAR;
      instruction.varIndex = index;
      
      EmitInstruction(instruction);
      
      result.type = VAR_BOOL;
      result.next = LexerNext();
      
      return result;
    }
  }
  
  // Fall back to a numeric expression (int/float literals, variables, and arithmetic parens)
  ExprResult numResult = ParseNumericExpression(token);
  
  result.type = numResult.type;
  result.next = numResult.next;
  
  return result;
}

// Parse an optional comparison: operand (comparison_op operand)?
CondResult ParseComparison(Token token) {
  CondResult left = ParseConditionOperand(token);
  
  TokenType op = left.next.type;
  
  if(op == TOKEN_OP_GT || op == TOKEN_OP_LT || op == TOKEN_OP_EQ ||
     op == TOKEN_OP_GE || op == TOKEN_OP_LE || op == TOKEN_OP_NE) {
    int opLine = left.next.line;
    
    CondResult right = ParseConditionOperand(LexerNext());
    
    Instruction instruction = {0};
    
    if(op == TOKEN_OP_GT) {
      instruction.opcode = OP_CMP_GT;
    } else if(op == TOKEN_OP_LT) {
      instruction.opcode = OP_CMP_LT;
    } else if(op == TOKEN_OP_EQ) {
      instruction.opcode = OP_CMP_EQ;
    } else if(op == TOKEN_OP_GE) {
      instruction.opcode = OP_CMP_GE;
    } else if(op == TOKEN_OP_LE) {
      instruction.opcode = OP_CMP_LE;
    } else {
      instruction.opcode = OP_CMP_NE;
    }
    
    instruction.line = opLine;
    
    EmitInstruction(instruction);
    
    left.type = VAR_BOOL;
    left.next = right.next;
  }
  
  return left;
}

// Parse 'and' chains (higher precedence than 'or')
CondResult ParseAndExpr(Token token) {
  CondResult left = ParseComparison(token);
  
  while(left.next.type == TOKEN_KW_AND) {
    CondResult right = ParseComparison(LexerNext());
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_AND;
    
    EmitInstruction(instruction);
    
    left.type = VAR_BOOL;
    left.next = right.next;
  }
  
  return left;
}

// Parse 'or' chains (lower precedence than 'and')
CondResult ParseOrExpr(Token token) {
  CondResult left = ParseAndExpr(token);
  
  while(left.next.type == TOKEN_KW_OR) {
    CondResult right = ParseAndExpr(LexerNext());
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_OR;
    
    EmitInstruction(instruction);
    
    left.type = VAR_BOOL;
    left.next = right.next;
  }
  
  return left;
}

// Parse an indented block of statements. 'token' is the first token of the block.
// 'parentColumn' is the column of the if/else keyword that owns this block; the
// block's own indentation is inferred from 'token' and must be deeper than that.
// Returns the lookahead token that follows the block (first token at or below its indentation).
Token ParseBlock(Token token, int parentColumn) {
  if(token.column <= parentColumn) {
    CompileError(token.line, "Expected indented block");
  }
  
  int blockColumn = token.column;
  
  token = ParseStatement(token);
  
  while(token.column == blockColumn && token.type != TOKEN_EOF) {
    token = ParseStatement(token);
  }
  
  return token;
}

// Parse an if statement, including any chained 'else if' / trailing 'else'.
// 'chainColumn' is the column of the very first 'if' in the chain; a following
// 'else' only belongs to this chain if its column matches exactly.
Token ParseIfStatement(int chainColumn) {
  Token lparen = LexerNext();
  
  if(lparen.type != TOKEN_LPAREN) {
    CompileError(lparen.line, "Expected (");
  }
  
  CondResult condition = ParseOrExpr(LexerNext());
  
  if(condition.next.type != TOKEN_RPAREN) {
    CompileError(condition.next.line, "Expected )");
  }
  
  Token assign = LexerNext();
  
  if(assign.type != TOKEN_OP_ASSIGN) {
    CompileError(assign.line, "Expected =");
  }
  
  // Placeholder jump, patched below once we know where the false branch starts
  Instruction jumpIfFalse = {0};
  
  jumpIfFalse.opcode = OP_JUMP_IF_FALSE;
  
  int jumpIfFalseIndex = EmitInstruction(jumpIfFalse);
  
  Token afterBlock = ParseBlock(LexerNext(), chainColumn);
  
  // Check for a matching else / else if at the same column as this chain
  if(afterBlock.type == TOKEN_KW_ELSE && afterBlock.column == chainColumn) {
    // Skip the false branch once the true branch finishes running
    Instruction jumpEnd = {0};
    
    jumpEnd.opcode = OP_JUMP;
    
    int jumpEndIndex = EmitInstruction(jumpEnd);
    
    // False branch starts here
    bytecode[jumpIfFalseIndex].jumpTarget = bytecodeCount;
    
    Token afterElse = LexerNext();
    
    if(afterElse.type == TOKEN_KW_IF) {
      afterBlock = ParseIfStatement(chainColumn);
    } else {
      if(afterElse.type != TOKEN_OP_ASSIGN) {
        CompileError(afterElse.line, "Expected =");
      }
      
      afterBlock = ParseBlock(LexerNext(), chainColumn);
    }
    
    bytecode[jumpEndIndex].jumpTarget = bytecodeCount;
  } else {
    // No else, false branch is simply whatever comes after this if
    bytecode[jumpIfFalseIndex].jumpTarget = bytecodeCount;
  }
  
  return afterBlock;
}

// Parse one full statement, returning the lookahead token that follows it
Token ParseStatement(Token token) {
  if(token.type == TOKEN_ERROR) {
    CompileError(token.line, "Unexpected character");
  }
  
  // Handle var declaration
  if(token.type == TOKEN_KW_VAR) {
    Token type = LexerNext();
    
    VarType varType;
    
    if(type.type == TOKEN_TYPE_INT) {
      varType = VAR_INT;
    } else if(type.type == TOKEN_TYPE_FLOAT) {
      varType = VAR_FLOAT;
    } else if(type.type == TOKEN_TYPE_STR) {
      varType = VAR_STR;
    } else if(type.type == TOKEN_TYPE_BOOL) {
      varType = VAR_BOOL;
    } else {
      CompileError(type.line, "Expected type");
    }
    
    Token name = LexerNext();
    
    // Validate identifier
    if(name.type != TOKEN_IDENTIFIER) {
      CompileError(name.line, "Expected variable name");
    }
    
    Token next = LexerNext();
    
    if(varType == VAR_STR) {
      StringResult value = {0};
      
      // Check optional assignment
      if(next.type == TOKEN_OP_ASSIGN) {
        value = ParseStringValue(LexerNext());
      } else {
        value.srcVarIndex = -1;
        value.next = next;
      }
      
      // Validate semicolon
      if(value.next.type != TOKEN_SEMICOLON) {
        CompileError(value.next.line, "Expected ;");
      }
      
      // Register variable after evaluating value, so it cannot reference itself
      int index = DeclareSymbol(name.value, varType, name.line);
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_DECLARE_STR;
      instruction.varIndex = index;
      instruction.srcVarIndex = value.srcVarIndex;
      
      strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
      instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      strncpy(instruction.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
      instruction.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      EmitInstruction(instruction);
    } else if(varType == VAR_BOOL) {
      Token nextAfter;
      
      // Check optional assignment
      if(next.type == TOKEN_OP_ASSIGN) {
        nextAfter = ParseBoolValue(LexerNext());
      } else {
        Instruction zero = {0};
        
        zero.opcode = OP_PUSH_NUMBER;
        zero.numberValue = 0;
        
        EmitInstruction(zero);
        
        nextAfter = next;
      }
      
      // Validate semicolon
      if(nextAfter.type != TOKEN_SEMICOLON) {
        CompileError(nextAfter.line, "Expected ;");
      }
      
      int index = DeclareSymbol(name.value, varType, name.line);
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_DECLARE_BOOL;
      instruction.varIndex = index;
      
      strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
      instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      EmitInstruction(instruction);
    } else {
      // int or float, both use the numeric expression parser
      Token nextAfter;
      
      // Check optional assignment
      if(next.type == TOKEN_OP_ASSIGN) {
        Token value = LexerNext();
        ExprResult exprResult = ParseNumericExpression(value);
        
        // Reject narrowing float value into an int variable
        if(varType == VAR_INT && exprResult.type == VAR_FLOAT) {
          CompileError(value.line, "Cannot assign float value to int variable '%s'", name.value);
        }
        
        nextAfter = exprResult.next;
      } else {
        Instruction zero = {0};
        
        zero.opcode = OP_PUSH_NUMBER;
        zero.numberValue = 0;
        
        EmitInstruction(zero);
        
        nextAfter = next;
      }
      
      // Validate semicolon
      if(nextAfter.type != TOKEN_SEMICOLON) {
        CompileError(nextAfter.line, "Expected ;");
      }
      
      int index = DeclareSymbol(name.value, varType, name.line);
      
      Instruction instruction = {0};
      
      instruction.opcode = (varType == VAR_INT) ? OP_DECLARE_INT : OP_DECLARE_FLOAT;
      instruction.varIndex = index;
      
      strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
      instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      EmitInstruction(instruction);
    }
    
    return LexerNext();
  }
  
  // handle exit func
  if(token.type == TOKEN_KW_EXIT) {
    Token semicolon = LexerNext();
    
    // semicolon
    if(semicolon.type != TOKEN_SEMICOLON) {
      CompileError(semicolon.line, "Expected ;");
    }
    
    Instruction instruction = {0};
    instruction.opcode = OP_HALT;
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  // Handle variable assignment, or increment/decrement
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    // Validate variable exists
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    Token next = LexerNext();
    
    // variabel++; or variabel--;
    if(next.type == TOKEN_OP_INC || next.type == TOKEN_OP_DEC) {
      if(symbols[index].type != VAR_INT) {
        CompileError(token.line, "Increment/decrement only supported for int variables");
      }
      
      Token semicolon = LexerNext();
      
      // Validate semicolon
      if(semicolon.type != TOKEN_SEMICOLON) {
        CompileError(semicolon.line, "Expected ;");
      }
      
      Instruction pushVar = {0};
      
      pushVar.opcode = OP_PUSH_VAR;
      pushVar.varIndex = index;
      
      EmitInstruction(pushVar);
      
      Instruction pushOne = {0};
      
      pushOne.opcode = OP_PUSH_NUMBER;
      pushOne.numberValue = 1;
      
      EmitInstruction(pushOne);
      
      Instruction op = {0};
      
      op.opcode = (next.type == TOKEN_OP_INC) ? OP_ADD : OP_SUB;
      
      EmitInstruction(op);
      
      Instruction store = {0};
      
      store.opcode = OP_STORE_VAR;
      store.varIndex = index;
      
      EmitInstruction(store);
      
      return LexerNext();
    }
    
    // Validate assignment operator
    if(next.type != TOKEN_OP_ASSIGN) {
      CompileError(next.line, "Expected =");
    }
    
    VarType varType = symbols[index].type;
    
    if(varType == VAR_STR) {
      StringResult value = ParseStringValue(LexerNext());
      
      // Validate semicolon
      if(value.next.type != TOKEN_SEMICOLON) {
        CompileError(value.next.line, "Expected ;");
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_STORE_STR;
      instruction.varIndex = index;
      instruction.srcVarIndex = value.srcVarIndex;
      
      strncpy(instruction.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
      instruction.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      EmitInstruction(instruction);
    } else if(varType == VAR_BOOL) {
      Token afterValue = ParseBoolValue(LexerNext());
      
      // Validate semicolon
      if(afterValue.type != TOKEN_SEMICOLON) {
        CompileError(afterValue.line, "Expected ;");
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_STORE_VAR;
      instruction.varIndex = index;
      
      EmitInstruction(instruction);
    } else {
      Token value = LexerNext();
      ExprResult result = ParseNumericExpression(value);
      
      // Reject narrowing float value into an int variable
      if(varType == VAR_INT && result.type == VAR_FLOAT) {
        CompileError(value.line, "Cannot assign float value to int variable '%s'", token.value);
      }
      
      // Validate semicolon
      if(result.next.type != TOKEN_SEMICOLON) {
        CompileError(result.next.line, "Expected ;");
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_STORE_VAR;
      instruction.varIndex = index;
      
      EmitInstruction(instruction);
    }
    
    return LexerNext();
  }
  
  // Handle print statement
  if(token.type == TOKEN_KW_PRINT) {
    Token value = LexerNext();
    
    // Print string literal
    if(value.type == TOKEN_LIT_STRING) {
      Token semicolon = LexerNext();
      
      // Validate semicolon
      if(semicolon.type != TOKEN_SEMICOLON) {
        CompileError(semicolon.line, "Expected ;");
      }
      
      // Create print opcode
      Instruction instruction = {0};
      
      instruction.opcode = OP_PRINT;
      
      strncpy(instruction.text, value.value, INSTRUCTION_MAX_LEN - 1);
      instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      EmitInstruction(instruction);
    } else if(value.type == TOKEN_LIT_BOOL) {
      // Print bool literal
      Token semicolon = LexerNext();
      
      // Validate semicolon
      if(semicolon.type != TOKEN_SEMICOLON) {
        CompileError(semicolon.line, "Expected ;");
      }
      
      Instruction pushInstruction = {0};
      
      pushInstruction.opcode = OP_PUSH_NUMBER;
      pushInstruction.numberValue = atoi(value.value);
      
      EmitInstruction(pushInstruction);
      
      Instruction printInstruction = {0};
      
      printInstruction.opcode = OP_PRINT_VALUE;
      printInstruction.valueType = VAR_BOOL;
      
      EmitInstruction(printInstruction);
    } else if(value.type == TOKEN_IDENTIFIER) {
      int index = FindSymbol(value.value);
      
      if(index != -1 && symbols[index].type == VAR_STR) {
        // Print string variable
        Token semicolon = LexerNext();
        
        // Validate semicolon
        if(semicolon.type != TOKEN_SEMICOLON) {
          CompileError(semicolon.line, "Expected ;");
        }
        
        Instruction instruction = {0};
        
        instruction.opcode = OP_PRINT_STR_VAR;
        instruction.varIndex = index;
        
        EmitInstruction(instruction);
      } else if(index != -1 && symbols[index].type == VAR_BOOL) {
        // Print bool variable
        Token semicolon = LexerNext();
        
        // Validate semicolon
        if(semicolon.type != TOKEN_SEMICOLON) {
          CompileError(semicolon.line, "Expected ;");
        }
        
        Instruction pushInstruction = {0};
        
        pushInstruction.opcode = OP_PUSH_VAR;
        pushInstruction.varIndex = index;
        
        EmitInstruction(pushInstruction);
        
        Instruction printInstruction = {0};
        
        printInstruction.opcode = OP_PRINT_VALUE;
        printInstruction.valueType = VAR_BOOL;
        
        EmitInstruction(printInstruction);
      } else {
        // Int or float variable, or an expression starting with an identifier
        ExprResult result = ParseNumericExpression(value);
        
        // Validate semicolon
        if(result.next.type != TOKEN_SEMICOLON) {
          CompileError(result.next.line, "Expected ;");
        }
        
        Instruction instruction = {0};
        
        instruction.opcode = OP_PRINT_VALUE;
        instruction.valueType = result.type;
        
        EmitInstruction(instruction);
      }
    } else {
      // Number, float literal, or parenthesized expression
      ExprResult result = ParseNumericExpression(value);
      
      // Validate semicolon
      if(result.next.type != TOKEN_SEMICOLON) {
        CompileError(result.next.line, "Expected ;");
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_PRINT_VALUE;
      instruction.valueType = result.type;
      
      EmitInstruction(instruction);
    }
    
    return LexerNext();
  }
  
  // Handle if / else if / else
  if(token.type == TOKEN_KW_IF) {
    return ParseIfStatement(token.column);
  }
  
  CompileError(token.line, "Expected statement");
}

// Convert source into bytecode
void Compile() {
  Token token = LexerNext();
  
  while(token.type != TOKEN_EOF) {
    token = ParseStatement(token);
  }
  
  // Add program stop
  Instruction halt = {0};
  
  halt.opcode = OP_HALT;
  
  EmitInstruction(halt);
}

int main(int argc, char** argv) {
  if(argc < 2) {
    printf("Usage: elvm <file.ell>\n");
    return 1;
  }
  
  // Remember filename for error messages
  sourceFilename = argv[1];
  
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
    bytecodeCount,
    sourceFilename
  );
  
  // Free memory
  free(source);
  free(bytecode);
  
  return 0;
}
