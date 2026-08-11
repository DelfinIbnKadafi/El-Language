#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include "lexer.h"
#include "elvm.h"
#include "parser.h"

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
  
  int isArray;
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
int DeclareSymbol(char* name, VarType type, int isArray, int line) {
  if(FindSymbol(name) != -1) {
    CompileError(line, "Variable '%s' already declared", name);
  }
  
  if(symbolCount >= MAX_VARIABLES) {
    CompileError(line, "Too many variables");
  }
  
  strncpy(symbols[symbolCount].name, name, TOKEN_MAX_LEN - 1);
  symbols[symbolCount].name[TOKEN_MAX_LEN - 1] = '\0';
  
  symbols[symbolCount].type = type;
  symbols[symbolCount].isArray = isArray;
  
  return symbolCount++;
}

// Add one instruction, growing storage if needed. Returns the index it was stored at.
int EmitInstruction(Instruction instruction) {
  if(bytecodeCount >= bytecodeCap) {
    bytecodeCap *= 2;
    
    bytecode = realloc(bytecode, bytecodeCap * sizeof(Instruction));
  }
  
  bytecode[bytecodeCount] = instruction;
  
  return bytecodeCount++;
}

// Re-emit a previously emitted range of instructions [start, end). Used to
// duplicate an array index's bytecode for variabel[i]++ / variabel[i]--,
// which need to evaluate the index once for the read and once for the write.
void DuplicateInstructions(int start, int end) {
  for(int i = start; i < end; i++) {
    EmitInstruction(bytecode[i]);
  }
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
  int srcIsArray;
  
  // True if the value was literally the NONE keyword
  int isNoneLiteral;
} StringResult;

// Result of parsing a condition: lookahead token and resulting type (always bool once combined)
typedef struct {
  Token next;
  
  VarType type;
} CondResult;

// Forward declarations, needed because parentheses / arrays make these mutually recursive
ExprResult ParseNumericExpression(Token token);
CondResult ParseOrExpr(Token token);
Token ParseStatement(Token token);

// Parse an optional array index following an identifier that resolves to symbol
// 'symbolIndex'. 'nameToken' identifies the variable for error messages, and
// 'afterName' is the token already read immediately after the identifier.
//
// If the symbol is an array, '[' is required: the index expression is parsed
// and its bytecode emitted (pushing the index onto the numeric stack), and the
// token following ']' is returned. If the symbol is not an array, '[' is
// rejected. Returns the lookahead token that follows the (optional) index.
Token ParseArrayIndex(int symbolIndex, Token nameToken, Token afterName) {
  if(symbols[symbolIndex].isArray) {
    if(afterName.type != TOKEN_LBRACKET) {
      CompileError(nameToken.line, "Variable '%s' is an array and must be indexed", nameToken.value);
    }
    
    ExprResult index = ParseNumericExpression(LexerNext());
    
    if(index.type != VAR_INT) {
      CompileError(nameToken.line, "Array index must be an int");
    }
    
    if(index.next.type != TOKEN_RBRACKET) {
      CompileError(index.next.line, "Expected ]");
    }
    
    return LexerNext();
  }
  
  if(afterName.type == TOKEN_LBRACKET) {
    CompileError(nameToken.line, "Variable '%s' is not an array", nameToken.value);
  }
  
  return afterName;
}

