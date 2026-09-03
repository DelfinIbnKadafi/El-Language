# Parameters

A function can receive values through parameters. Every parameter is declared with `var`, just like a normal variable, and supports every data type EL has (`int`, `float`, `str`, `bool`), including sized strings.

## Syntax

```el
function function_name(var int number, var float decimal, var str[16] text, var bool flag) =
  // code
```

## Example

```el
function greet(var str[16] name) =
  print name;

greet("Delfin");
```

## Behavior

- Parameters follow the same local-scope rules as any other local variable — see [Scope: Global & Local](content/variables/scope.md).
- A parameter's name cannot reuse a name already visible from an outer scope (no shadowing).
- Calling a function with too few or too many arguments is a compile-time error.
- Passing a `float` value for a parameter declared as `int` is a compile-time error.
- Arrays can also be used as parameters — see [Array Parameters](content/functions/array-parameters.md).

## Related

- [Functions](content/functions/functions.md)
- [Array Parameters](content/functions/array-parameters.md)
- [Return](content/functions/return.md)
