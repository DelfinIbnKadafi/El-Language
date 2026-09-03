# Functions

`function` creates a reusable, named block of code.

## Declaration

```el
function function_name() =
  // code
```

## Calling

```el
function_name();
```

## Example

```el
function say_hello() =
  print "Hello!";

say_hello();
```

## Behavior

- A function can call itself directly (recursion) without needing to be declared first — see [Recursion](content/functions/recursion.md).
- A function **must be declared before** it's called elsewhere in the file. EL compiles top-to-bottom in a single pass, so calling a function that's defined further down the file is a compile-time error (`Undefined function`).
- Functions cannot be nested — declaring a `function` inside another function's body is a compile-time error.
- A function can accept values through parameters, and can optionally return a value. See [Parameters](content/functions/parameters.md) and [Return](content/functions/return.md).

## Related

- [Parameters](content/functions/parameters.md)
- [Return](content/functions/return.md)
- [Recursion](content/functions/recursion.md)
