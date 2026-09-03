# Return

`return` sends a value back to whoever called the function. There's no return type to declare — EL infers it automatically from the value used in the function's `return` statement(s).

## Syntax

```el
function add(var int a, var int b) =
  return a + b;

print add(3, 4);
// prints 7
```

## Using a Function Call as a Value

Because a function call produces a value, it can be used directly anywhere a value is expected — inside `if` conditions, math expressions, or as another function's argument.

```el
function is_adult(var int age) =
  return age >= 18;

if(is_adult(20)) =
  print "Allowed";
```

## Bare Return

A `return` with no value simply exits the function early, without producing anything usable by the caller.

```el
function check(var int n) =
  if(n < 0) =
    return;
  print n;
```

## Behavior

- Every `return` in a function must produce a consistent type. Returning `int`/`float` from different `return` statements is fine (they widen), but returning `bool` or `str` must match exactly across every `return` in the same function.
- A function that returns an array must return an array on every possible path — a bare `return;` is not allowed in that case. See [Returning Arrays](content/functions/returning-arrays.md).

## Related

- [Functions](content/functions/functions.md)
- [Returning Arrays](content/functions/returning-arrays.md)
- [Recursion](content/functions/recursion.md)
