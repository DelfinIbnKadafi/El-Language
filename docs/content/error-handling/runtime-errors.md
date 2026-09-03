# Runtime Errors

A runtime error means your program compiled successfully but ran into a problem while executing. The program stops at the point where the error occurred — any output printed before that point stays on screen.

## Error Format

```
filename (line) : message
```

`line` is the line number in your `.ell` file that triggered the error.

## Common Runtime Errors

| Message | Cause |
|---|---|
| `Division by zero` | An `int` or `float` division (`/`) had `0` as its divisor. |
| `Array index N out of bounds (size S)` | An array was accessed at an index outside its declared size. |
| `Stack overflow: too many nested/recursive function calls` | A function call chain (typically recursion) exceeded EL's maximum call depth, a safeguard against runaway/infinite recursion. |

## Examples

```el
var int a = 10;
var int b = 0;

print a / b;
```

```
file.ell (4) : Division by zero
```

```el
var int numbers[3] = {1, 2, 3};

print numbers[5];
```

```
file.ell (2) : Array index 5 out of bounds (size 3)
```

```el
function countdown(var int n) =
  if(n <= 0) =
    return 0;
  return countdown(n - 1);

print countdown(1000);
```

```
file.ell (4) : Stack overflow: too many nested/recursive function calls
```

## Behavior Notes

- Runtime errors are unrecoverable — EL does not have a try/catch or error-handling mechanism. If you want to prevent a runtime error, check the condition beforehand (e.g. check a divisor isn't `0` before dividing, or check an index against the array's known size).
- Truncating a string that's too long for its declared size (see [Sized Strings](content/strings/sized-strings.md)) is **not** a runtime error — it happens silently.

## Related

- [Compile-Time Errors](content/error-handling/compile-time-errors.md)
- [Arithmetic](content/math/arithmetic.md)
- [Recursion](content/functions/recursion.md)
