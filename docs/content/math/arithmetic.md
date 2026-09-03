# Arithmetic

EL supports the four basic arithmetic operators: `+`, `-`, `*`, and `/`.

## Syntax

```el
10 + 5
(10 + 5) * 5
```

## Example

```el
var int age;

age = 10 - 1;
print age;
// prints 9
```

## Operator Precedence

`*` and `/` are evaluated before `+` and `-`. Parentheses can be used to control evaluation order.

```el
print 10 + 5 * 5;
// prints 35, not 75

print (10 + 5) * 5;
// prints 75
```

## Unary + and -

A leading `+` or `-` can be applied to a numeric expression:

```el
var int x = -5;
var int y = -(x + 1);
```

## Division: int vs. float

Division behaves differently depending on the operand types:

- If **both** operands are `int`, `/` performs integer division — the result is truncated toward zero, not rounded.
- If **either** operand is `float`, `/` performs real (floating-point) division.

```el
var int a = 7;
var int b = 2;
var float c = 2.0;

print a / b;
// prints 3 (integer division)

print a / c;
// prints 3.5 (float division)
```

## Division by Zero

Dividing by zero is a runtime error:

```el
var int a = 10;
var int b = 0;

print a / b;
// a.ell (line) : Division by zero
```

See [Runtime Errors](content/error-handling/runtime-errors.md) for the exact error format.

## Not Supported

EL does not currently provide a modulo (`%`), exponentiation, or bitwise operator.

## Related

- [int](content/data-types/int.md)
- [float](content/data-types/float.md)
- [Assignment Operators](content/operators/assignment-operators.md)
