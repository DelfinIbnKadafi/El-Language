# Compile-Time Errors

A compile-time error means ELVM found a problem in your code before running it. Compilation stops immediately and the program never runs.

## Error Format

```
filename (line) : message
```

`line` is the line number in your `.ell` file where the problem was found. If the problem is in code that came from [`module`](content/modules/module.md) or [`library`](content/modules/library.md), `filename` is the file that code is written in, and `line` is the line inside that file.

## Common Compile-Time Errors

| Message | Cause |
|---|---|
| `Variable 'x' already declared` | A variable name is already visible in the current or an enclosing scope. See [Scope: Global & Local](content/variables/scope.md). |
| `Function 'x' already declared` | Two functions in the same file share the same name. |
| `Functions cannot be nested` | A `function` was declared inside another function's body. |
| `Undefined symbol "x"` | A variable name was used that hasn't been declared. |
| `Undefined function "x"` | A function name was called that hasn't been declared yet. |
| `Too many arguments for function 'x'` / equivalent "too few" case | The number of arguments in a call doesn't match the function's parameter count. |
| `Cannot assign float value to int variable 'x'` | A `float` value was assigned directly to an `int` variable. |
| `String variable 'x' can only be compared with == or !=` | `<`, `>`, `<=`, or `>=` was used on a `str` value. |
| `Variable 'x' is an array and must be indexed` | An array variable was used without `[index]`. |
| `Expected ;` | A statement is missing its terminating semicolon. |
| `Cannot open file "x.ell"` | The file given to `module` (or listed in a library) does not exist. See [Module](content/modules/module.md). |
| `Library "x" not found (expected <path>)` | `library "x";` found no library with that name next to the `elvm` executable. See [Library](content/modules/library.md). |
| `'module' can only be used at the top level, not inside a function or a block` | `module` (or `library`) was used inside a function or a block. |

## Example

```el
var int score = 10;

if(true) =
  var int score = 20;
```

```
file.ell (3) : Variable 'score' already declared
```

## Related

- [Runtime Errors](content/error-handling/runtime-errors.md)
- [Scope: Global & Local](content/variables/scope.md)
- [Module](content/modules/module.md)
- [Library](content/modules/library.md)
