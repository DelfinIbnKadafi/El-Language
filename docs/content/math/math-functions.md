# Math Functions

## abs()

Returns the absolute value of an `int` or `float`.

```el
print abs(-5);
// prints 5
```

## min() / max()

Return the smaller or larger of two `int`/`float` values.

```el
print min(3, 7);
// prints 3

print max(3.5, 2);
// prints 3.5
```

### int vs. float result

`abs()`, `min()`, and `max()` follow the same widening rule as EL's own arithmetic operators: the result is `float` if any argument is `float`, and `int` only when every argument is `int`.

## pow()

Returns `base` raised to the power `exp`. Always returns a `float`, regardless of the argument types.

```el
print pow(2, 10);
// prints 1024
```

## sqrt()

Returns the square root of an `int` or `float`. Always returns a `float`. Taking the square root of a negative number is a runtime error.

```el
print sqrt(16);
// prints 4
```

## round() / floor() / ceil()

Round a value to the nearest whole number (`round`), down (`floor`), or up (`ceil`). All three always return an `int`.

```el
print round(3.6);
// prints 4

print floor(3.9);
// prints 3

print ceil(3.1);
// prints 4
```

## Behavior

- `abs()`, `pow()`, `sqrt()`, `min()`, and `max()` treat a [`NONE`](content/none-value/none.md) argument as `0`, the same way any other math expression does.
- `round()`, `floor()`, and `ceil()` are different: since they act on a single value rather than combining several, a `NONE` argument produces a `NONE` result.

```el
var float x;

print round(x);
// prints NONE

print abs(x);
// prints 0
```

## Related

- [Arithmetic](content/math/arithmetic.md)
- [random()](content/math/random.md)
- [Type Conversion](content/data-types/type-conversion.md)
