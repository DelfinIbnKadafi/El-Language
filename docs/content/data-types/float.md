# float

`float` stores a decimal number.

## Syntax

```el
var float name;
var float name = value;
```

## Example

```el
var float price = 19.99;

print price;
```

## Behavior

- A `float` value can receive an `int` value directly — `int` widens to `float` automatically.
- Division involving at least one `float` operand always performs real division (not integer division). See [Arithmetic](content/math/arithmetic.md).
- Output is printed without trailing zeros, e.g. `3.5` prints as `3.5`, not `3.500000`.
- A `float` variable declared without an initial value holds [`NONE`](content/none-value/none.md) until assigned. When a `NONE` float is used in a math expression, it's treated as `0`.
- `++` and `--` are **not** supported on `float` variables — only `int`. `+=` and `-=` do work on `float`. See [Assignment & Updates](content/variables/assignment-and-updates.md).
- `float` supports array declarations, e.g. `var float prices[5];`. See [Arrays](content/arrays/arrays.md).

## Example: Mixed Math

```el
var int a = 7;
var float b = 2.0;

print a / b;
// prints 3.5
```

## Related

- [int](content/data-types/int.md)
- [Arithmetic](content/math/arithmetic.md)
- [The NONE Value](content/none-value/none.md)
