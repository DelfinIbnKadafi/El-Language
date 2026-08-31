# El Language (Ellang)

**El Language**, or **Ellang**, is a custom programming language with clean, easy-to-read syntax, running on its own virtual machine called **ELVM (El Virtual Machine)**. Built from scratch in C, no external dependencies.

```el
function factorial(var int n) =
  if(n <= 1) =
    return 1;
  return n * factorial(n - 1);

var str[16] name = input "What's your name? ";

print "Hello,";
print name;
print factorial(5);
```

![Logo El](https://el-lang.my.id/assets/logo.png)

## What is El Language?

Ellang is a high-level language designed to feel familiar to anyone coming from C, JavaScript, or Python, while keeping its own rules simple and consistent:

- **Indentation-based blocks**, similar to Python, but using `=` to open a block instead of `:`.
- **Statically-ish typed** — types are checked at compile time, with automatic widening (`int` to `float`) and a universal `NONE` value for "no value yet", across every type.
- **No shadowing** — a local variable can never reuse a name still visible from an outer scope, removing a whole class of naming bugs.
- **Real recursion** — every function call, including a function calling itself, gets its own independent copy of its local variables through an actual call stack.
- **Clear errors** — every compile or runtime failure is reported as `file.ell (line) : message`, never a silent crash.

## Architecture

Ellang source code goes through three stages before it runs:

```
source.ell → [Lexer] → tokens → [Parser] → bytecode → [ELVM] → result
```

| Component | Files | Role |
|---|---|---|
| **Lexer** | `lexer.c` / `lexer.h` | Turns source code into tokens (keywords, identifiers, literals, operators, etc). |
| **Parser** | `parser.c` / `parser.h` | Parses syntax, checks types, resolves scope/variables, and compiles everything into bytecode. |
| **ELVM** | `elvm.c` / `elvm.h` | A stack-based virtual machine that executes the bytecode — variable storage, the call stack (for recursion), and every runtime operation. |
| **Entry point** | `main.c` | Wires the three together: reads the file, then runs lexer → parser → VM. |

A couple of the more important design choices behind it:

- **Compile-time scope tracking** — every block (`if`/`for`/`while`/`function`) gets its own scope through a cheap push/pop mechanism, with no runtime allocation needed just to track name visibility.
- **Real call frames for recursion** — a function's local variables live on a separate stack from globals, so recursive calls (`factorial`, `fibonacci`, etc.) never clobber each other.
- **Dynamically sized arrays and strings** — allocated with `malloc` as needed, with no upper limit beyond available memory.

## Example Code

### Hello World

```el
print "Hello, World!";
```

### Variables & Types

```el
var int age = 21;
var str[32] name = "Delfin";
var bool isOnline = true;

print age;
print name;
print isOnline;
```

### Conditionals & Loops

```el
for(var int i = 0; i < 5; i++) =
  if(i < 3) =
    print "small";
  else =
    print "big";
```

### Functions & Recursion

```el
function fib(var int n) =
  if(n <= 1) =
    return n;
  return fib(n - 1) + fib(n - 2);

var int i = 0;
while(i < 8) =
  print fib(i);
  i++;
```

### Arrays as Parameters & Return Values

```el
function sumOf(var int numbers[3]) =
  return numbers[0] + numbers[1] + numbers[2];

function makeArray() =
  var int result[3] = {10, 20, 30};
  return result;

print sumOf([1, 2, 3]);   // list literal
print sumOf(10);          // broadcast, every element becomes 10
print makeArray()[1];     // indexed directly, no need to store it first
```

### Interactive Input & String Comparison

```el
var str[16] answer = input "Continue? (y/n): ";

if(answer == "y") =
  print "Continuing...";
else =
  print "Cancelled.";
```


## Getting Started

Install ELVM in your computer, read [Installation](installation.md)

## License

[Apache License](assets/LICENSE)

Open source. Feel free to explore, contribute, and improve the language.