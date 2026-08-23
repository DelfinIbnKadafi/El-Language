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
  
  // True if this variable is local to the function currently being parsed
  // (a parameter, or a var declared anywhere in its body) rather than global.
  int isLocal;
} Symbol;

// True while parsing a function's parameter list or body, so every symbol
// declared during that span is correctly marked local (see DeclareSymbol).
// Functions cannot nest, so a simple flag (not a counter) is enough.
int insideFunctionBody = 0;

// Symbol table storage
Symbol symbols[MAX_VARIABLES];

// Current symbol table size. At any point during parsing, symbols[0..symbolCount)
// is exactly the set of variables currently visible (every enclosing scope that
// hasn't closed yet, plus the current scope so far) -- see PushScope/PopScope.
int symbolCount = 0;

// Stack of block scopes. Each entry records the symbolCount snapshot from the
// moment that scope was entered, so closing it can "forget" every variable
// declared since then in O(1), making their name (and physical VM slot index)
// available for reuse by a later sibling block. No shadowing is supported: a
// name still visible from any open enclosing scope makes a redeclaration an
// error, exactly like the existing (pre-scoping) duplicate-name check.
#define MAX_SCOPE_DEPTH 64

int scopeStack[MAX_SCOPE_DEPTH];
int scopeDepth = 0;

// Enter a new block scope (if/else/for/while body).
void PushScope(int line) {
  if(scopeDepth >= MAX_SCOPE_DEPTH) {
    CompileError(line, "Too many nested blocks");
  }
  
  scopeStack[scopeDepth++] = symbolCount;
}

// Leave the current block scope, forgetting every variable declared inside it.
void PopScope(void) {
  symbolCount = scopeStack[--scopeDepth];
}

// Find variable index by name, or -1 if not declared
int FindSymbol(char* name) {
  for(int i = 0; i < symbolCount; i++) {
    if(strcmp(symbols[i].name, name) == 0) {
      return i;
    }
  }
  
  return -1;
}

#define MAX_FUNCTIONS 64
#define MAX_PARAMS 16

// A declared function's signature and where its compiled body lives.
typedef struct {
  char name[TOKEN_MAX_LEN];
  
  int paramCount;
  VarType paramTypes[MAX_PARAMS];
  int paramVarIndex[MAX_PARAMS];
  int paramStrSize[MAX_PARAMS];
  
  // Meaningful only when hasReturnValue is true. Inferred from the function's
  // own 'return' statements (int/float widen like everywhere else in this
  // language; bool/str must match every other typed return exactly).
  VarType returnType;
  int hasReturnValue;
  
  // Bytecode index of the function's first real instruction, right after its
  // parameter-binding declares. Known before the body is parsed, so a
  // function calling itself resolves correctly without any forward declaration.
  int bodyStart;
  
  // Bytecode positions of 'return;' statements compiled before hasReturnValue
  // was known yet, needing a fixup once (if) it becomes known -- see
  // ParseReturnStatement.
  int pendingBareReturns[64];
  int pendingBareReturnCount;
  
  // True if a call to this function (from inside its own body, i.e.
  // recursion) was compiled while hasReturnValue was still false. If the
  // function's return kind later does resolve to true, that earlier call
  // was compiled wrongly (assuming no value), which would corrupt the stack
  // at runtime -- checked and rejected at the end of the definition.
  int hadAmbiguousSelfCall;
} FunctionSymbol;

FunctionSymbol functionSymbols[MAX_FUNCTIONS];
int functionSymbolCount = 0;

// Index into functionSymbols of the function currently being defined, or -1
// at the top level. Functions cannot nest, so a single index is enough.
int currentFunctionIndex = -1;

