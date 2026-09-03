# Recursion

A function can call itself. This works without needing a forward declaration, because a function's name becomes callable as soon as its declaration starts, before its body is compiled.

## Example

```el
function factorial(var int n) =
  if(n <= 1) =
    return 1;
  return n * factorial(n - 1);

print factorial(5);
// prints 120
```

The base case (`return 1;`) and the recursive case (`return n * factorial(n - 1);`) can appear in either order in the function body — EL resolves the function's return type correctly either way.

```el
function f(var int n) =
  if(n > 1) =
    return n * f(n - 1);
  return 1;

print f(5);
// also prints 120
```

## Recursion Depth

Every recursive call adds a new frame to the call stack. EL enforces a maximum call depth to guard against runaway recursion. Exceeding it stops the program with a runtime "Stack overflow" error:

```el
function countdown(var int n) =
  if(n <= 0) =
    return 0;
  return countdown(n - 1);

print countdown(1000000);
// countdown.ell (4) : Stack overflow: too many nested/recursive function calls
```

See [Runtime Errors](content/error-handling/runtime-errors.md) for the exact error format.

## Related

- [Functions](content/functions/functions.md)
- [Return](content/functions/return.md)
- [Runtime Errors](content/error-handling/runtime-errors.md)
