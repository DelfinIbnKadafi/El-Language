# Assignment & Updates

Once a variable is declared, its value can be changed using assignment or one of EL's update operators.

## Basic Assignment

```el
name = value;
```

Example:

```el
var int age;

age = 10;
```

## Update Operators

| Operator | Meaning | Supported types |
|---|---|---|
| `++` | Increment by 1 | `int` only |
| `--` | Decrement by 1 | `int` only |
| `+=` | Add and assign | `int`, `float` |
| `-=` | Subtract and assign | `int`, `float` |

Example:

```el
var int age = 10;
var float price = 19.99;

age++;
age--;
age += 10;
age -= 10;
price += 5.0;
```

> **Note:** `++` and `--` only work on `int` variables. Using them on a `float`, `bool`, or `str` variable is a compile-time error. `+=` and `-=` work on `int` and `float` variables only — not `bool` or `str`. This restriction isn't obvious from the operator names alone, so keep it in mind when writing update expressions.

## Assigning to Array Elements

An array element is updated the same way, but must always be indexed:

```el
var int numbers[5];

numbers[0] = 10;
numbers[0]++;
numbers[0] += 5;
```

Assigning to an array variable without an index is a compile-time error — see [Arrays](content/arrays/arrays.md).

## Type Rules

- Assigning a `float` value to an `int` variable using `+=`/`-=` is a compile-time error. A plain `=` assignment from `float` to `int` is also not allowed directly; see [int](content/data-types/int.md) and [float](content/data-types/float.md) for widening rules.
- Assigning to a `str` variable follows the rules described in [Sized Strings](content/strings/sized-strings.md), including automatic truncation.

## Related

- [Declaring Variables](content/variables/declaring-variables.md)
- [Arithmetic](content/math/arithmetic.md)
- [Comparison Operators](content/operators/comparison-operators.md)
