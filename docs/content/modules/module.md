# Module

`module` copies the whole content of another `.ell` file into your program, exactly where the `module` statement is written. It lets you split a big program into several files.

## Syntax

```el
module "file.ell";
```

## Example

`math_utils.ell`:

```el
var int base = 100;

function double_it(var int n) =
  return n * 2;
```

`main.ell`:

```el
module "math_utils.ell";

print base;
print double_it(21);
// prints 100 and 42
```

The result is the same as if the variable and the function were written at the top of `main.ell`.

## Behavior

- The path is relative to the file that contains the `module` statement, **not** to the folder where you run `elvm`. An absolute path also works.
- `module` can only be used at the top level of a file. Using it inside a function or inside an `if` / `for` / `while` block is a compile-time error.
- EL compiles top-to-bottom, so a `module` statement must come **before** the code that uses its variables and functions. See [Functions](content/functions/functions.md).
- The code of the module shares the same global scope as the file that includes it. A name that is already declared cannot be declared again, see [Scope: Global & Local](content/variables/scope.md).
- A module can use `module` too, and the path is relative to that module's own file.
- Every file is included only once. A second `module` statement for the same file is ignored, so two files can include each other without looping forever.
- `module` is a reserved word, it cannot be used as a variable or function name.

## Errors Point to the Right File

When an error happens in code that came from a module, the error message shows the module's file name and the line **inside that file**, not the line of the `module` statement.

`shapes.ell`:

```el
// shapes.ell
function ratio(var int a, var int b) =
  return a / b;
```

`main.ell`:

```el
module "shapes.ell";

print ratio(4, 0);
```

```
shapes.ell (3) : Division by zero
```

The same applies to compile-time errors.

## Common Errors

| Message | Cause |
|---|---|
| `Cannot open file "x.ell"` | The file does not exist, or cannot be read. |
| `Expected string after 'module'` | The path was not written as a string, e.g. `module util;`. |
| `'module' can only be used at the top level, not inside a function or a block` | `module` was used inside a function or a block. |
| `Variable 'x' already declared` | A module and the file that includes it declare the same variable name. |

## Related

- [Library](content/modules/library.md)
- [Scope: Global & Local](content/variables/scope.md)
- [Compile-Time Errors](content/error-handling/compile-time-errors.md)
