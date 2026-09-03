# Declaring Variables

A variable is declared with the `var` keyword, followed by a data type and a name.

## Syntax

```el
var data_type name;
var data_type name = value;
```

## Supported Data Types

- [int](content/data-types/int.md)
- [float](content/data-types/float.md)
- [bool](content/data-types/bool.md)
- [str](content/data-types/str.md)

## Examples

```el
var int age = 10;
var bool did_you_love_me;
var float price = 19.99;
var str[16] name = "Delfin";
```

## Behavior

- A variable can be declared with or without an initial value.
- If no initial value is given, the variable's value is [`NONE`](content/none-value/none.md) until it's assigned one.
- `str` variables can optionally declare a fixed size using `str[size]`. See [Sized Strings](content/strings/sized-strings.md).
- A variable can also be declared as an array by adding `[size]` after its name, e.g. `var int numbers[5];`. See [Arrays](content/arrays/arrays.md).

## Common Mistakes

Declaring a variable with a name that's already visible in the current or an enclosing scope is a compile-time error, even if the earlier declaration is in a different block:

```el
var int score = 10;

if(true) =
  var int score = 20;
  // error: Variable 'score' already declared
```

See [Scope: Global & Local](content/variables/scope.md) for details.

## Related

- [Assignment & Updates](content/variables/assignment-and-updates.md)
- [The NONE Value](content/none-value/none.md)
- [Arrays](content/arrays/arrays.md)