// Parse a single numeric value: literal, variable (with optional array index),
// or parenthesized expression. 'token' is the already-read token to interpret.
ExprResult ParseNumericFactor(Token token) {
  ExprResult result = {0};
  
  if(token.type == TOKEN_ERROR) {
    CompileError(token.line, "%s", token.value);
  }
  
  // Unary minus, computed as (0 - value) so it reuses OP_SUB
  if(token.type == TOKEN_OP_SUB) {
    Instruction zero = {0};
    
    zero.opcode = OP_PUSH_NUMBER;
    zero.numberValue = 0;
    
    EmitInstruction(zero);
    
    ExprResult inner = ParseNumericFactor(LexerNext());
    
    Instruction sub = {0};
    
    sub.opcode = OP_SUB;
    sub.line = token.line;
    
    EmitInstruction(sub);
    
    result.type = inner.type;
    result.next = inner.next;
    
    return result;
  }
  
  // Unary plus, no-op
  if(token.type == TOKEN_OP_ADD) {
    return ParseNumericFactor(LexerNext());
  }
  
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
  
  // Int or float variable reference, optionally indexed
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    VarType type = symbols[index].type;
    
    if(type != VAR_INT && type != VAR_FLOAT) {
      CompileError(token.line, "Variable '%s' cannot be used in a math expression", token.value);
    }
    
    Token afterName = LexerNext();
    
    result.next = ParseArrayIndex(index, token, afterName);
    
    Instruction instruction = {0};
    
    instruction.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
    instruction.varIndex = index;
    instruction.line = token.line;
    
    EmitInstruction(instruction);
    
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

// Parse a single string value: string literal, or an existing string variable
// (whole, or one array element) - no concatenation supported
StringResult ParseStringValue(Token token) {
  StringResult result = {0};
  
  if(token.type == TOKEN_ERROR) {
    CompileError(token.line, "%s", token.value);
  }
  
  if(token.type == TOKEN_KW_NONE) {
    result.isNoneLiteral = 1;
    result.srcVarIndex = -1;
    result.next = LexerNext();
    
    return result;
  }
  
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
    
    Token afterName = LexerNext();
    
    result.next = ParseArrayIndex(index, token, afterName);
    result.srcVarIndex = index;
    result.srcIsArray = symbols[index].isArray;
    
    return result;
  }
  
  CompileError(token.line, "Expected string value");
}

// Result of parsing a bool value: lookahead token, and whether it was NONE
typedef struct {
  Token next;
  
  int isNoneLiteral;
} BoolResult;

