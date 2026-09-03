# int

`int` stores a whole number.

## Syntax

```el
var int name;
var int name = value;
```

## Example

```el
var int age = 21;

print age;
```

## Behavior

- Division between two `int` values (`/`) produces integer division — the result is truncated, not rounded. See [Arithmetic](content/math/arithmetic.md).
- An `int` value is automatically widened to `float` when mixed with a `float` value in a math expression. The reverse is not automatic: assigning a `float` value directly to an `int` variable is a compile-time error.
- `int` is the only type that supports the `++` and `--` operators. See [Assignment & Updates](content/variables/assignment-and-updates.md).
- An `int` variable declared without an initial value holds [`NONE`](content/none-value/none.md) until assigned.
- `int` supports array declarations, e.g. `var int numbers[5];`. See [Arrays](content/arrays/arrays.md).

## Common Mistakes

```el
var int age = 10;
var float bonus = 5.5;

age = bonus;
// error: Cannot assign float value to int variable 'age'
```

## Related

- [float](content/data-types/float.md)
- [Arithmetic](content/math/arithmetic.md)
- [The NONE Value](content/none-value/none.md)