// Find function index by name, or -1 if not declared
int FindFunction(char* name) {
  for(int i = 0; i < functionSymbolCount; i++) {
    if(strcmp(functionSymbols[i].name, name) == 0) {
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
  symbols[symbolCount].isLocal = insideFunctionBody;
  
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
  int srcIsLocal;
  
  // True if this value came from a function call (its string return value is
  // sitting on top of the string-value stack), instead of a literal or an
  // existing variable -- consumers must emit sourceFromArgStack instead of
  // using literal/srcVarIndex.
  int sourceFromArgStack;
  
  // True if the value was literally the NONE keyword
  int isNoneLiteral;
} StringResult;

// Result of parsing a condition: lookahead token and resulting type (always bool once combined)
typedef struct {
  Token next;
  
  VarType type;
} CondResult;

// Result of parsing a bool value: lookahead token, and whether it was NONE
typedef struct {
  Token next;
  
  int isNoneLiteral;
} BoolResult;

// Result of parsing a function call: lookahead token, and whether/what it returns
typedef struct {
  Token next;
  
  int hasReturnValue;
  VarType returnType;
} FunctionCallResult;

// Forward declarations, needed because parentheses / arrays make these mutually recursive
ExprResult ParseNumericExpression(Token token);
ExprResult ParseNumericIdentifier(Token token, Token afterName);
ExprResult ParseNumericTermTail(ExprResult left);
ExprResult ParseNumericExpressionTail(ExprResult left);
CondResult ParseOrExpr(Token token);
CondResult ParseConditionOperand(Token token);
CondResult ParseComparisonTail(CondResult left, int leftStart, int leftEnd);
CondResult ParseAndExprTail(CondResult left);
CondResult ParseOrExprTail(CondResult left);
Token ParseStatement(Token token);
Token ParseIdentifierStatement(Token token, TokenType terminator);
StringResult ParseStringValue(Token token);
BoolResult ParseBoolValue(Token token);
FunctionCallResult ParseFunctionCallArgs(int funcIndex, Token nameToken);

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

// Parse the parenthesized argument list of a call to function 'funcIndex'
// (already resolved by the caller), whose opening '(' has already been
// consumed. Parses one expression per parameter, checked against its
// declared type, and emits the bytecode to pass it in: numeric-family
// arguments (int/float/bool) are left on the numeric eval stack, string
// arguments are pushed onto the string-value stack. Arguments are parsed and
// pushed in their natural left-to-right order; the callee's own
// parameter-binding declares (emitted in reverse parameter order, see
// ParseFunctionDefinition) unwind them correctly. Emits the call itself last.
FunctionCallResult ParseFunctionCallArgs(int funcIndex, Token nameToken) {
  FunctionSymbol* func = &functionSymbols[funcIndex];
  
  Token token = LexerNext();
  int argIndex = 0;
  
  if(token.type != TOKEN_RPAREN) {
    while(1) {
      if(argIndex >= func->paramCount) {
        CompileError(nameToken.line, "Too many arguments for function '%s'", func->name);
      }
      
      VarType paramType = func->paramTypes[argIndex];
      
      if(paramType == VAR_STR) {
        StringResult value = ParseStringValue(token);
        
        Instruction push = {0};
        
        push.opcode = OP_PUSH_STRING_VALUE;
        push.storeNone = value.isNoneLiteral;
        push.srcVarIndex = value.srcVarIndex;
        push.srcIsArray = value.srcIsArray;
        push.srcIsLocal = value.srcIsLocal;
        push.line = nameToken.line;
        
        strncpy(push.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
        push.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
        
        EmitInstruction(push);
        
        token = value.next;
      } else if(paramType == VAR_BOOL) {
        BoolResult value = ParseBoolValue(token);
        
        token = value.next;
      } else {
        // int or float parameter
        ExprResult value = ParseNumericExpression(token);
        
        if(paramType == VAR_INT && value.type == VAR_FLOAT) {
          CompileError(nameToken.line, "Cannot pass a float value for int parameter %d of function '%s'", argIndex + 1, func->name);
        }
        
        token = value.next;
      }
      
      argIndex++;
      
      if(token.type == TOKEN_COMMA) {
        token = LexerNext();
        continue;
      }
      
      break;
    }
  }
  
  if(argIndex < func->paramCount) {
    CompileError(nameToken.line, "Too few arguments for function '%s': expected %d, got %d", func->name, func->paramCount, argIndex);
  }
  
  if(token.type != TOKEN_RPAREN) {
    CompileError(token.line, "Expected )");
  }
  
  Instruction call = {0};
  
  call.opcode = OP_CALL;
  call.jumpTarget = func->bodyStart;
  call.line = nameToken.line;
  
  EmitInstruction(call);
  
  if(!func->hasReturnValue && funcIndex == currentFunctionIndex) {
    // Self-recursive call made while this function's own return kind isn't
    // known yet (an unusual style where the recursive case is written before
    // the base case) -- flagged so the definition can reject it if the kind
    // later does resolve, since that would otherwise silently corrupt the stack.
    functionSymbols[currentFunctionIndex].hadAmbiguousSelfCall = 1;
  }
  
  FunctionCallResult result = {0};
  
  result.next = LexerNext();
  result.hasReturnValue = func->hasReturnValue;
  result.returnType = func->returnType;
  
  return result;
}

// Parse a single numeric value: literal, variable (with optional array index),
// or parenthesized expression. 'token' is the already-read token to interpret.
// Parse an int/float variable reference (optionally indexed), or a function
// call returning an int/float/bool value, given the identifier token and the
// token already read immediately after it (so a caller that had to peek
// ahead to tell this apart from something else, like ParsePrintStatement,
// doesn't cause a token to be skipped by fetching it a second time).
ExprResult ParseNumericIdentifier(Token token, Token afterName) {
  ExprResult result = {0};
  
  if(afterName.type == TOKEN_LPAREN) {
    int funcIndex = FindFunction(token.value);
    
    if(funcIndex == -1) {
      CompileError(token.line, "Undefined function \"%s\"", token.value);
    }
    
    FunctionSymbol* func = &functionSymbols[funcIndex];
    
    if(!func->hasReturnValue || func->returnType == VAR_STR) {
      CompileError(token.line, "Function '%s' does not return a numeric/bool value", token.value);
    }
    
    FunctionCallResult call = ParseFunctionCallArgs(funcIndex, token);
    
    result.next = call.next;
    result.type = call.returnType;
    
    return result;
  }
  
  int index = FindSymbol(token.value);
  
  if(index == -1) {
    CompileError(token.line, "Undefined symbol \"%s\"", token.value);
  }
  
  VarType type = symbols[index].type;
  
  if(type != VAR_INT && type != VAR_FLOAT) {
    CompileError(token.line, "Variable '%s' cannot be used in a math expression", token.value);
  }
  
  result.next = ParseArrayIndex(index, token, afterName);
  
  Instruction instruction = {0};
  
  instruction.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
  instruction.varIndex = index;
  instruction.isLocal = symbols[index].isLocal;
  instruction.line = token.line;
  
  EmitInstruction(instruction);
  
  result.type = type;
  
  return result;
}

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
  
  // Int or float variable reference, optionally indexed, or a function call
  // returning an int/float/bool value
  if(token.type == TOKEN_IDENTIFIER) {
    return ParseNumericIdentifier(token, LexerNext());
  }
  
  CompileError(token.line, "Expected value");
}

// Parse * and / (higher precedence)
// Continue parsing any * and / following an already-parsed leading operand
// ('left'). Split out from ParseNumericTerm so a caller that already parsed
// its own leading term (e.g. print's function-call/identifier handling) can
// still pick up any operators that follow it.
ExprResult ParseNumericTermTail(ExprResult left) {
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

ExprResult ParseNumericTerm(Token token) {
  return ParseNumericTermTail(ParseNumericFactor(token));
}

// Continue parsing any + and - following an already-parsed leading term.
// Split out from ParseNumericExpression for the same reason as
// ParseNumericTermTail above.
ExprResult ParseNumericExpressionTail(ExprResult left) {
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

// Parse + and - (lower precedence)
ExprResult ParseNumericExpression(Token token) {
  return ParseNumericExpressionTail(ParseNumericTerm(token));
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
    Token afterName = LexerNext();
    
    if(afterName.type == TOKEN_LPAREN) {
      int funcIndex = FindFunction(token.value);
      
      if(funcIndex == -1) {
        CompileError(token.line, "Undefined function \"%s\"", token.value);
      }
      
      if(!functionSymbols[funcIndex].hasReturnValue || functionSymbols[funcIndex].returnType != VAR_STR) {
        CompileError(token.line, "Function '%s' does not return a string value", token.value);
      }
      
      FunctionCallResult call = ParseFunctionCallArgs(funcIndex, token);
      
      result.next = call.next;
      result.srcVarIndex = -1;
      result.sourceFromArgStack = 1;
      
      return result;
    }
    
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    if(symbols[index].type != VAR_STR) {
      CompileError(token.line, "Variable '%s' is not a string", token.value);
    }
    
    result.next = ParseArrayIndex(index, token, afterName);
    result.srcVarIndex = index;
    result.srcIsArray = symbols[index].isArray;
    result.srcIsLocal = symbols[index].isLocal;
    
    return result;
  }
  
  CompileError(token.line, "Expected string value");
}

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
    Token afterName = LexerNext();
    
    if(afterName.type == TOKEN_LPAREN) {
      int funcIndex = FindFunction(token.value);
      
      if(funcIndex == -1) {
        CompileError(token.line, "Undefined function \"%s\"", token.value);
      }
      
      if(!functionSymbols[funcIndex].hasReturnValue || functionSymbols[funcIndex].returnType != VAR_BOOL) {
        CompileError(token.line, "Function '%s' does not return a bool value", token.value);
      }
      
      FunctionCallResult call = ParseFunctionCallArgs(funcIndex, token);
      
      result.next = call.next;
      
      return result;
    }
    
    int index = FindSymbol(token.value);
    
    if(index == -1) {
      CompileError(token.line, "Undefined symbol \"%s\"", token.value);
    }
    
    if(symbols[index].type != VAR_BOOL) {
      CompileError(token.line, "Variable '%s' is not a bool", token.value);
    }
    
    Token next = ParseArrayIndex(index, token, afterName);
    
    Instruction instruction = {0};
    
    instruction.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
    instruction.varIndex = index;
    instruction.isLocal = symbols[index].isLocal;
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
      instruction.isLocal = symbols[index].isLocal;
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

// Continue parsing an optional trailing comparison operator + RHS operand,
// given an already-parsed leading operand ('left', spanning bytecode
// [leftStart, leftEnd) ). Split out from ParseComparison so a caller that had
// to parse its own leading operand (return statement parsing peeks ahead to
// tell a string result apart from everything else) can still pick up a
// trailing comparison.
CondResult ParseComparisonTail(CondResult left, int leftStart, int leftEnd) {
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
      instruction.isLocal = symbols[strIndex].isLocal;
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
  
  return ParseComparisonTail(left, leftStart, leftEnd);
}

// Parse 'not' prefixes (higher precedence than 'and', binds to one comparison).
// Chains like 'not not x' are allowed by recursing on itself.
CondResult ParseNotExpr(Token token) {
  if(token.type == TOKEN_KW_NOT) {
    CondResult inner = ParseNotExpr(LexerNext());
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_NOT;
    instruction.line = token.line;
    
    EmitInstruction(instruction);
    
    inner.type = VAR_BOOL;
    
    return inner;
  }
  
  return ParseComparison(token);
}

// Continue parsing any trailing 'and' chain, given an already-parsed leading
// operand. Split out from ParseAndExpr for the same reason as ParseComparisonTail.
CondResult ParseAndExprTail(CondResult left) {
  while(left.next.type == TOKEN_KW_AND) {
    CondResult right = ParseNotExpr(LexerNext());
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_AND;
    
    EmitInstruction(instruction);
    
    left.type = VAR_BOOL;
    left.next = right.next;
  }
  
  return left;
}

// Parse 'and' chains (higher precedence than 'or')
CondResult ParseAndExpr(Token token) {
  return ParseAndExprTail(ParseNotExpr(token));
}

// Continue parsing any trailing 'or' chain, given an already-parsed leading
// operand (which must already have run through any 'and' chain of its own).
// Split out from ParseOrExpr for the same reason as ParseComparisonTail.
CondResult ParseOrExprTail(CondResult left) {
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

// Parse 'or' chains (lower precedence than 'and')
CondResult ParseOrExpr(Token token) {
  return ParseOrExprTail(ParseAndExpr(token));
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
  
  Token afterBlock;
  
  PushScope(assign.line);
  afterBlock = ParseBlock(LexerNext(), chainColumn, assign.line);
  PopScope();
  
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
      
      PushScope(afterElse.line);
      afterBlock = ParseBlock(LexerNext(), chainColumn, afterElse.line);
      PopScope();
    }
    
    bytecode[jumpEndIndex].jumpTarget = bytecodeCount;
  } else {
    // No else, false branch is simply whatever comes after this if
    bytecode[jumpIfFalseIndex].jumpTarget = bytecodeCount;
  }
  
  return afterBlock;
}

// Parse a while statement: while(condition) = body. Structured just like an
// if statement, except the body jumps back to re-check the condition instead
// of continuing past it.
Token ParseWhileStatement(Token whileToken) {
  int loopColumn = whileToken.column;
  
  Token lparen = LexerNext();
  
  if(lparen.type != TOKEN_LPAREN) {
    CompileError(lparen.line, "Expected (");
  }
  
  int loopStart = bytecodeCount;
  
  CondResult condition = ParseOrExpr(LexerNext());
  
  if(condition.next.type != TOKEN_RPAREN) {
    CompileError(condition.next.line, "Expected )");
  }
  
  Token assign = LexerNext();
  
  if(assign.type != TOKEN_OP_ASSIGN) {
    CompileError(assign.line, "Expected =");
  }
  
  Instruction jumpIfFalse = {0};
  
  jumpIfFalse.opcode = OP_JUMP_IF_FALSE;
  
  int jumpIfFalseIndex = EmitInstruction(jumpIfFalse);
  
  PushScope(assign.line);
  Token afterBlock = ParseBlock(LexerNext(), loopColumn, assign.line);
  PopScope();
  
  Instruction jumpBack = {0};
  
  jumpBack.opcode = OP_JUMP;
  jumpBack.jumpTarget = loopStart;
  
  EmitInstruction(jumpBack);
  
  bytecode[jumpIfFalseIndex].jumpTarget = bytecodeCount;
  
  return afterBlock;
}

// Parse a C-style for statement: for(init; condition; increment) = body.
// Any of the three clauses may be left empty (an empty condition behaves as
// always-true). The clauses are laid out in source order (init, condition,
// increment, body), but executed as init, then repeatedly condition/body/
// increment; a pair of unconditional jumps redirects the flow around the
// increment on the way into the loop, and back through it at the end of
// each iteration, so nothing needs to be parsed out of order or duplicated.
Token ParseForStatement(Token forToken) {
  int loopColumn = forToken.column;
  
  Token lparen = LexerNext();
  
  if(lparen.type != TOKEN_LPAREN) {
    CompileError(lparen.line, "Expected (");
  }
  
  // The whole for-statement (init, condition, increment, body) shares one
  // scope, so a counter declared in the init clause stays visible throughout
  // and disappears once the loop ends -- same as for(int i...) in C-like languages.
  PushScope(forToken.line);
  
  // Init clause (optional)
  Token afterInit = LexerNext();
  
  if(afterInit.type != TOKEN_SEMICOLON) {
    afterInit = ParseStatement(afterInit);
  } else {
    afterInit = LexerNext();
  }
  
  // Condition clause (optional, empty means always-true)
  int loopStart = bytecodeCount;
  int hasCondition = afterInit.type != TOKEN_SEMICOLON;
  
  Token afterCond;
  
  if(hasCondition) {
    CondResult condition = ParseOrExpr(afterInit);
    
    if(condition.next.type != TOKEN_SEMICOLON) {
      CompileError(condition.next.line, "Expected ;");
    }
    
    afterCond = LexerNext();
  } else {
    afterCond = LexerNext();
  }
  
  Instruction jumpIfFalse = {0};
  int jumpIfFalseIndex = -1;
  
  if(hasCondition) {
    jumpIfFalse.opcode = OP_JUMP_IF_FALSE;
    jumpIfFalseIndex = EmitInstruction(jumpIfFalse);
  }
  
  Instruction skipIncrement = {0};
  
  skipIncrement.opcode = OP_JUMP;
  
  int skipIncrementIndex = EmitInstruction(skipIncrement);
  
  // Increment clause (optional), laid out here so the loop body can jump
  // back to it at the end of every iteration
  int incrementStart = bytecodeCount;
  
  if(afterCond.type != TOKEN_RPAREN) {
    afterCond = ParseIdentifierStatement(afterCond, TOKEN_RPAREN);
  } else {
    afterCond = LexerNext();
  }
  
  Instruction jumpToStart = {0};
  
  jumpToStart.opcode = OP_JUMP;
  jumpToStart.jumpTarget = loopStart;
  
  EmitInstruction(jumpToStart);
  
  // Body starts here; the very first pass jumps straight to it, skipping increment
  int bodyStart = bytecodeCount;
  
  bytecode[skipIncrementIndex].jumpTarget = bodyStart;
  
  if(afterCond.type != TOKEN_OP_ASSIGN) {
    CompileError(afterCond.line, "Expected =");
  }
  
  Token afterBlock = ParseBlock(LexerNext(), loopColumn, afterCond.line);
  
  Instruction jumpToIncrement = {0};
  
  jumpToIncrement.opcode = OP_JUMP;
  jumpToIncrement.jumpTarget = incrementStart;
  
  EmitInstruction(jumpToIncrement);
  
  if(hasCondition) {
    bytecode[jumpIfFalseIndex].jumpTarget = bytecodeCount;
  }
  
  PopScope();
  
  return afterBlock;
}

// Emit bytecode to parse one value and store it into an array element, assuming
// the destination index has already been pushed onto the stack by the caller
// immediately before this call. Used by both the list and broadcast forms of
// an array initializer, reusing the same per-element store opcodes as a normal
// 'arr[i] = value;' statement. Returns the lookahead token after the value.
Token EmitElementStore(VarType varType, int varIndex, int destIsLocal, Token valueToken) {
  if(varType == VAR_STR) {
    StringResult value = ParseStringValue(valueToken);
    
    Instruction store = {0};
    
    store.opcode = OP_STORE_STR;
    store.varIndex = varIndex;
    store.isLocal = destIsLocal;
    store.destIsArray = 1;
    store.storeNone = value.isNoneLiteral;
    store.srcVarIndex = value.srcVarIndex;
    store.srcIsArray = value.srcIsArray;
    store.srcIsLocal = value.srcIsLocal;
    store.sourceFromArgStack = value.sourceFromArgStack;
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
    store.isLocal = destIsLocal;
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
    store.isLocal = destIsLocal;
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
  store.isLocal = destIsLocal;
  store.line = valueToken.line;
  
  EmitInstruction(store);
  
  return result.next;
}

// Emit "push elementIndex" followed by EmitElementStore, for one list item.
Token EmitArrayElementStore(VarType varType, int varIndex, int destIsLocal, int elementIndex, Token valueToken) {
  Instruction pushIdx = {0};
  
  pushIdx.opcode = OP_PUSH_NUMBER;
  pushIdx.numberValue = elementIndex;
  
  EmitInstruction(pushIdx);
  
  return EmitElementStore(varType, varIndex, destIsLocal, valueToken);
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
    declareInstr.isLocal = symbols[index].isLocal;
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
          
          Token afterItem = EmitArrayElementStore(varType, index, symbols[index].isLocal, elementIndex, itemToken);
          
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
      broadcast.isLocal = symbols[index].isLocal;
      broadcast.storeNone = value.isNoneLiteral;
      broadcast.srcVarIndex = value.srcVarIndex;
      broadcast.srcIsArray = value.srcIsArray;
      broadcast.srcIsLocal = value.srcIsLocal;
      broadcast.sourceFromArgStack = value.sourceFromArgStack;
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
      broadcast.isLocal = symbols[index].isLocal;
      broadcast.storeNone = value.isNoneLiteral;
      broadcast.line = afterAssign.line;
      
      EmitInstruction(broadcast);
      
      afterValue = value.next;
    } else if(afterAssign.type == TOKEN_KW_NONE) {
      Instruction broadcast = {0};
      
      broadcast.opcode = OP_BROADCAST_ARR;
      broadcast.varIndex = index;
      broadcast.isLocal = symbols[index].isLocal;
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
      broadcast.isLocal = symbols[index].isLocal;
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
    instruction.isLocal = symbols[index].isLocal;
    instruction.strSize = strSize;
    instruction.storeNone = value.isNoneLiteral;
    instruction.srcVarIndex = value.srcVarIndex;
    instruction.srcIsArray = value.srcIsArray;
    instruction.srcIsLocal = value.srcIsLocal;
    instruction.sourceFromArgStack = value.sourceFromArgStack;
    
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
        (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR ||
     bytecode[valueEnd - 1].opcode == OP_CALL);
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
    instruction.isLocal = symbols[index].isLocal;
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
        (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR ||
     bytecode[valueEnd - 1].opcode == OP_CALL);
      
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
  instruction.isLocal = symbols[index].isLocal;
  instruction.storeNone = storeNone;
  instruction.propagateNone = propagateNone;
  
  strncpy(instruction.text, name.value, INSTRUCTION_MAX_LEN - 1);
  instruction.text[INSTRUCTION_MAX_LEN - 1] = '\0';
  
  EmitInstruction(instruction);
  
  return LexerNext();
}

// Parse an identifier-led statement: assignment, or increment/decrement,
// for either a scalar variable or one array element.
// Check that 'token' matches the expected statement terminator (';' for a
// normal statement, ')' for a for-loop's increment clause), stopping the
// program with a clear error if not.
void ExpectTerminator(Token token, TokenType terminator) {
  if(token.type != terminator) {
    CompileError(token.line, terminator == TOKEN_RPAREN ? "Expected )" : "Expected ;");
  }
}

// Parse an identifier-led statement: assignment, increment/decrement, or a
// function call (its return value, if any, is discarded), for either a
// scalar variable or one array element. 'terminator' is the token that ends
// the statement: ';' for a normal statement, or ')' when this is used as a
// for-loop's increment clause.
Token ParseIdentifierStatement(Token token, TokenType terminator) {
  Token afterName = LexerNext();
  
  if(afterName.type == TOKEN_LPAREN) {
    int funcIndex = FindFunction(token.value);
    
    if(funcIndex == -1) {
      CompileError(token.line, "Undefined function \"%s\"", token.value);
    }
    
    FunctionCallResult call = ParseFunctionCallArgs(funcIndex, token);
    
    if(call.hasReturnValue) {
      // Statement context: the return value isn't used, discard it. String
      // returns live on a separate stack from numeric ones, so which opcode
      // discards it depends on the return type.
      if(call.returnType == VAR_STR) {
        Instruction popStr = {0};
        
        popStr.opcode = OP_POP_STRING_VALUE;
        popStr.line = token.line;
        
        EmitInstruction(popStr);
      } else {
        Instruction popNum = {0};
        
        popNum.opcode = OP_POP;
        popNum.line = token.line;
        
        EmitInstruction(popNum);
      }
    }
    
    ExpectTerminator(call.next, terminator);
    
    return LexerNext();
  }
  
  int index = FindSymbol(token.value);
  
  if(index == -1) {
    CompileError(token.line, "Undefined symbol \"%s\"", token.value);
  }
  
  int destIsLocal = symbols[index].isLocal;
  
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
    
    ExpectTerminator(semicolon, terminator);
    
    if(isIndexed) {
      // Duplicate the index bytecode: one copy to read, one to write back
      DuplicateInstructions(idxStart, idxEnd);
    }
    
    Instruction pushVar = {0};
    
    pushVar.opcode = isIndexed ? OP_PUSH_ARR : OP_PUSH_VAR;
    pushVar.varIndex = index;
    pushVar.isLocal = destIsLocal;
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
    store.isLocal = destIsLocal;
    store.line = token.line;
    
    EmitInstruction(store);
    
    return LexerNext();
  }
  
  // variabel += expr; or variabel -= expr;
  if(next.type == TOKEN_OP_PLUS_ASSIGN || next.type == TOKEN_OP_MINUS_ASSIGN) {
    VarType compoundType = symbols[index].type;
    
    if(compoundType != VAR_INT && compoundType != VAR_FLOAT) {
      CompileError(token.line, "+= and -= are only supported for int and float variables");
    }
    
    if(isIndexed) {
      // Duplicate the index bytecode: one copy to read the current value, one
      // still on the stack (from the earlier index parse above) to write back
      DuplicateInstructions(idxStart, idxEnd);
    }
    
    Instruction pushCurrent = {0};
    
    pushCurrent.opcode = isIndexed ? OP_PUSH_ARR : OP_PUSH_VAR;
    pushCurrent.varIndex = index;
    pushCurrent.isLocal = destIsLocal;
    pushCurrent.line = token.line;
    
    EmitInstruction(pushCurrent);
    
    Token rhsToken = LexerNext();
    ExprResult rhs = ParseNumericExpression(rhsToken);
    
    if(compoundType == VAR_INT && rhs.type == VAR_FLOAT) {
      CompileError(rhsToken.line, "Cannot use a float value with += or -= on int variable '%s'", token.value);
    }
    
    ExpectTerminator(rhs.next, terminator);
    
    Instruction op = {0};
    
    op.opcode = (next.type == TOKEN_OP_PLUS_ASSIGN) ? OP_ADD : OP_SUB;
    op.line = next.line;
    
    EmitInstruction(op);
    
    Instruction store = {0};
    
    store.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
    store.varIndex = index;
    store.isLocal = destIsLocal;
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
    
    ExpectTerminator(value.next, terminator);
    
    Instruction instruction = {0};
    
    instruction.opcode = OP_STORE_STR;
    instruction.varIndex = index;
    instruction.isLocal = destIsLocal;
    instruction.destIsArray = isIndexed;
    instruction.srcVarIndex = value.srcVarIndex;
    instruction.srcIsArray = value.srcIsArray;
    instruction.srcIsLocal = value.srcIsLocal;
    instruction.sourceFromArgStack = value.sourceFromArgStack;
    instruction.line = token.line;
    
    strncpy(instruction.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
    instruction.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(instruction);
  } else if(varType == VAR_BOOL) {
    int valueStart = bytecodeCount;
    BoolResult value = ParseBoolValue(LexerNext());
    int valueEnd = bytecodeCount;
    
    ExpectTerminator(value.next, terminator);
    
    int isBareCopy = !value.isNoneLiteral && valueEnd > valueStart &&
      (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR ||
     bytecode[valueEnd - 1].opcode == OP_CALL);
    
    Instruction instruction = {0};
    
    instruction.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
    instruction.varIndex = index;
    instruction.isLocal = destIsLocal;
    instruction.storeNone = value.isNoneLiteral;
    instruction.propagateNone = isBareCopy;
    instruction.line = token.line;
    
    EmitInstruction(instruction);
  } else {
    Token value = LexerNext();
    
    if(value.type == TOKEN_KW_NONE) {
      Token semicolon = LexerNext();
      
      ExpectTerminator(semicolon, terminator);
      
      Instruction instruction = {0};
      
      instruction.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
      instruction.varIndex = index;
      instruction.isLocal = destIsLocal;
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
      
      ExpectTerminator(result.next, terminator);
      
      int propagateNone = valueEnd > valueStart &&
        (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR ||
     bytecode[valueEnd - 1].opcode == OP_CALL);
      
      Instruction instruction = {0};
      
      instruction.opcode = isIndexed ? OP_STORE_ARR : OP_STORE_VAR;
      instruction.varIndex = index;
      instruction.isLocal = destIsLocal;
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
    Token afterName = LexerNext();
    
    if(afterName.type == TOKEN_LPAREN) {
      int funcIndex = FindFunction(value.value);
      
      if(funcIndex == -1) {
        CompileError(value.line, "Undefined function \"%s\"", value.value);
      }
      
      FunctionSymbol* func = &functionSymbols[funcIndex];
      
      if(!func->hasReturnValue) {
        CompileError(value.line, "Function '%s' does not return a value", value.value);
      }
      
      FunctionCallResult call = ParseFunctionCallArgs(funcIndex, value);
      
      if(call.next.type != TOKEN_SEMICOLON) {
        CompileError(call.next.line, "Expected ;");
      }
      
      Instruction printInstruction = {0};
      
      if(call.returnType == VAR_STR) {
        printInstruction.opcode = OP_PRINT_STRING_VALUE;
      } else {
        printInstruction.opcode = OP_PRINT_VALUE;
        printInstruction.valueType = call.returnType;
      }
      
      printInstruction.line = value.line;
      
      EmitInstruction(printInstruction);
      
      return LexerNext();
    }
    
    int index = FindSymbol(value.value);
    
    if(index != -1 && symbols[index].type == VAR_STR) {
      // Print string variable, optionally an array element
      Token next = ParseArrayIndex(index, value, afterName);
      
      if(next.type != TOKEN_SEMICOLON) {
        CompileError(next.line, "Expected ;");
      }
      
      Instruction instruction = {0};
      
      instruction.opcode = OP_PRINT_STR_VAR;
      instruction.varIndex = index;
      instruction.isLocal = symbols[index].isLocal;
      instruction.destIsArray = symbols[index].isArray;
      instruction.line = value.line;
      
      EmitInstruction(instruction);
      
      return LexerNext();
    }
    
    if(index != -1 && symbols[index].type == VAR_BOOL) {
      // Print bool variable, optionally an array element
      Token next = ParseArrayIndex(index, value, afterName);
      
      if(next.type != TOKEN_SEMICOLON) {
        CompileError(next.line, "Expected ;");
      }
      
      Instruction pushInstruction = {0};
      
      pushInstruction.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
      pushInstruction.varIndex = index;
      pushInstruction.isLocal = symbols[index].isLocal;
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
    ExprResult result = ParseNumericIdentifier(value, afterName);
    
    // Continue parsing any following * / + - operators, exactly like a
    // normal numeric expression would (ParseNumericIdentifier only covers
    // the single leading term).
    result = ParseNumericTermTail(result);
    result = ParseNumericExpressionTail(result);
    
    int valueEnd = bytecodeCount;
    
    if(result.next.type != TOKEN_SEMICOLON) {
      CompileError(result.next.line, "Expected ;");
    }
    
    int isBare = valueEnd > valueStart &&
      (bytecode[valueEnd - 1].opcode == OP_PUSH_VAR || bytecode[valueEnd - 1].opcode == OP_PUSH_ARR ||
     bytecode[valueEnd - 1].opcode == OP_CALL);
    
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

// Parse a function's whole parameter list, from just after the opening '('.
// Emits the parameter-binding declare instructions in REVERSE parameter
// order, so they correctly unwind the arguments the caller pushed/staged in
// natural left-to-right order (last argument ends up on top, so it must be
// the first one bound). Returns the token after the closing ')'.
Token ParseParameterList(FunctionSymbol* func, Token afterParen) {
  Token names[MAX_PARAMS];
  int strSizes[MAX_PARAMS];
  
  if(afterParen.type == TOKEN_RPAREN) {
    return LexerNext();
  }
  
  Token token = afterParen;
  
  while(1) {
    Token varKeyword = token;
    
    if(varKeyword.type != TOKEN_KW_VAR) {
      CompileError(varKeyword.line, "Expected 'var'");
    }
    
    Token typeToken = LexerNext();
    VarType paramType;
    
    if(typeToken.type == TOKEN_TYPE_INT) {
      paramType = VAR_INT;
    } else if(typeToken.type == TOKEN_TYPE_FLOAT) {
      paramType = VAR_FLOAT;
    } else if(typeToken.type == TOKEN_TYPE_STR) {
      paramType = VAR_STR;
    } else if(typeToken.type == TOKEN_TYPE_BOOL) {
      paramType = VAR_BOOL;
    } else {
      CompileError(typeToken.line, "Expected type");
    }
    
    Token afterType = LexerNext();
    int strSize = 0;
    
    if(paramType == VAR_STR && afterType.type == TOKEN_LBRACKET) {
      Token sizeToken = LexerNext();
      
      if(sizeToken.type != TOKEN_LIT_NUMBER) {
        CompileError(sizeToken.line, "Expected string size");
      }
      
      strSize = ParsePositiveSize(sizeToken, "String size");
      
      Token closeBracket = LexerNext();
      
      if(closeBracket.type != TOKEN_RBRACKET) {
        CompileError(closeBracket.line, "Expected ]");
      }
      
      afterType = LexerNext();
    }
    
    Token name = afterType;
    
    if(name.type != TOKEN_IDENTIFIER) {
      CompileError(name.line, "Expected parameter name");
    }
    
    if(func->paramCount >= MAX_PARAMS) {
      CompileError(name.line, "Too many parameters (max %d)", MAX_PARAMS);
    }
    
    // Arrays are not supported as parameters yet
    int index = DeclareSymbol(name.value, paramType, 0, name.line);
    
    func->paramTypes[func->paramCount] = paramType;
    func->paramVarIndex[func->paramCount] = index;
    func->paramStrSize[func->paramCount] = strSize;
    
    names[func->paramCount] = name;
    strSizes[func->paramCount] = strSize;
    
    func->paramCount++;
    
    Token afterParam = LexerNext();
    
    if(afterParam.type == TOKEN_COMMA) {
      token = LexerNext();
      continue;
    }
    
    if(afterParam.type == TOKEN_RPAREN) {
      break;
    }
    
    CompileError(afterParam.line, "Expected , or )");
  }
  
  // Emit parameter-binding declares in reverse order (see function comment)
  for(int i = func->paramCount - 1; i >= 0; i--) {
    Instruction declareInstr = {0};
    
    declareInstr.opcode = (func->paramTypes[i] == VAR_INT) ? OP_DECLARE_INT :
                           (func->paramTypes[i] == VAR_FLOAT) ? OP_DECLARE_FLOAT :
                           (func->paramTypes[i] == VAR_BOOL) ? OP_DECLARE_BOOL : OP_DECLARE_STR;
    declareInstr.varIndex = func->paramVarIndex[i];
    declareInstr.isLocal = 1;
    declareInstr.strSize = strSizes[i];
    declareInstr.line = names[i].line;
    
    if(func->paramTypes[i] == VAR_STR) {
      declareInstr.sourceFromArgStack = 1;
      declareInstr.srcVarIndex = -1;
    }
    
    strncpy(declareInstr.text, names[i].value, INSTRUCTION_MAX_LEN - 1);
    declareInstr.text[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(declareInstr);
  }
  
  return LexerNext();
}

// Parse a function definition: 'function name(params) = body'. The
// function's name is registered (and its body's bytecode start location
// known) before its own body is parsed, so it can call itself recursively.
// Return type is inferred entirely from its own 'return' statements (see
// ParseReturnStatement); no annotation is written or read here.
Token ParseFunctionDefinition(Token functionToken) {
  Token name = LexerNext();
  
  if(name.type != TOKEN_IDENTIFIER) {
    CompileError(name.line, "Expected function name");
  }
  
  if(FindFunction(name.value) != -1) {
    CompileError(name.line, "Function '%s' already declared", name.value);
  }
  
  if(functionSymbolCount >= MAX_FUNCTIONS) {
    CompileError(name.line, "Too many functions");
  }
  
  Token lparen = LexerNext();
  
  if(lparen.type != TOKEN_LPAREN) {
    CompileError(lparen.line, "Expected (");
  }
  
  int funcIndex = functionSymbolCount++;
  FunctionSymbol* func = &functionSymbols[funcIndex];
  
  memset(func, 0, sizeof(FunctionSymbol));
  strncpy(func->name, name.value, TOKEN_MAX_LEN - 1);
  func->name[TOKEN_MAX_LEN - 1] = '\0';
  
  // Skip over the function's own body during normal top-to-bottom execution;
  // it only ever runs when actually called. Backpatched below, once the
  // body's end is known.
  Instruction skipJump = {0};
  
  skipJump.opcode = OP_JUMP;
  skipJump.line = name.line;
  
  int skipJumpIndex = EmitInstruction(skipJump);
  
  insideFunctionBody = 1;
  currentFunctionIndex = funcIndex;
  
  PushScope(name.line);
  
  Token afterParen = LexerNext();
  
  // bodyStart must point at the very first parameter-binding instruction
  // (about to be emitted by ParseParameterList below), not at the user's own
  // body statements after it -- otherwise OP_CALL would jump straight past
  // parameter binding entirely.
  func->bodyStart = bytecodeCount;
  
  Token afterParams = ParseParameterList(func, afterParen);
  
  if(afterParams.type != TOKEN_OP_ASSIGN) {
    CompileError(afterParams.line, "Expected =");
  }
  
  Token afterBlock = ParseBlock(LexerNext(), functionToken.column, afterParams.line);
  
  // Implicit 'return NONE;' in case a code path falls off the end of the
  // body without an explicit return.
  Instruction implicitReturn = {0};
  
  implicitReturn.opcode = OP_RETURN;
  implicitReturn.line = afterParams.line;
  
  if(func->hasReturnValue) {
    Instruction pushNone = {0};
    
    pushNone.line = afterParams.line;
    
    if(func->returnType == VAR_STR) {
      pushNone.opcode = OP_PUSH_STRING_VALUE;
      pushNone.storeNone = 1;
    } else {
      pushNone.opcode = OP_PUSH_NUMBER;
      pushNone.numberValue = 0;
      pushNone.storeNone = 1;
    }
    
    EmitInstruction(pushNone);
  }
  
  EmitInstruction(implicitReturn);
  
  PopScope();
  
  insideFunctionBody = 0;
  currentFunctionIndex = -1;
  
  // Now that the function's return kind is fully known, reject it if an
  // earlier self-recursive call was compiled while it still looked void --
  // that call would be missing a value the real function now does push,
  // corrupting the stack at runtime.
  if(func->hasReturnValue && func->hadAmbiguousSelfCall) {
    CompileError(name.line,
      "Function '%s' calls itself before its first 'return' with a value; "
      "put at least one 'return <value>;' before any recursive call", name.value);
  }
  
  // Fix up any bare 'return;' compiled before the return kind was known
  for(int i = 0; i < func->pendingBareReturnCount; i++) {
    int idx = func->pendingBareReturns[i];
    
    if(func->hasReturnValue && func->returnType == VAR_STR) {
      bytecode[idx].opcode = OP_PUSH_STRING_VALUE;
      bytecode[idx].storeNone = 1;
    } else {
      bytecode[idx].opcode = OP_PUSH_NUMBER;
      bytecode[idx].numberValue = 0;
      bytecode[idx].storeNone = 1;
    }
  }
  
  skipJump.jumpTarget = bytecodeCount;
  bytecode[skipJumpIndex] = skipJump;
  
  return afterBlock;
}

// Parse a 'return' statement. Return type is inferred from the expression's
// type: the first typed return fixes the function's return kind; a later
// typed return must match (int/float widen to float, like everywhere else in
// this language; bool/str must match exactly). A bare 'return;' contributes
// no type information by itself -- if the function's kind isn't known yet
// when this bare return is compiled, a placeholder is recorded and fixed up
// once (if) a later typed return resolves it (see ParseFunctionDefinition).
Token ParseReturnStatement(Token returnToken) {
  if(currentFunctionIndex == -1) {
    CompileError(returnToken.line, "'return' can only be used inside a function");
  }
  
  FunctionSymbol* func = &functionSymbols[currentFunctionIndex];
  
  Token afterReturn = LexerNext();
  
  // 'return;' and 'return NONE;' behave identically: contribute NONE of
  // whatever the function's return kind eventually turns out to be, without
  // forcing any particular type.
  int isBareNoneReturn = afterReturn.type == TOKEN_SEMICOLON;
  Token semicolonAfterNone = {0};
  
  if(afterReturn.type == TOKEN_KW_NONE) {
    semicolonAfterNone = LexerNext();
    
    if(semicolonAfterNone.type == TOKEN_SEMICOLON) {
      isBareNoneReturn = 1;
    }
  }
  
  if(isBareNoneReturn) {
    Instruction pushValue = {0};
    
    pushValue.line = returnToken.line;
    
    if(func->hasReturnValue) {
      if(func->returnType == VAR_STR) {
        pushValue.opcode = OP_PUSH_STRING_VALUE;
        pushValue.storeNone = 1;
      } else {
        pushValue.opcode = OP_PUSH_NUMBER;
        pushValue.numberValue = 0;
        pushValue.storeNone = 1;
      }
      
      EmitInstruction(pushValue);
    } else {
      // Not known yet: emit a numeric placeholder and remember to fix it up.
      pushValue.opcode = OP_PUSH_NUMBER;
      pushValue.numberValue = 0;
      pushValue.storeNone = 1;
      
      int idx = EmitInstruction(pushValue);
      
      if(func->pendingBareReturnCount < 64) {
        func->pendingBareReturns[func->pendingBareReturnCount++] = idx;
      }
    }
    
    Instruction ret = {0};
    
    ret.opcode = OP_RETURN;
    ret.line = returnToken.line;
    
    EmitInstruction(ret);
    
    return LexerNext();
  }
  
  // Decide which kind of value this return produces by peeking at the
  // leading token(s) -- there's no type keyword here to dispatch on like
  // 'var <type>' has, so infer from the expression itself.
  Token afterName = {0};
  int isFunctionCallLookingAtLparen = 0;
  
  if(afterReturn.type == TOKEN_IDENTIFIER) {
    afterName = LexerNext();
    isFunctionCallLookingAtLparen = (afterName.type == TOKEN_LPAREN);
  }
  
  int isStringReturn =
    afterReturn.type == TOKEN_LIT_STRING ||
    (afterReturn.type == TOKEN_IDENTIFIER && !isFunctionCallLookingAtLparen &&
      FindSymbol(afterReturn.value) != -1 && symbols[FindSymbol(afterReturn.value)].type == VAR_STR) ||
    (isFunctionCallLookingAtLparen && FindFunction(afterReturn.value) != -1 &&
      functionSymbols[FindFunction(afterReturn.value)].hasReturnValue &&
      functionSymbols[FindFunction(afterReturn.value)].returnType == VAR_STR);
  
  VarType returnedType;
  Token afterExpr;
  
  if(isStringReturn) {
    StringResult value = {0};
    
    if(isFunctionCallLookingAtLparen) {
      int funcIndex = FindFunction(afterReturn.value);
      FunctionCallResult call = ParseFunctionCallArgs(funcIndex, afterReturn);
      
      value.next = call.next;
      value.srcVarIndex = -1;
      value.sourceFromArgStack = 1;
    } else if(afterReturn.type == TOKEN_IDENTIFIER) {
      int index = FindSymbol(afterReturn.value);
      
      value.next = ParseArrayIndex(index, afterReturn, afterName);
      value.srcVarIndex = index;
      value.srcIsArray = symbols[index].isArray;
      value.srcIsLocal = symbols[index].isLocal;
    } else {
      value = ParseStringValue(afterReturn);
    }
    
    returnedType = VAR_STR;
    afterExpr = value.next;
    
    if(!func->hasReturnValue) {
      func->hasReturnValue = 1;
      func->returnType = VAR_STR;
    } else if(func->returnType != VAR_STR) {
      CompileError(returnToken.line, "Function '%s' returns inconsistent types", func->name);
    }
    
    Instruction pushValue = {0};
    
    pushValue.opcode = OP_PUSH_STRING_VALUE;
    pushValue.storeNone = value.isNoneLiteral;
    pushValue.srcVarIndex = value.srcVarIndex;
    pushValue.srcIsArray = value.srcIsArray;
    pushValue.srcIsLocal = value.srcIsLocal;
    pushValue.sourceFromArgStack = value.sourceFromArgStack;
    pushValue.line = returnToken.line;
    
    strncpy(pushValue.stringLiteral, value.literal, INSTRUCTION_MAX_LEN - 1);
    pushValue.stringLiteral[INSTRUCTION_MAX_LEN - 1] = '\0';
    
    EmitInstruction(pushValue);
  } else {
    // Not a string: build the leading operand (numeric, bool variable,
    // function call, or a fully bracketed/negated sub-expression), then run
    // it through the exact same comparison/and/or/not grammar 'if'/'while'
    // use, so things like 'return a > b and c < d;' or
    // 'return not isValid();' work correctly, not just bare values.
    int leftStart = bytecodeCount;
    CondResult condResult = {0};
    
    if(afterReturn.type == TOKEN_KW_NOT) {
      condResult = ParseNotExpr(afterReturn);
    } else if(isFunctionCallLookingAtLparen) {
      int funcIndex = FindFunction(afterReturn.value);
      FunctionCallResult call = ParseFunctionCallArgs(funcIndex, afterReturn);
      
      ExprResult numResult = {0};
      
      numResult.next = call.next;
      numResult.type = call.returnType;
      
      numResult = ParseNumericTermTail(numResult);
      numResult = ParseNumericExpressionTail(numResult);
      
      condResult.next = numResult.next;
      condResult.type = numResult.type;
    } else if(afterReturn.type == TOKEN_IDENTIFIER) {
      int index = FindSymbol(afterReturn.value);
      
      if(index != -1 && symbols[index].type == VAR_BOOL) {
        Token afterIdx = ParseArrayIndex(index, afterReturn, afterName);
        
        Instruction push = {0};
        
        push.opcode = symbols[index].isArray ? OP_PUSH_ARR : OP_PUSH_VAR;
        push.varIndex = index;
        push.isLocal = symbols[index].isLocal;
        push.line = afterReturn.line;
        
        EmitInstruction(push);
        
        condResult.next = afterIdx;
        condResult.type = VAR_BOOL;
      } else {
        // int/float identifier: parse the leading factor, then greedily
        // consume any trailing * / + - before treating it as ready for an
        // optional comparison (matches normal arithmetic precedence, e.g.
        // 'a + b > c' means '(a + b) > c').
        ExprResult numResult = ParseNumericIdentifier(afterReturn, afterName);
        
        numResult = ParseNumericTermTail(numResult);
        numResult = ParseNumericExpressionTail(numResult);
        
        condResult.next = numResult.next;
        condResult.type = numResult.type;
      }
    } else if(afterReturn.type == TOKEN_LPAREN || afterReturn.type == TOKEN_LIT_NUMBER ||
              afterReturn.type == TOKEN_OP_SUB || afterReturn.type == TOKEN_OP_ADD) {
      // Numeric leading term (parenthesized sub-expression, number literal,
      // or unary +/-): same reasoning as the identifier case above -- parse
      // it as a full numeric expression (so parens correctly support
      // trailing arithmetic like '(a + b) / 2.0'), then optionally compare.
      ExprResult numResult = ParseNumericFactor(afterReturn);
      
      numResult = ParseNumericTermTail(numResult);
      numResult = ParseNumericExpressionTail(numResult);
      
      condResult.next = numResult.next;
      condResult.type = numResult.type;
    } else {
      condResult = ParseConditionOperand(afterReturn);
    }
    
    int leftEnd = bytecodeCount;
    
    condResult = ParseComparisonTail(condResult, leftStart, leftEnd);
    condResult = ParseAndExprTail(condResult);
    condResult = ParseOrExprTail(condResult);
    
    returnedType = condResult.type;
    afterExpr = condResult.next;
    
    if(!func->hasReturnValue) {
      func->hasReturnValue = 1;
      func->returnType = returnedType;
    } else if(func->returnType == VAR_INT && returnedType == VAR_FLOAT) {
      func->returnType = VAR_FLOAT;
    } else if(func->returnType == VAR_FLOAT && returnedType == VAR_INT) {
      // Already the wider type, fine
    } else if(func->returnType != returnedType) {
      CompileError(returnToken.line, "Function '%s' returns inconsistent types", func->name);
    }
  }
  
  ExpectTerminator(afterExpr, TOKEN_SEMICOLON);
  
  Instruction ret = {0};
  
  ret.opcode = OP_RETURN;
  ret.line = returnToken.line;
  
  EmitInstruction(ret);
  
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
    return ParseIdentifierStatement(token, TOKEN_SEMICOLON);
  }
  
  if(token.type == TOKEN_KW_PRINT) {
    return ParsePrintStatement();
  }
  
  if(token.type == TOKEN_KW_IF) {
    return ParseIfStatement(token.column);
  }
  
  if(token.type == TOKEN_KW_WHILE) {
    return ParseWhileStatement(token);
  }
  
  if(token.type == TOKEN_KW_FOR) {
    return ParseForStatement(token);
  }
  
  if(token.type == TOKEN_KW_FUNCTION) {
    if(insideFunctionBody) {
      CompileError(token.line, "Functions cannot be nested");
    }
    
    return ParseFunctionDefinition(token);
  }
  
  if(token.type == TOKEN_KW_RETURN) {
    return ParseReturnStatement(token);
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