// Parse a single bool value: true/false literal, NONE, or an existing bool
// variable (whole, or one array element) - no math allowed
BoolResult ParseBoolValue(Token token) {
  BoolResult result = {0};
  
  if(token.type == TOKEN_ERROR) {
    CompileError(token.line, "%s", token.value);
  }
  
  if(token.type == TOKEN_KW_NONE) {
    result.isNoneLiteral = 1;
    result.next = LexerNext();
    
    return result;
  }
  
  if(token.type == TOKEN_LIT_BOOL) {
    Instruction instruction = {0};
    
    instruction.opcode = OP_PUSH_NUMBER;
    instruction.numberValue = atoi(token.value);
    
    EmitInstruction(instruction);
    
    result.next = LexerNext();
    
    return result;
  }
  
  if(token.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    if(symbols[index].type != VAR_BOOL) {
      CompileError(token.line, "Variable '%s' is not a bool", token.value);
    }
    
    Token afterName = LexerNext();
    
    Token next = ParseArrayIndex(index, token, afterName);
    
    Instruction instruction = {0};
    
    instruction.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
    instruction.varIndex = index;
    instruction.line = token.line;
    
    EmitInstruction(instruction);
    
    result.next = next;
    
    return result;
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
      Token afterName = LexerNext();
      
      result.next = ParseArrayIndex(index, token, afterName);
      
      Instruction instruction = {0};
      
      instruction.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
      instruction.varIndex = index;
      instruction.line = token.line;
      
      EmitInstruction(instruction);
      
      result.type = VAR_BOOL;
      
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
  // A string variable can only ever be compared with == NONE or != NONE
  if(token.type == TOKEN_IDENTIFIER) {
    int strIndex = FindSymbol(token.value);
    
    if(strIndex != -1 && symbols[strIndex].type == VAR_STR) {
      Token afterName = LexerNext();
      Token afterIndex = ParseArrayIndex(strIndex, token, afterName);
      
      if(afterIndex.type != TOKEN_OP_EQ && afterIndex.type != TOKEN_OP_NE) {
        CompileError(token.line, "String variable '%s' can only be compared with == NONE or != NONE", token.value);
      }
      
      TokenType op = afterIndex.type;
      int opLine = afterIndex.line;
      
      Token rhs = LexerNext();
      
      if(rhs.type != TOKEN_KW_NONE) {
        CompileError(opLine, "String variable '%s' can only be compared with NONE", token.value);
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = (op == TOKEN_OP_EQ) ? OP_STR_IS_NONE : OP_STR_IS_NOT_NONE;
      instruction.varIndex = strIndex;
      instruction.destIsArray = symbols[strIndex].isArray;
      instruction.line = token.line;
      
      EmitInstruction(instruction);
      
      CondResult result = {0};
      
      result.type = VAR_BOOL;
      result.next = LexerNext();
      
      return result;
    }
  }
  
  int leftStart = bytecodeCount;
  CondResult left = ParseConditionOperand(token);
  int leftEnd = bytecodeCount;
  
  TokenType op = left.next.type;
  
  if(op == TOKEN_OP_GT || op == TOKEN_OP_LT || op == TOKEN_OP_EQ ||
     op == TOKEN_OP_GE || op == TOKEN_OP_LE || op == TOKEN_OP_NE) {
    int opLine = left.next.line;
    
    Token rhsToken = LexerNext();
    
    // "vars == NONE" / "vars != NONE": only valid directly after a bare variable reference
    if(rhsToken.type == TOKEN_KW_NONE) {
      if(op != TOKEN_OP_EQ && op != TOKEN_OP_NE) {
        CompileError(opLine, "NONE can only be compared using == or !=");
      }
      
      if(leftEnd == leftStart ||
         (bytecode[leftEnd - 1].opcode != OP_PUSH_VAR && bytecode[leftEnd - 1].opcode != OP_PUSH_ARR)) {
        CompileError(opLine, "NONE can only be compared against a variable");
      }
      
      // Discard the value that was just pushed, we only care about its NONE status
      Instruction discard = {0};
      
      discard.opcode = OP_POP;
      
      EmitInstruction(discard);
      
      Instruction checkNone = {0};
      
      checkNone.opcode = OP_PUSH_LAST_NONE_FLAG;
      checkNone.numberValue = (op == TOKEN_OP_NE) ? 1 : 0;
      
      EmitInstruction(checkNone);
      
      left.type = VAR_BOOL;
      left.next = LexerNext();
      
      return left;
    }
    
    CondResult right = ParseConditionOperand(rhsToken);
    
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

// Parse the body of an if / else if / else clause. 'token' is the first token
// after the '='. 'parentColumn' is the column of the if/else keyword that owns
// this body. 'headerLine' is the source line of the '=' that opened this body.
//
// The body may start inline (same line as '=') and/or continue on following
// lines indented deeper than parentColumn, in any combination and in any order.
// A statement is considered part of the body as long as it is either on the
// same source line as the previous statement in the body, or indented deeper
// than parentColumn. The body ends at EOF, at 'else', or at the first token
// that satisfies neither condition. An empty body (nothing follows) is a no-op.
Token ParseBlock(Token token, int parentColumn, int headerLine) {
  if(token.type == TOKEN_EOF || token.type == TOKEN_KW_ELSE) {
    return token;
  }
  
  if(token.line != headerLine && token.column <= parentColumn) {
    // Nothing on the header line, and the next line isn't indented either
    return token;
  }
  
  int lastLine = token.line;
  
  token = ParseStatement(token);
  
  while(token.type != TOKEN_EOF && token.type != TOKEN_KW_ELSE &&
        (token.line == lastLine || token.column > parentColumn)) {
    lastLine = token.line;
    
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
  
  Token afterBlock = ParseBlock(LexerNext(), chainColumn, assign.line);
  
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
      
      afterBlock = ParseBlock(LexerNext(), chainColumn, afterElse.line);
    }
    
    bytecode[jumpEndIndex].jumpTarget = bytecodeCount;
  } else {
    // No else, false branch is simply whatever comes after this if
    bytecode[jumpIfFalseIndex].jumpTarget = bytecodeCount;
  }
  
  return afterBlock;
}

// Parse a var declaration, from just after the 'var' keyword. Returns the
// lookahead token that follows the declaration.
// Emit bytecode to parse one value and store it into an array element, assuming
// the destination index has already been pushed onto the stack by the caller
// immediately before this call. Used by both the list and broadcast forms of
// an array initializer, reusing the same per-element store opcodes as a normal
// 'arr[i] = value;' statement. Returns the lookahead token after the value.
Token EmitElementStore(VarType varType, int varIndex, Token valueToken) {
  if(varType == VAR_STR) {
    StringResult value = ParseStringValue(valueToken);
    
    Instruction store = {0};
    
    store.opcode = OP_STORE_STR;
    store.varIndex = varIndex;
    store.destIsArray = 1;
    store.storeNone = value.isNoneLiteral;
    store.srcVarIndex = value.srcVarIndex;
    store.srcIsArray = value.srcIsArray;
    store.line = valueToken.line;
    
    strncpy(store.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
    store.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(store);
    
    return value.next;
  }
  
  if(varType == VAR_BOOL) {
    BoolResult value = ParseBoolValue(valueToken);
    
    Instruction store = {0};
    
    store.opcode = OP_STORE_ARR;
    store.varIndex = varIndex;
    store.storeNone = value.isNoneLiteral;
    store.line = valueToken.line;
    
    EmitInstruction(store);
    
    return value.next;
  }
  
  // int or float
  if(valueToken.type == TOKEN_KW_NONE) {
    Instruction store = {0};
    
    store.opcode = OP_STORE_ARR;
    store.varIndex = varIndex;
    store.storeNone = 1;
    store.line = valueToken.line;
    
    EmitInstruction(store);
    
    return LexerNext();
  }
  
  ExprResult result = ParseNumericExpression(valueToken);
  
  if(varType == VAR_INT && result.type == VAR_FLOAT) {
    CompileError(valueToken.line, "Cannot assign float value to int array element");
  }
  
  Instruction store = {0};
  
  store.opcode = OP_STORE_ARR;
  store.varIndex = varIndex;
  store.line = valueToken.line;
  
  EmitInstruction(store);
  
  return result.next;
}

// Emit "push elementIndex" followed by EmitElementStore, for one list item.
Token EmitArrayElementStore(VarType varType, int varIndex, int elementIndex, Token valueToken) {
  Instruction pushIdx = {0};
  
  pushIdx.opcode = OP_PUSH_NUMBER;
  pushIdx.numberValue = elementIndex;
  
  EmitInstruction(pushIdx);
  
  return EmitElementStore(varType, varIndex, valueToken);
}

// Parse a number token as a positive size (for array size or string size),
// safely detecting overflow instead of relying on atoi's undefined behavior
// on out-of-range input. Stops the program with a clear error on failure.
int ParsePositiveSize(Token sizeToken, char* what) {
  errno = 0;
  
  char* end;
  long value = strtol(sizeToken.value, &end, 10);
  
  if(errno == ERANGE || value > INT_MAX) {
    CompileError(sizeToken.line, "%s is too large", what);
  }
  
  if(value <= 0) {
    CompileError(sizeToken.line, "%s must be positive", what);
  }
  
  return (int) value;
}

Token ParseVarDeclaration() {
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
  
  Token afterType = LexerNext();
  
  // Optional str[size] form, sets the max characters per string
  int strSize = 0;
  
  if(varType == VAR_STR && afterType.type == TOKEN_LBRACKET) {
    Token sizeToken = LexerNext();
    
    if(sizeToken.type != TOKEN_LIT_NUMBER) {
      CompileError(sizeToken.line, "Expected string size");
    }
    
    strSize = ParsePositiveSize(sizeToken, "String size");
    
    if(strSize >= MAX_STR_LEN) {
      CompileError(sizeToken.line, "String size must be less than %d", MAX_STR_LEN);
    }
    
    Token closeBracket = LexerNext();
    
    if(closeBracket.type != TOKEN_RBRACKET) {
      CompileError(closeBracket.line, "Expected ]");
    }
    
    afterType = LexerNext();
  }
  
  Token name = afterType;
  
  if(name.type != TOKEN_IDENTIFIER) {
    CompileError(name.line, "Expected variable name");
  }
  
  Token next = LexerNext();
  
  // Optional [arraySize] form, makes this an array declaration
  int isArray = 0;
  int arraySize = 1;
  
  if(next.type == TOKEN_LBRACKET) {
    isArray = 1;
    
    Token sizeToken = LexerNext();
    
    if(sizeToken.type != TOKEN_LIT_NUMBER) {
      CompileError(sizeToken.line, "Expected array size");
    }
    
    arraySize = ParsePositiveSize(sizeToken, "Array size");
    
    Token closeBracket = LexerNext();
    
    if(closeBracket.type != TOKEN_RBRACKET) {
      CompileError(closeBracket.line, "Expected ]");
    }
    
    next = LexerNext();
  }
  
  if(isArray) {
    int index = DeclareSymbol(name.value, varType, 1, name.line);
    
    Instruction declareInstr = {0};
    
    declareInstr.opcode = (varType == VAR_INT) ? OP_DECLARE_INT :
                           (varType == VAR_FLOAT) ? OP_DECLARE_FLOAT :
                           (varType == VAR_BOOL) ? OP_DECLARE_BOOL : OP_DECLARE_STR;
    declareInstr.varIndex = index;
    declareInstr.isArray = 1;
    declareInstr.arraySize = arraySize;
    declareInstr.strSize = strSize;
    
    strncpy(declareInstr.text, name.value, INSTRUCTION_MAX_LEN - 1);
    declareInstr.text[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(declareInstr);
    
    // No initializer: every element stays NONE, as set up by the declare above
    if(next.type == TOKEN_SEMICOLON) {
      return LexerNext();
    }
    
    if(next.type != TOKEN_OP_ASSIGN) {
      CompileError(next.line, "Expected ; or =");
    }
    
    Token afterAssign = LexerNext();
    
    if(afterAssign.type == TOKEN_LBRACE) {
      // List form: { v1, v2, ..., vn }, applied in order starting at index 0.
      // Any elements past the last listed value stay NONE.
      int elementIndex = 0;
      
      Token itemToken = LexerNext();
      
      if(itemToken.type != TOKEN_RBRACE) {
        while(1) {
          if(elementIndex >= arraySize) {
            CompileError(itemToken.line, "Too many initializer values for array of size %d", arraySize);
          }
          
          Token afterItem = EmitArrayElementStore(varType, index, elementIndex, itemToken);
          
          elementIndex++;
          
          if(afterItem.type == TOKEN_COMMA) {
            itemToken = LexerNext();
            continue;
          }
          
          if(afterItem.type == TOKEN_RBRACE) {
            break;
          }
          
          CompileError(afterItem.line, "Expected , or }");
        }
      }
      
      Token afterBrace = LexerNext();
      
      if(afterBrace.type != TOKEN_SEMICOLON) {
        CompileError(afterBrace.line, "Expected ;");
      }
      
      return LexerNext();
    }
    
    // Broadcast form: a single value applied to every element. The value is
    // evaluated once and the VM fills every element with it, so bytecode size
    // stays constant no matter how large the array is.
    Token afterValue;
    
    if(varType == VAR_STR) {
      StringResult value = ParseStringValue(afterAssign);
      
      Instruction broadcast = {0};
      
      broadcast.opcode = OP_BROADCAST_STR_ARR;
      broadcast.varIndex = index;
      broadcast.storeNone = value.isNoneLiteral;
      broadcast.srcVarIndex = value.srcVarIndex;
      broadcast.srcIsArray = value.srcIsArray;
      broadcast.line = afterAssign.line;
      
      strncpy(broadcast.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
      broadcast.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
      
      EmitInstruction(broadcast);
      
      afterValue = value.next;
    } else if(varType == VAR_BOOL) {
      BoolResult value = ParseBoolValue(afterAssign);
      
      Instruction broadcast = {0};
      
      broadcast.opcode = OP_BROADCAST_ARR;
      broadcast.varIndex = index;
      broadcast.storeNone = value.isNoneLiteral;
      broadcast.line = afterAssign.line;
      
      EmitInstruction(broadcast);
      
      afterValue = value.next;
    } else if(afterAssign.type == TOKEN_KW_NONE) {
      Instruction broadcast = {0};
      
      broadcast.opcode = OP_BROADCAST_ARR;
      broadcast.varIndex = index;
      broadcast.storeNone = 1;
      broadcast.line = afterAssign.line;
      
      EmitInstruction(broadcast);
      
      afterValue = LexerNext();
    } else {
      ExprResult result = ParseNumericExpression(afterAssign);
      
      if(varType == VAR_INT && result.type == VAR_FLOAT) {
        CompileError(afterAssign.line, "Cannot assign float value to int array");
      }
      
      Instruction broadcast = {0};
      
      broadcast.opcode = OP_BROADCAST_ARR;
      broadcast.varIndex = index;
      broadcast.line = afterAssign.line;
      
      EmitInstruction(broadcast);
      
      afterValue = result.next;
    }
    
    if(afterValue.type != TOKEN_SEMICOLON) {
      CompileError(afterValue.line, "Expected ;");
    }
    
    return LexerNext();
  }
  
  if(varType == VAR_STR) {
    StringResult value = {0};
    
    if(next.type == TOKEN_OP_ASSIGN) {
      value = ParseStringValue(LexerNext());
    } else {
      // No initializer at all: starts as NONE
      value.srcVarIndex = -1;
      value.isNoneLiteral = 1;
      value.next = next;
    }
    
    if(value.next.type != TOKEN_SEMICOLON) {
      CompileError(value.next.line, "Expected ;");
    }
    
    int index = DeclareSymbol(name.value, varType, 0, name.line);
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_DECLARE_STR;
    instruction.varIndex = index;
    instruction.strSize = strSize;
    instruction.storeNone = value.isNoneLiteral;
    instruction.srcVarIndex = value.srcVarIndex;
    instruction.srcIsArray = value.srcIsArray;
    
    strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
    instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    strncpy(instruction.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
    instruction.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  if(varType == VAR_BOOL) {
    Token nextAfter;
    int storeNone = 0;
    int propagateNone = 0;
    
    if(next.type == TOKEN_OP_ASSIGN) {
      int valueStart = bytecodeCount;
      BoolResult value = ParseBoolValue(LexerNext());
      int valueEnd = bytecodeCount;
      
      storeNone = value.isNoneLiteral;
      propagateNone = !storeNone && valueEnd > valueStart &&
        (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR);
      nextAfter = value.next;
    } else {
      // No initializer at all: starts as NONE
      storeNone = 1;
      nextAfter = next;
    }
    
    if(nextAfter.type != TOKEN_SEMICOLON) {
      CompileError(nextAfter.line, "Expected ;");
    }
    
    int index = DeclareSymbol(name.value, varType, 0, name.line);
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_DECLARE_BOOL;
    instruction.varIndex = index;
    instruction.storeNone = storeNone;
    instruction.propagateNone = propagateNone;
    
    strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
    instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  // int or float
  Token nextAfter;
  int storeNone = 0;
  int propagateNone = 0;
  
  if(next.type == TOKEN_OP_ASSIGN) {
    Token value = LexerNext();
    
    if(value.type == TOKEN_KW_NONE) {
      storeNone = 1;
      nextAfter = LexerNext();
    } else {
      int valueStart = bytecodeCount;
      ExprResult exprResult = ParseNumericExpression(value);
      int valueEnd = bytecodeCount;
      
      if(varType == VAR_INT && exprResult.type == VAR_FLOAT) {
        CompileError(value.line, "Cannot assign float value to int variable '%s'", name.value);
      }
      
      propagateNone = valueEnd > valueStart &&
        (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR);
      
      nextAfter = exprResult.next;
    }
  } else {
    // No initializer at all: starts as NONE
    storeNone = 1;
    nextAfter = next;
  }
  
  if(nextAfter.type != TOKEN_SEMICOLON) {
    CompileError(nextAfter.line, "Expected ;");
  }
  
  int index = DeclareSymbol(name.value, varType, 0, name.line);
  
  Instruction instruction = {0};
  
  instruction.opcode = (varType == VAR_INT) ? OP_DECLARE_INT : OP_DECLARE_FLOAT;
  instruction.varIndex = index;
  instruction.storeNone = storeNone;
  instruction.propagateNone = propagateNone;
  
  strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
  instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
  
  EmitInstruction(instruction);
  
  return LexerNext();
}

// Parse an identifier-led statement: assignment, or increment/decrement,
// for either a scalar variable or one array element.
Token ParseIdentifierStatement(Token token) {
  int index = FindSymbol(token.value);
  
  if(index == -1) {
    CompileError(token.line, "Undefined symbol \"%s\"", token.value);
  }
  
  Token afterName = LexerNext();
  
  int isIndexed = symbols[index].isArray;
  int idxStart = 0;
  int idxEnd = 0;
  
  Token next;
  
  if(isIndexed) {
    if(afterName.type != TOKEN_LBRACKET) {
      CompileError(token.line, "Variable '%s' is an array and must be indexed", token.value);
    }
    
    idxStart = bytecodeCount;
    
    ExprResult idxExpr = ParseNumericExpression(LexerNext());
    
    if(idxExpr.type != VAR_INT) {
      CompileError(token.line, "Array index must be an int");
    }
    
    idxEnd = bytecodeCount;
    
    if(idxExpr.next.type != TOKEN_RBRACKET) {
      CompileError(idxExpr.next.line, "Expected ]");
    }
    
    next = LexerNext();
  } else {
    if(afterName.type == TOKEN_LBRACKET) {
      CompileError(token.line, "Variable '%s' is not an array", token.value);
    }
    
    next = afterName;
  }
  
  // variabel++; or variabel--;
  if(next.type == TOKEN_OP_INC || next.type == TOKEN_OP_DEC) {
    if(symbols[index].type != VAR_INT) {
      CompileError(token.line, "Increment/decrement only supported for int variables");
    }
    
    Token semicolon = LexerNext();
    
    if(semicolon.type != TOKEN_SEMICOLON) {
      CompileError(semicolon.line, "Expected ;");
    }
    
    if(isIndexed) {
      // Duplicate the index bytecode: one copy to read, one to write back
      DuplicateInstructions(idxStart, idxEnd);
    }
    
    Instruction pushVar = {0};
    
    pushVar.opcode = isIndexed ? OP_PUSH_ARR : OP_PUSH_VAR;
    pushVar.varIndex = index;
    pushVar.line = token.line;
    
    EmitInstruction(pushVar);
    
    Instruction pushOne = {0};
    
    pushOne.opcode = OP_PUSH_NUMBER;
    pushOne.numberValue = 1;
    
    EmitInstruction(pushOne);
    
    Instruction op = {0};
    
    op.opcode = (next.type == TOKEN_OP_INC) ? OP_ADD : OP_SUB;
    
    EmitInstruction(op);
    
    Instruction store = {0};
    
    store.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
    store.varIndex = index;
    store.line = token.line;
    
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
    
    if(value.next.type != TOKEN_SEMICOLON) {
      CompileError(value.next.line, "Expected ;");
    }
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_STORE_STR;
    instruction.varIndex = index;
    instruction.destIsArray = isIndexed;
    instruction.srcVarIndex = value.srcVarIndex;
    instruction.srcIsArray = value.srcIsArray;
    instruction.line = token.line;
    
    strncpy(instruction.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
    instruction.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(instruction);
  } else if(varType == VAR_BOOL) {
    int valueStart = bytecodeCount;
    BoolResult value = ParseBoolValue(LexerNext());
    int valueEnd = bytecodeCount;
    
    if(value.next.type != TOKEN_SEMICOLON) {
      CompileError(value.next.line, "Expected ;");
    }
    
    int isBareCopy = !value.isNoneLiteral && valueEnd > valueStart &&
      (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR);
    
    Instruction instruction = {0};
    
    instruction.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
    instruction.varIndex = index;
    instruction.storeNone = value.isNoneLiteral;
    instruction.propagateNone = isBareCopy;
    instruction.line = token.line;
    
    EmitInstruction(instruction);
  } else {
    Token value = LexerNext();
    
    if(value.type == TOKEN_KW_NONE) {
      Token semicolon = LexerNext();
      
      if(semicolon.type != TOKEN_SEMICOLON) {
        CompileError(semicolon.line, "Expected ;");
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
      instruction.varIndex = index;
      instruction.storeNone = 1;
      instruction.line = token.line;
      
      EmitInstruction(instruction);
    } else {
      int valueStart = bytecodeCount;
      ExprResult result = ParseNumericExpression(value);
      int valueEnd = bytecodeCount;
      
      if(varType == VAR_INT && result.type == VAR_FLOAT) {
        CompileError(value.line, "Cannot assign float value to int variable '%s'", token.value);
      }
      
      if(result.next.type != TOKEN_SEMICOLON) {
        CompileError(result.next.line, "Expected ;");
      }
      
      int propagateNone = valueEnd > valueStart &&
        (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR);
      
      Instruction instruction = {0};
      
      instruction.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
      instruction.varIndex = index;
      instruction.propagateNone = propagateNone;
      instruction.line = token.line;
      
      EmitInstruction(instruction);
    }
  }
  
  return LexerNext();
}

// Parse the print statement, from just after the 'print' keyword's identification.
Token ParsePrintStatement() {
  Token value = LexerNext();
  
  // Print string literal
  if(value.type == TOKEN_LIT_STRING) {
    Token semicolon = LexerNext();
    
    if(semicolon.type != TOKEN_SEMICOLON) {
      CompileError(semicolon.line, "Expected ;");
    }
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_PRINT;
    
    strncpy(instruction.text, value.value, INSTRUCTION_MAX_LEN - 1);
    instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  // Print bool literal
  if(value.type == TOKEN_LIT_BOOL) {
    Token semicolon = LexerNext();
    
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
    
    return LexerNext();
  }
  
  if(value.type == TOKEN_IDENTIFIER) {
    int index = FindSymbol(value.value);
    
    if(index != -1 && symbols[index].type == VAR_STR) {
      // Print string variable, optionally an array element
      Token afterName = LexerNext();
      Token next = ParseArrayIndex(index, value, afterName);
      
      if(next.type != TOKEN_SEMICOLON) {
        CompileError(next.line, "Expected ;");
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_PRINT_STR_VAR;
      instruction.varIndex = index;
      instruction.destIsArray = symbols[index].isArray;
      instruction.line = value.line;
      
      EmitInstruction(instruction);
      
      return LexerNext();
    }
    
    if(index != -1 && symbols[index].type == VAR_BOOL) {
      // Print bool variable, optionally an array element
      Token afterName = LexerNext();
      Token next = ParseArrayIndex(index, value, afterName);
      
      if(next.type != TOKEN_SEMICOLON) {
        CompileError(next.line, "Expected ;");
      }
      
      Instruction pushInstruction = {0};
      
      pushInstruction.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
      pushInstruction.varIndex = index;
      pushInstruction.line = value.line;
      
      EmitInstruction(pushInstruction);
      
      Instruction printInstruction = {0};
      
      printInstruction.opcode = OP_PRINT_VALUE;
      printInstruction.valueType = VAR_BOOL;
      printInstruction.propagateNone = 1;
      
      EmitInstruction(printInstruction);
      
      return LexerNext();
    }
    
    // Int or float variable (possibly indexed), or an expression starting with an identifier
    int valueStart = bytecodeCount;
    ExprResult result = ParseNumericExpression(value);
    int valueEnd = bytecodeCount;
    
    if(result.next.type != TOKEN_SEMICOLON) {
      CompileError(result.next.line, "Expected ;");
    }
    
    int isBare = valueEnd > valueStart &&
      (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR);
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_PRINT_VALUE;
    instruction.valueType = result.type;
    instruction.propagateNone = isBare;
    
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  // Number, float literal, or parenthesized expression
  ExprResult result = ParseNumericExpression(value);
  
  if(result.next.type != TOKEN_SEMICOLON) {
    CompileError(result.next.line, "Expected ;");
  }
  
  Instruction instruction = {0};
  
  instruction.opcode = OP_PRINT_VALUE;
  instruction.valueType = result.type;
  
  EmitInstruction(instruction);
  
  return LexerNext();
}

// Parse one full statement, returning the lookahead token that follows it
Token ParseStatement(Token token) {
  if(token.type == TOKEN_ERROR) {
    CompileError(token.line, "%s", token.value);
  }
  
  if(token.type == TOKEN_KW_VAR) {
    return ParseVarDeclaration();
  }
  
  if(token.type == TOKEN_KW_EXIT) {
    Token semicolon = LexerNext();
    
    if(semicolon.type != TOKEN_SEMICOLON) {
      CompileError(semicolon.line, "Expected ;");
    }
    
    Instruction instruction = {0};
    instruction.opcode = OP_HALT;
    EmitInstruction(instruction);
    
    return LexerNext();
  }
  
  if(token.type == TOKEN_IDENTIFIER) {
    return ParseIdentifierStatement(token);
  }
  
  if(token.type == TOKEN_KW_PRINT) {
    return ParsePrintStatement();
  }
  
  if(token.type == TOKEN_KW_IF) {
    return ParseIfStatement(token.column);
  }
  
  CompileError(token.line, "Expected statement");
}

// Convert source into bytecode
void Compile() {
  bytecodeCap = INITIAL_BYTECODE_CAP;
  bytecode = malloc(bytecodeCap * sizeof(Instruction));
  
  Token token = LexerNext();
  
  while(token.type != TOKEN_EOF) {
    token = ParseStatement(token);
  }
  
  // Add program stop
  Instruction halt = {0};
  
  halt.opcode = OP_HALT;
  
  EmitInstruction(halt);
}
